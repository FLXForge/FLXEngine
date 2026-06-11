#include "JsonLoader.h"
#include "../debug/Logger.h"
#include "../tools/ColorParser.h"
#include "../tools/TextTools.h"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <vector>

namespace
{
    std::string ensureExtension(
        const std::string& path,
        const std::string& extension
    )
    {
        if (path.ends_with(extension))
        {
            return path;
        }

        return path + extension;
    }

    std::filesystem::path resolvePath(
        const std::filesystem::path& parentFile,
        const std::string& child
    )
    {
        return parentFile.parent_path() / ensureExtension(child, ".json");
    }

    bool loadJson(
        const std::filesystem::path& path,
        nlohmann::json& data
    )
    {
        std::ifstream file(path);

        if (!file.is_open())
        {
            Logger::error(
                "json",
                "The file could not be opened " + path.string()
            );

            return false;
        }

        try
        {
            file >> data;
        }
        catch (const nlohmann::json::parse_error& error)
        {
            Logger::error(
                "json",
                "Invalid JSON in " +
                path.string() +
                ": " +
                error.what()
            );

            return false;
        }

        return true;
    }

    void mergeJson(
        nlohmann::json& base,
        const nlohmann::json& override
    )
    {
        for (auto it = override.begin(); it != override.end(); ++it)
        {
            const std::string key = it.key();

            if (
                base.contains(key) &&
                base[key].is_object() &&
                it.value().is_object()
                )
            {
                mergeJson(base[key], it.value());
            }
            else
            {
                base[key] = it.value();
            }
        }
    }

    bool resolveLike(
        const std::filesystem::path& currentFile,
        const nlohmann::json& object,
        nlohmann::json& resolved
    );

    bool resolveBlockReference(
        const std::filesystem::path& currentFile,
        nlohmann::json& object,
        const std::string& key
    )
    {
        if (!object.contains(key) || !object[key].is_string())
        {
            return true;
        }

        const auto blockPath =
            resolvePath(
                currentFile,
                object[key].get<std::string>()
            );

        nlohmann::json block;

        if (!loadJson(blockPath, block))
        {
            return false;
        }

        nlohmann::json resolvedBlock;

        if (!resolveLike(blockPath, block, resolvedBlock))
        {
            return false;
        }

        if (resolvedBlock.contains(key))
        {
            object[key] = resolvedBlock[key];
        }
        else
        {
            object[key] = resolvedBlock;
        }

        return true;
    }

    bool resolveBlockReferences(
        const std::filesystem::path& currentFile,
        nlohmann::json& object
    )
    {
        static const std::vector<std::string> blockKeys = {
            "shape",
            "motion",
            "bounds",
            "collision",
            "behavior"
        };

        for (const auto& key : blockKeys)
        {
            if (!resolveBlockReference(currentFile, object, key))
            {
                return false;
            }
        }

        return true;
    }

    bool resolveLike(
        const std::filesystem::path& currentFile,
        const nlohmann::json& object,
        nlohmann::json& resolved
    )
    {
        if (!object.is_object())
        {
            return false;
        }

        if (!object.contains("like"))
        {
            resolved = object;
            resolved["__sourceFile"] = currentFile.generic_string();
            return resolveBlockReferences(currentFile, resolved);
        }

        const auto basePath =
            resolvePath(
                currentFile,
                object["like"].get<std::string>()
            );

        nlohmann::json base;

        if (!loadJson(basePath, base))
        {
            Logger::error(
                "json",
                "Like target could not be loaded: " + basePath.string()
            );

            return false;
        }

        nlohmann::json resolvedBase;

        if (!resolveLike(basePath, base, resolvedBase))
        {
            return false;
        }

        nlohmann::json override =
            object;

        override.erase("like");

        mergeJson(resolvedBase, override);

        resolved = resolvedBase;
        resolved["__sourceFile"] = basePath.generic_string();

        return resolveBlockReferences(currentFile, resolved);
    }

    bool hasShape(const nlohmann::json& object)
    {
        return object.contains("shape") &&
            object["shape"].is_object();
    }

    bool hasRootSize(const nlohmann::json& object)
    {
        return object.contains("size") &&
            object["size"].is_object();
    }

    Vector2 parseOrigin(const nlohmann::json& object)
    {
        if (!object.contains("origin") || !object["origin"].is_object())
        {
            return Vector2{ 0.0f, 0.0f };
        }

        const auto& origin = object["origin"];

        return Vector2{
            origin.value("x", 0.0f),
            origin.value("y", 0.0f)
        };
    }

    Vector2 parseSize(const nlohmann::json& object)
    {
        if (hasShape(object))
        {
            const auto& shape = object["shape"];

            if (shape.contains("size") && shape["size"].is_object())
            {
                const auto& size = shape["size"];

                return Vector2{
                    size.value("width", 0.0f),
                    size.value("height", 0.0f)
                };
            }
        }

        if (hasRootSize(object))
        {
            const auto& size = object["size"];

            return Vector2{
                size.value("width", 0.0f),
                size.value("height", 0.0f)
            };
        }

        return Vector2{ 0.0f, 0.0f };
    }

