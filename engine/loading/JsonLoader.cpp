#include "JsonLoader.h"
#include "../debug/Logger.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <unordered_map>
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

    std::optional<int> hexValue(char value)
    {
        if (value >= '0' && value <= '9')
        {
            return value - '0';
        }

        if (value >= 'a' && value <= 'f')
        {
            return 10 + value - 'a';
        }

        if (value >= 'A' && value <= 'F')
        {
            return 10 + value - 'A';
        }

        return std::nullopt;
    }

    std::optional<Color> parseHexColor(const std::string& value)
    {
        if (value.size() != 4 && value.size() != 7)
        {
            return std::nullopt;
        }

        if (value[0] != '#')
        {
            return std::nullopt;
        }

        auto readDigit = [](char digit) -> std::optional<unsigned char>
        {
            const auto parsed = hexValue(digit);

            if (!parsed.has_value())
            {
                return std::nullopt;
            }

            return static_cast<unsigned char>(
                parsed.value() * 17
            );
        };

        auto readByte = [](char high, char low) -> std::optional<unsigned char>
        {
            const auto parsedHigh = hexValue(high);
            const auto parsedLow = hexValue(low);

            if (!parsedHigh.has_value() || !parsedLow.has_value())
            {
                return std::nullopt;
            }

            return static_cast<unsigned char>(
                parsedHigh.value() * 16 + parsedLow.value()
            );
        };

        std::optional<unsigned char> r;
        std::optional<unsigned char> g;
        std::optional<unsigned char> b;

        if (value.size() == 4)
        {
            r = readDigit(value[1]);
            g = readDigit(value[2]);
            b = readDigit(value[3]);
        }
        else
        {
            r = readByte(value[1], value[2]);
            g = readByte(value[3], value[4]);
            b = readByte(value[5], value[6]);
        }

        if (!r.has_value() || !g.has_value() || !b.has_value())
        {
            return std::nullopt;
        }

        return Color{
            r.value(),
            g.value(),
            b.value(),
            255
        };
    }

    Color parseColor(std::string colorName)
    {
        const auto hexColor =
            parseHexColor(colorName);

        if (hexColor.has_value())
        {
            return hexColor.value();
        }

        std::transform(
            colorName.begin(),
            colorName.end(),
            colorName.begin(),
            [](unsigned char value)
            {
                return static_cast<char>(std::tolower(value));
            }
        );

        static const std::unordered_map<std::string, Color> colors = {
            { "lightgray", LIGHTGRAY },
            { "lightgrey", LIGHTGRAY },
            { "gray", GRAY },
            { "grey", GRAY },
            { "darkgray", DARKGRAY },
            { "darkgrey", DARKGRAY },
            { "yellow", YELLOW },
            { "gold", GOLD },
            { "orange", ORANGE },
            { "pink", PINK },
            { "red", RED },
            { "maroon", MAROON },
            { "green", GREEN },
            { "lime", LIME },
            { "darkgreen", DARKGREEN },
            { "skyblue", SKYBLUE },
            { "blue", BLUE },
            { "darkblue", DARKBLUE },
            { "purple", PURPLE },
            { "violet", VIOLET },
            { "darkpurple", DARKPURPLE },
            { "beige", BEIGE },
            { "brown", BROWN },
            { "darkbrown", DARKBROWN },
            { "white", WHITE },
            { "black", BLACK },
            { "blank", BLANK },
            { "magenta", MAGENTA },
            { "raywhite", RAYWHITE }
        };

        const auto it =
            colors.find(colorName);

        if (it != colors.end())
        {
            return it->second;
        }

        Logger::warning(
            "json",
            "Unknown color '" + colorName + "', using white"
        );

        return WHITE;
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
        definition.shapeType = shape.value("type", "block");
        definition.shapeMode = shape.value("mode", "fill");

        if (shape.contains("color"))
        {
            definition.color =
                parseColor(shape["color"].get<std::string>());
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
            bounds.value("mode", definition.boundsMode);

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
            collision.value("type", definition.collisionType);

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
            object.value("spawn", definition.spawnMode);

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
