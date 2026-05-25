#include "JsonLoader.h"
#include "../debug/Logger.h"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <raylib.h>

std::vector<RuntimeObject> JsonLoader::loadObjects(
    const std::string& path
)
{
    std::vector<RuntimeObject> objects;

    std::ifstream file(path);

    if (!file.is_open())
    {
        Logger::error(
            "json",
            "The file could not be opened " + path
        );
        return objects;
    }

    nlohmann::json data;
    file >> data;

    for (const auto& object : data["children"])
    {
        const std::string name =
            object["name"].get<std::string>();

        const float x =
            object["origin"]["x"].get<float>();

        const float y =
            object["origin"]["y"].get<float>();

        const float width =
            object["size"]["width"].get<float>();

        const float height =
            object["size"]["height"].get<float>();

        RuntimeObject runtimeObject(
            name,
            Vector2{ x, y },
            Vector2{ width, height },
            WHITE
        );

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

        objects.push_back(runtimeObject);
    }

    return objects;
}

GameConfig JsonLoader::loadGameConfig(const std::string& path)
{
    GameConfig config;

    std::ifstream file(path);

    if (!file.is_open())
    {
        Logger::error(
            "json",
            "The file could not be opened " + path
        );
        return config;
    }

    nlohmann::json data;
    file >> data;

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