    void parseShape(
        const nlohmann::json& object,
        ObjectDefinition& definition
    )
    {
        if (!hasShape(object))
        {
            definition.hasVisual = hasRootSize(object);
            return;
        }

        const auto& shape = object["shape"];

        definition.hasVisual = true;
        definition.shapeType =
            TextTools::toLower(shape.value("type", "block"));
        definition.shapeMode =
            TextTools::toLower(shape.value("mode", "fill"));
        definition.textContent =
            shape.value("content", definition.textContent);

        if (shape.contains("color"))
        {
            definition.color =
                ColorParser::parse(
                    shape["color"].get<std::string>(),
                    WHITE
                );
        }

        if (shape.contains("radius"))
        {
            definition.radius =
                shape["radius"].get<float>();
        }
        else
        {
            definition.radius =
                std::max(
                    definition.size.x,
                    definition.size.y
                ) / 2.0f;
        }

        definition.points.clear();

        if (shape.contains("points") && shape["points"].is_array())
        {
            for (const auto& point : shape["points"])
            {
                definition.points.push_back(
                    Vector2{
                        point.value("x", 0.0f),
                        point.value("y", 0.0f)
                    }
                );
            }
        }
    }

    void parseMotion(
        const nlohmann::json& object,
        ObjectDefinition& definition
    )
    {
        if (!object.contains("motion") || !object["motion"].is_object())
        {
            return;
        }

        const auto& motion = object["motion"];

        definition.rotationSpeed =
            motion.value("rotationSpeed", definition.rotationSpeed);

        definition.acceleration =
            motion.value("acceleration", definition.acceleration);

        definition.inertia =
            motion.value("inertia", definition.inertia);

        definition.maxSpeed =
            motion.value("maxSpeed", definition.maxSpeed);
    }

    void parseBounds(
        const nlohmann::json& object,
        ObjectDefinition& definition
    )
    {
        if (!object.contains("bounds") || !object["bounds"].is_object())
        {
            return;
        }

        const auto& bounds = object["bounds"];

        definition.boundsMode =
            TextTools::toLower(
                bounds.value("mode", definition.boundsMode)
            );

        definition.boundsOverflow =
            bounds.value("overflow", definition.boundsOverflow);
    }

    void parseBehavior(
        const nlohmann::json& object,
        ObjectDefinition& definition
    )
    {
        if (!object.contains("behavior") || !object["behavior"].is_object())
        {
            return;
        }

        const auto& behavior = object["behavior"];

        if (!behavior.contains("scripts") || !behavior["scripts"].is_array())
        {
            return;
        }

        for (const auto& script : behavior["scripts"])
        {
            definition.scripts.push_back(
                script.get<std::string>()
            );
        }
    }

    void parseCollision(
        const nlohmann::json& object,
        ObjectDefinition& definition
    )
    {
        if (!object.contains("collision") || !object["collision"].is_object())
        {
            return;
        }

        const auto& collision = object["collision"];

        definition.collisionType =
            TextTools::toLower(
                collision.value("type", definition.collisionType)
            );

        definition.collisionRadius =
            collision.value("radius", definition.collisionRadius);

        definition.collisionActive =
            collision.value("active", definition.collisionActive);

        definition.collisionWith.clear();

        if (collision.contains("with") && collision["with"].is_array())
        {
            for (const auto& group : collision["with"])
            {
                definition.collisionWith.push_back(
                    group.get<std::string>()
                );
            }
        }

        if (
            definition.collisionType == "circle" &&
            definition.collisionRadius <= 0.0f
            )
        {
            definition.collisionRadius =
                std::max(
                    definition.size.x,
                    definition.size.y
                ) / 2.0f;
        }
    }

    nlohmann::json normalizeSoundValue(
        const nlohmann::json& value,
        const std::filesystem::path& currentFile
    )
    {
        if (value.is_string())
        {
            const auto soundPath =
                resolvePath(
                    currentFile,
                    value.get<std::string>()
                );

            nlohmann::json soundData;

            if (!loadJson(soundPath, soundData))
            {
                return nlohmann::json{};
            }

            nlohmann::json resolvedSound;

            if (!resolveLike(soundPath, soundData, resolvedSound))
            {
                return nlohmann::json{};
            }

            if (resolvedSound.contains("sound"))
            {
                return resolvedSound["sound"];
            }

            return resolvedSound;
        }

        if (value.is_object() && value.contains("like"))
        {
            nlohmann::json resolvedSound;

            if (!resolveLike(currentFile, value, resolvedSound))
            {
                return nlohmann::json{};
            }

            if (resolvedSound.contains("sound"))
            {
                return resolvedSound["sound"];
            }

            return resolvedSound;
        }

        return value;
    }

