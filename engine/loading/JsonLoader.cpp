#include "JsonLoader.h"
#include "../debug/Logger.h"

#include <algorithm>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <raylib.h>

namespace
{
    Color parseColor(const std::string& colorName)
    {
        if (colorName == "red") return RED;
        if (colorName == "green") return GREEN;
        if (colorName == "blue") return BLUE;
        if (colorName == "black") return BLACK;
        if (colorName == "yellow") return YELLOW;
        if (colorName == "orange") return ORANGE;
        if (colorName == "purple") return PURPLE;
        if (colorName == "gray") return GRAY;
        if (colorName == "white") return WHITE;
        if (colorName == "brown") return BROWN;

        Logger::warning(
            "json",
            "Unknown color '" + colorName + "', using white"
        );

        return WHITE;
    }

    std::string ensureJsonExtension(const std::string& path)
    {
        if (path.ends_with(".json"))
        {
            return path;
        }

        return path + ".json";
    }

    std::filesystem::path resolveChildPath(
        const std::filesystem::path& parentFile,
        const std::string& child
    )
    {
        return parentFile.parent_path() / ensureJsonExtension(child);
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
        const nlohmann::json & override
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
    )
    {
        if (!object.is_object() || !object.contains("like"))
        {
            resolved = object;
            resolved["__sourceFile"] = currentFile.generic_string();
            return true;
        }

        const std::string likePath =
            object["like"].get<std::string>();

        const auto basePath =
            resolveChildPath(currentFile, likePath);

        nlohmann::json base;

        if (!loadJson(basePath, base))
        {
            Logger::error(
                "json",
                "Like target could not be loaded: " + basePath.string()
            );

            return false;
        }

        base["__sourceFile"] =
            basePath.generic_string();

        nlohmann::json override =
            object;

        override.erase("like");

        mergeJson(base, override);

        resolved = base;

        return true;
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
        if (!object.contains("origin"))
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
        RuntimeObject& runtimeObject
    )
    {
        if (!hasShape(object))
        {
            return;
        }

        const auto& shape = object["shape"];

        runtimeObject.shapeType =
            shape.value("type", "block");

        runtimeObject.shapeMode =
            shape.value("mode", "fill");

        if (shape.contains("color"))
        {
            runtimeObject.color =
                parseColor(shape["color"].get<std::string>());
        }

        if (shape.contains("radius"))
        {
            runtimeObject.radius =
                shape["radius"].get<float>();
        }
        else
        {
            runtimeObject.radius =
                std::max(
                    runtimeObject.size.x,
                    runtimeObject.size.y
                ) / 2.0f;
        }

        runtimeObject.points.clear();

        if (shape.contains("points") && shape["points"].is_array())
        {
            for (const auto& point : shape["points"])
            {
                runtimeObject.points.push_back(
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
        RuntimeObject& runtimeObject
    )
    {
        if (!object.contains("motion"))
        {
            return;
        }

        const auto& motion = object["motion"];

        if (motion.contains("rotationSpeed"))
        {
            runtimeObject.rotationSpeed =
                motion["rotationSpeed"].get<float>();
        }

        if (motion.contains("acceleration"))
        {
            runtimeObject.acceleration =
                motion["acceleration"].get<float>();
        }

        if (motion.contains("inertia"))
        {
            runtimeObject.inertia =
                motion["inertia"].get<float>();
        }

        if (motion.contains("maxSpeed"))
        {
            runtimeObject.maxSpeed =
                motion["maxSpeed"].get<float>();
        }
    }

    void parseBounds(
        const nlohmann::json& object,
        RuntimeObject& runtimeObject
    )
    {
        if (!object.contains("bounds"))
        {
            return;
        }

        const auto& bounds = object["bounds"];

        if (bounds.contains("mode"))
        {
            runtimeObject.boundsMode =
                bounds["mode"].get<std::string>();
        }

        if (bounds.contains("overflow"))
        {
            runtimeObject.boundsOverflow =
                bounds["overflow"].get<bool>();
        }
    }

    void parseBehavior(
        const nlohmann::json& object,
        RuntimeObject& runtimeObject
    )
    {
        if (!object.contains("behavior"))
        {
            return;
        }

        const auto& behavior = object["behavior"];

        if (!behavior.contains("scripts"))
        {
            return;
        }

        for (const auto& script : behavior["scripts"])
        {
            runtimeObject.scripts.push_back(
                script.get<std::string>()
            );
        }
    }

    void parseCollision(
        const nlohmann::json& object,
        RuntimeObject& runtimeObject
    )
    {
        if (!object.contains("collision") ||
            !object["collision"].is_object())
        {
            return;
        }

        const auto& collision =
            object["collision"];

        if (collision.contains("type"))
        {
            runtimeObject.collisionType =
                collision["type"].get<std::string>();
        }

        if (collision.contains("radius"))
        {
            runtimeObject.collisionRadius =
                collision["radius"].get<float>();
        }

        if (collision.contains("active"))
        {
            runtimeObject.collisionActive =
                collision["active"].get<bool>();
        }

        runtimeObject.collisionWith.clear();

        if (collision.contains("with") &&
            collision["with"].is_array())
        {
            for (const auto& group : collision["with"])
            {
                runtimeObject.collisionWith.push_back(
                    group.get<std::string>()
                );
            }
        }

        if (
            runtimeObject.collisionType == "circle" &&
            runtimeObject.collisionRadius <= 0.0f
            )
        {
            runtimeObject.collisionRadius =
                std::max(
                    runtimeObject.size.x,
                    runtimeObject.size.y
                ) / 2.0f;
        }
    }

    void parseSpawns(
        const nlohmann::json& object,
        const std::filesystem::path& sourceFile,
        RuntimeObject& runtimeObject
    )
    {
        if (!object.contains("spawns") || !object["spawns"].is_object())
        {
            return;
        }

        const auto& spawns = object["spawns"];

        for (auto it = spawns.begin(); it != spawns.end(); ++it)
        {
            const auto& spawnData = it.value();

            if (!spawnData.is_object())
            {
                Logger::warning(
                    "json",
                    "Invalid spawn '" + it.key() + "': expected object"
                );

                continue;
            }

            if (!spawnData.contains("prefab") || !spawnData["prefab"].is_string())
            {
                Logger::warning(
                    "json",
                    "Invalid spawn '" + it.key() + "': missing prefab"
                );

                continue;
            }

            SpawnDefinition spawn;

            spawn.prefab =
                spawnData["prefab"].get<std::string>();

            spawn.basePath =
                sourceFile.parent_path().generic_string();

            spawn.offset = Vector2{ 0.0f, 0.0f };
            spawn.hasOffset = false;

            if (spawnData.contains("offset") && spawnData["offset"].is_object())
            {
                spawn.hasOffset = true;

                spawn.offset.x =
                    spawnData["offset"].value("x", 0.0f);

                spawn.offset.y =
                    spawnData["offset"].value("y", 0.0f);
            }

            runtimeObject.spawns[it.key()] = spawn;
        }
    }

    RuntimeObject parseRuntimeObject(
        const nlohmann::json& object,
        const std::filesystem::path& currentFile
    )
    {
        const std::string name =
            object.value("name", "Unnamed");

        std::filesystem::path sourceFile =
            currentFile;

        if (object.contains("__sourceFile"))
        {
            sourceFile =
                object["__sourceFile"].get<std::string>();
        }

        const bool objectHasShape =
            hasShape(object);

        const bool objectHasSize =
            hasRootSize(object);

        const bool objectHasOrigin =
            object.contains("origin");

        const Vector2 origin =
            parseOrigin(object);

        const Vector2 size =
            parseSize(object);

        RuntimeObject runtimeObject(
            name,
            origin,
            size,
            WHITE
        );

        runtimeObject.hasOrigin =
            objectHasOrigin;

        if (!objectHasShape && !objectHasSize)
        {
            runtimeObject.visible = false;
        }

        parseShape(object, runtimeObject);

        if (object.contains("speed"))
        {
            runtimeObject.speed =
                object["speed"].get<float>();

            runtimeObject.originSpeed =
                runtimeObject.speed;
        }

        if (object.contains("angle"))
        {
            runtimeObject.angle =
                object["angle"].get<float>();
        }

        if (object.contains("group"))
        {
            runtimeObject.group =
                object["group"].get<std::string>();
        }

        if (object.contains("visible"))
        {
            runtimeObject.visible =
                object["visible"].get<bool>();
        }

        parseMotion(object, runtimeObject);
        parseBounds(object, runtimeObject);
        parseBehavior(object, runtimeObject);
        parseCollision(object, runtimeObject);
        parseSpawns(object, sourceFile, runtimeObject);

        return runtimeObject;
    }

    void loadInlineChild(
        const std::filesystem::path& path,
        const nlohmann::json& child,
        const std::string& fallbackName,
        std::vector<RuntimeObject>& objects
    )
    {
        if (!child.is_object())
        {
            return;
        }

        nlohmann::json childData =
            child;

        if (!fallbackName.empty() && !childData.contains("name"))
        {
            childData["name"] = fallbackName;
        }

        nlohmann::json resolvedChild;

        if (!resolveLike(path, childData, resolvedChild))
        {
            Logger::warning(
                "json",
                "Skipping child because like could not be resolved"
            );

            return;
        }

        objects.push_back(
            parseRuntimeObject(resolvedChild, path)
        );
    }

    void loadNodeRecursive(
        const std::filesystem::path& path,
        std::vector<RuntimeObject>& objects
    )
    {
        nlohmann::json data;

        if (!loadJson(path, data))
        {
            return;
        }

        const bool isRuntimeObject =
            hasShape(data) ||
            data.contains("behavior") ||
            data.contains("spawns") ||
            data.contains("collision");

        if (isRuntimeObject)
        {
            objects.push_back(
                parseRuntimeObject(data, path)
            );
        }

        if (!data.contains("children"))
        {
            return;
        }

        const auto& children =
            data["children"];

        if (children.is_array())
        {
            for (const auto& child : children)
            {
                if (child.is_string())
                {
                    const auto childPath =
                        resolveChildPath(
                            path,
                            child.get<std::string>()
                        );

                    loadNodeRecursive(childPath, objects);
                }
                else if (child.is_object())
                {
                    loadInlineChild(path, child, "", objects);
                }
            }
        }
        else if (children.is_object())
        {
            for (auto it = children.begin(); it != children.end(); ++it)
            {
                if (it.value().is_string())
                {
                    const auto childPath =
                        resolveChildPath(
                            path,
                            it.value().get<std::string>()
                        );

                    loadNodeRecursive(childPath, objects);
                }
                else if (it.value().is_object())
                {
                    loadInlineChild(
                        path,
                        it.value(),
                        it.key(),
                        objects
                    );
                }
            }
        }
    }
}

std::vector<RuntimeObject> JsonLoader::loadObjects(
    const std::string& path
)
{
    std::vector<RuntimeObject> objects;

    loadNodeRecursive(
        std::filesystem::path(path),
        objects
    );

    return objects;
}
