#include "JsonLoader.h"
#include "../debug/Logger.h"

#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <raylib.h>

namespace
{
    Color parseColor(const std::string& colorName)
    {
        if (colorName == "RED")
        {
            return RED;
        }

        if (colorName == "GREEN")
        {
            return GREEN;
        }

        if (colorName == "BLUE")
        {
            return BLUE;
        }

        if (colorName == "BLACK")
        {
            return BLACK;
        }

        if (colorName == "YELLOW")
        {
            return YELLOW;
        }

        if (colorName == "ORANGE")
        {
            return ORANGE;
        }

        if (colorName == "PURPLE")
        {
            return PURPLE;
        }

        if (colorName == "GRAY")
        {
            return GRAY;
        }

        if (colorName == "WHITE")
        {
            return WHITE;
        }

        Logger::warning(
            "json",
            "Unknown color '" + colorName + "', using WHITE"
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

        file >> data;
        return true;
    }

    RuntimeObject parseDrawable(const nlohmann::json& object)
    {
        const std::string name =
            object.value("name", "Unnamed");

        float x = 0.0f;
        float y = 0.0f;

        if (object.contains("origin"))
        {
            x =object["origin"]["x"].get<float>();
            y =object["origin"]["y"].get<float>();
        }

        std::string shapeType = "block";
        float width = 0.0f;
        float height = 0.0f;

        if (object.contains("shape"))
        {
            const auto& shape = object["shape"];

            shapeType = shape.value("type", "block");

            width =
                shape["size"]["width"].get<float>();

            height =
                shape["size"]["height"].get<float>();
        }
        else
        {
            width =
                object["size"]["width"].get<float>();

            height =
                object["size"]["height"].get<float>();
        }

        Color color = WHITE;

        if (object.contains("shape"))
        {
            const auto& shape = object["shape"];

            if (shape.contains("color"))
            {
                color = parseColor(
                    shape["color"].get<std::string>()
                );
            }
        }

        RuntimeObject runtimeObject(
            name,
            Vector2{ x, y },
            Vector2{ width, height },
            color
        );

        runtimeObject.shapeType = shapeType;

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

        if (object.contains("motion"))
        {
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

        if (object.contains("bounds"))
        {
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

        if (object.contains("behavior"))
        {
            const auto& behavior = object["behavior"];

            if (behavior.contains("scripts"))
            {
                for (const auto& script : behavior["scripts"])
                {
                    runtimeObject.scripts.push_back(
                        script.get<std::string>()
                    );
                }
            }
        }

        if (object.contains("spawns"))
        {
            const auto& spawns = object["spawns"];

            for (auto it = spawns.begin(); it != spawns.end(); ++it)
            {
                SpawnDefinition spawn;

                const auto& spawnData = it.value();

                spawn.prefab =
                    spawnData["prefab"].get<std::string>();

                spawn.offset = Vector2{ 0.0f, 0.0f };

                if (spawnData.contains("offset"))
                {
                    spawn.offset.x =
                        spawnData["offset"].value("x", 0.0f);

                    spawn.offset.y =
                        spawnData["offset"].value("y", 0.0f);
                }

                runtimeObject.spawns[it.key()] = spawn;
            }
        }

        return runtimeObject;
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

        if (data.contains("shape"))
        {
            objects.push_back(parseDrawable(data));
            return;
        }

        if (!data.contains("children"))
        {
            return;
        }

        for (const auto& child : data["children"])
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
                if (child.contains("origin"))
                {
                    objects.push_back(parseDrawable(child));
                }
                else
                {
                    Logger::warning(
                        "json",
                        "Inline child without origin is not supported yet"
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

GameConfig JsonLoader::loadGameConfig(const std::string& path)
{
    GameConfig config;

    nlohmann::json data;

    if (!loadJson(path, data))
    {
        return config;
    }

    if (data.contains("name"))
    {
        config.name = data["name"].get<std::string>();
    }

    if (data.contains("description"))
    {
        config.description = data["description"].get<std::string>();
    }

    if (data.contains("screen"))
    {
        const auto& screen = data["screen"];

        if (screen.contains("title"))
        {
            config.screenTitle = screen["title"].get<std::string>();
        }

        if (screen.contains("width"))
        {
            config.screenWidth = screen["width"].get<int>();
        }

        if (screen.contains("height"))
        {
            config.screenHeight = screen["height"].get<int>();
        }
    }

    if (data.contains("scale"))
    {
        config.scale = data["scale"].get<int>();
    }

    if (data.contains("program"))
    {
        const auto& program = data["program"];

        if (program.contains("scripts"))
        {
            for (const auto& script : program["scripts"])
            {
                config.programScripts.push_back(
                    script.get<std::string>()
                );
            }
        }
    }

    if (data.contains("children"))
    {
        for (const auto& child : data["children"])
        {
            config.children.push_back(
                child.get<std::string>()
            );
        }
    }

    return config;
}