    void parseSounds(
        const nlohmann::json& object,
        const std::filesystem::path& currentFile,
        ObjectDefinition& definition
    )
    {
        if (!object.contains("sounds") || !object["sounds"].is_object())
        {
            return;
        }

        const auto& sounds =
            object["sounds"];

        for (auto it = sounds.begin(); it != sounds.end(); ++it)
        {
            const nlohmann::json data =
                normalizeSoundValue(
                    it.value(),
                    currentFile
                );

            if (!data.is_object())
            {
                Logger::warning(
                    "json",
                    "Invalid sound '" + it.key() + "': expected object"
                );

                continue;
            }

            SoundDefinition sound;
            sound.wave =
                TextTools::toLower(data.value("wave", sound.wave));
            sound.frequency =
                data.value("frequency", sound.frequency);
            sound.duration =
                data.value("duration", sound.duration);
            sound.volume =
                data.value("volume", sound.volume);

            definition.sounds[it.key()] =
                sound;
        }
    }

    ObjectDefinition parseDefinition(
        const nlohmann::json& object,
        const std::filesystem::path& currentFile,
        const std::string& id
    );

    nlohmann::json normalizeChildValue(const nlohmann::json& value)
    {
        if (value.is_string())
        {
            return nlohmann::json{
                { "like", value.get<std::string>() }
            };
        }

        return value;
    }

    void parseChildren(
        const nlohmann::json& object,
        const std::filesystem::path& currentFile,
        ObjectDefinition& definition
    )
    {
        if (!object.contains("children"))
        {
            return;
        }

        if (!object["children"].is_object())
        {
            Logger::warning(
                "json",
                "Invalid children in " + currentFile.string() +
                ": expected object"
            );

            return;
        }

        const auto& children = object["children"];

        for (auto it = children.begin(); it != children.end(); ++it)
        {
            nlohmann::json childData =
                normalizeChildValue(it.value());

            if (!childData.is_object())
            {
                Logger::warning(
                    "json",
                    "Invalid child '" + it.key() + "': expected object"
                );

                continue;
            }

            nlohmann::json resolvedChild;

            if (!resolveLike(currentFile, childData, resolvedChild))
            {
                Logger::warning(
                    "json",
                    "Skipping child '" + it.key() +
                    "' because it could not be resolved"
                );

                continue;
            }

            definition.children[it.key()] =
                parseDefinition(
                    resolvedChild,
                    currentFile,
                    it.key()
                );
        }
    }

    ObjectDefinition parseDefinition(
        const nlohmann::json& object,
        const std::filesystem::path& currentFile,
        const std::string& id
    )
    {
        ObjectDefinition definition;
        definition.id = id;

        const std::filesystem::path sourceFile =
            object.value(
                "__sourceFile",
                currentFile.generic_string()
            );

        definition.sourcePath =
            sourceFile.generic_string();

        definition.spawnMode =
            TextTools::toLower(
                object.value("spawn", definition.spawnMode)
            );

        definition.hasOffset =
            object.contains("offset") && object["offset"].is_object();

        if (definition.hasOffset)
        {
            const auto& offset = object["offset"];

            definition.offset = Vector2{
                offset.value("x", 0.0f),
                offset.value("y", 0.0f)
            };
        }

        definition.hasOrigin =
            object.contains("origin") && object["origin"].is_object();

        definition.origin =
            parseOrigin(object);

        definition.size =
            parseSize(object);

        definition.group =
            object.value("group", definition.group);

        definition.visible =
            object.value("visible", definition.visible);

        definition.speed =
            object.value("speed", definition.speed);

        definition.angle =
            object.value("angle", definition.angle);

        parseShape(object, definition);
        parseMotion(object, definition);
        parseBounds(object, definition);
        parseBehavior(object, definition);
        parseCollision(object, definition);
        parseSounds(object, sourceFile, definition);
        parseChildren(object, sourceFile, definition);

        return definition;
    }
}

std::string JsonLoader::resolveProjectPath(
    const std::string& projectPath,
    const std::string& path,
    const std::string& extension
)
{
    const std::filesystem::path resolved =
        std::filesystem::path(projectPath) /
        ensureExtension(path, extension);

    return resolved.generic_string();
}

std::string JsonLoader::resolveReferencedPath(
    const std::string& sourceFile,
    const std::string& path,
    const std::string& extension
)
{
    const std::filesystem::path resolved =
        std::filesystem::path(sourceFile).parent_path() /
        ensureExtension(path, extension);

    return resolved.generic_string();
}

ObjectDefinition JsonLoader::loadObjectDefinition(
    const std::string& path
)
{
    const std::filesystem::path objectPath(path);

    nlohmann::json data;

    if (!loadJson(objectPath, data))
    {
        return ObjectDefinition{};
    }

    nlohmann::json resolved;

    if (!resolveLike(objectPath, data, resolved))
    {
        return ObjectDefinition{};
    }

    return parseDefinition(
        resolved,
        objectPath,
        objectPath.stem().generic_string()
    );
}
