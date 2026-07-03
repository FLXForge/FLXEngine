#include "JsonLoader.h"
#include "../debug/Logger.h"
#include "../tools/ColorParser.h"
#include "../tools/TextTools.h"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <vector>

using Json = nlohmann::ordered_json;

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
        Json& data
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
        catch (const Json::parse_error& error)
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
        Json& base,
        const Json& override
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
        const Json& object,
        Json& resolved
    );

    bool resolveBlockReference(
        const std::filesystem::path& currentFile,
        Json& object,
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

        Json block;

        if (!loadJson(blockPath, block))
        {
            return false;
        }

        Json resolvedBlock;

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
        Json& object
    )
    {
        static const std::vector<std::string> blockKeys = {
            "shape",
            "motion",
            "bounds",
            "collision",
            "behavior",
            "creation",
            "states"
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
        const Json& object,
        Json& resolved
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

        Json base;

        if (!loadJson(basePath, base))
        {
            Logger::error(
                "json",
                "Like target could not be loaded: " + basePath.string()
            );

            return false;
        }

        Json resolvedBase;

        if (!resolveLike(basePath, base, resolvedBase))
        {
            return false;
        }

        Json override =
            object;

        override.erase("like");

        mergeJson(resolvedBase, override);

        resolved = resolvedBase;
        resolved["__sourceFile"] = basePath.generic_string();

        return resolveBlockReferences(currentFile, resolved);
    }

    bool hasShape(const Json& object)
    {
        return object.contains("shape") &&
            object["shape"].is_object();
    }

    void rejectRootProperty(
        const Json& object,
        const std::string& property,
        const std::string& owner,
        const std::string& expectedBlock
    )
    {
        if (!object.contains(property))
        {
            return;
        }

        throw std::runtime_error(
            "Invalid FLX object '" + owner + "': property '" +
            property + "' must be declared inside '" +
            expectedBlock + "'"
        );
    }

    void validateObjectRootProperties(
        const Json& object,
        const std::string& owner
    )
    {
        rejectRootProperty(object, "size", owner, "shape");
        rejectRootProperty(object, "color", owner, "shape");
        rejectRootProperty(object, "layer", owner, "shape");
        rejectRootProperty(object, "speed", owner, "motion");
        rejectRootProperty(object, "angle", owner, "motion");
    }

    Vector2 parseOrigin(const Json& object)
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

    Vector2 parseSize(const Json& object)
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

        return Vector2{ 0.0f, 0.0f };
    }

    void parseShape(
        const Json& object,
        ObjectDefinition& definition
    )
    {
        if (!hasShape(object))
        {
            definition.hasVisual = false;
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

        definition.layer =
            shape.value("layer", definition.layer);

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
        const Json& object,
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

        definition.speed =
            motion.value("speed", definition.speed);

        definition.angle =
            motion.value("angle", definition.angle);

        definition.acceleration =
            motion.value("acceleration", definition.acceleration);

        definition.inertia =
            motion.value("inertia", definition.inertia);

        definition.maxSpeed =
            motion.value("maxSpeed", definition.maxSpeed);
    }

    void parseAttach(
        const Json& object,
        ObjectDefinition& definition
    )
    {
        if (!object.contains("attach") || !object["attach"].is_object())
        {
            return;
        }

        const auto& attach =
            object["attach"];

        const bool position =
            attach.value("position", false);

        definition.attachFollowX =
            attach.contains("x") ?
            attach.value("x", false) :
            position;

        definition.attachFollowY =
            attach.contains("y") ?
            attach.value("y", false) :
            position;

        definition.attachFollowAngle =
            attach.value("angle", false);

        definition.attachOnCreate =
            attach.value("born", false);
    }

    void parseBounds(
        const Json& object,
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
        const Json& object,
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

    void parseCreationPattern(
        const Json& pattern,
        ObjectDefinition& definition
    )
    {
        definition.gridPattern.clear();
        definition.gridRowPattern.clear();
        definition.gridPatternIsRows = false;

        if (!pattern.is_array())
        {
            Logger::error(
                "json",
                "Invalid grid creation pattern in '" + definition.id +
                "': expected array"
            );

            return;
        }

        if (pattern.empty())
        {
            return;
        }

        if (pattern.front().is_array())
        {
            definition.gridPatternIsRows = true;

            for (const auto& row : pattern)
            {
                if (!row.is_array())
                {
                    Logger::warning(
                        "json",
                        "Ignoring invalid grid pattern row in '" +
                        definition.id + "'"
                    );

                    continue;
                }

                std::vector<std::string> rowPattern;

                for (const auto& childId : row)
                {
                    if (!childId.is_string())
                    {
                        Logger::warning(
                            "json",
                            "Ignoring invalid grid pattern value in '" +
                            definition.id + "'"
                        );

                        continue;
                    }

                    rowPattern.push_back(
                        childId.get<std::string>()
                    );
                }

                definition.gridRowPattern.push_back(rowPattern);
            }

            return;
        }

        for (const auto& childId : pattern)
        {
            if (!childId.is_string())
            {
                Logger::warning(
                    "json",
                    "Ignoring invalid grid pattern value in '" +
                    definition.id + "'"
                );

                continue;
            }

            definition.gridPattern.push_back(
                childId.get<std::string>()
            );
        }
    }

    void parseCreation(
        const Json& object,
        ObjectDefinition& definition
    )
    {
        if (!object.contains("creation") || !object["creation"].is_object())
        {
            return;
        }

        const auto& creation =
            object["creation"];

        definition.creationMode =
            TextTools::toLower(
                creation.value("mode", definition.creationMode)
            );

        if (definition.creationMode != "individual" &&
            definition.creationMode != "grid")
        {
            Logger::warning(
                "json",
                "Unsupported creation mode '" + definition.creationMode +
                "' in '" + definition.id + "'"
            );
        }

        if (definition.creationMode != "grid")
        {
            return;
        }

        if (creation.contains("rules") && creation["rules"].is_object())
        {
            const auto& rules =
                creation["rules"];

            definition.gridRules.rows =
                rules.value("rows", definition.gridRules.rows);

            definition.gridRules.columns =
                rules.value("columns", definition.gridRules.columns);

            definition.gridRules.cellWidth =
                rules.value("cellWidth", definition.gridRules.cellWidth);

            definition.gridRules.cellHeight =
                rules.value("cellHeight", definition.gridRules.cellHeight);
        }

        if (creation.contains("pattern"))
        {
            parseCreationPattern(
                creation["pattern"],
                definition
            );
        }
    }

    void parseCollision(
        const Json& object,
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

    Json normalizeSoundValue(
        const Json& value,
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

            Json soundData;

            if (!loadJson(soundPath, soundData))
            {
                return Json{};
            }

            Json resolvedSound;

            if (!resolveLike(soundPath, soundData, resolvedSound))
            {
                return Json{};
            }

            if (resolvedSound.contains("sound"))
            {
                return resolvedSound["sound"];
            }

            return resolvedSound;
        }

        if (value.is_object() && value.contains("like"))
        {
            Json resolvedSound;

            if (!resolveLike(currentFile, value, resolvedSound))
            {
                return Json{};
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
        const Json& object,
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
            const Json data =
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

            if (data.contains("note") && data["note"].is_string())
            {
                sound.note =
                    data["note"].get<std::string>();
            }

            if (data.contains("frequency"))
            {
                sound.frequency =
                    data.value("frequency", sound.frequency);
                sound.hasFrequency = true;
            }

            if (sound.hasFrequency && !sound.note.empty())
            {
                Logger::warning(
                    "json",
                    "Sound '" + it.key() +
                    "' declares both frequency and note; frequency has priority"
                );
            }

            sound.duration =
                data.value("duration", sound.duration);
            sound.volume =
                data.value("volume", sound.volume);

            definition.sounds[it.key()] =
                sound;
        }
    }

    Json normalizeMusicValue(
        const Json& value,
        const std::filesystem::path& currentFile
    )
    {
        if (value.is_string())
        {
            const auto musicPath =
                resolvePath(
                    currentFile,
                    value.get<std::string>()
                );

            Json musicData;

            if (!loadJson(musicPath, musicData))
            {
                return Json{};
            }

            Json resolvedMusic;

            if (!resolveLike(musicPath, musicData, resolvedMusic))
            {
                return Json{};
            }

            if (resolvedMusic.contains("music"))
            {
                return resolvedMusic["music"];
            }

            return resolvedMusic;
        }

        if (value.is_object() && value.contains("like"))
        {
            Json resolvedMusic;

            if (!resolveLike(currentFile, value, resolvedMusic))
            {
                return Json{};
            }

            if (resolvedMusic.contains("music"))
            {
                return resolvedMusic["music"];
            }

            return resolvedMusic;
        }

        return value;
    }

    void parseMusic(
        const Json& object,
        const std::filesystem::path& currentFile,
        ObjectDefinition& definition
    )
    {
        if (!object.contains("music") || !object["music"].is_object())
        {
            return;
        }

        const auto& music =
            object["music"];

        for (auto it = music.begin(); it != music.end(); ++it)
        {
            const Json data =
                normalizeMusicValue(
                    it.value(),
                    currentFile
                );

            if (!data.is_object())
            {
                Logger::warning(
                    "json",
                    "Invalid music '" + it.key() + "': expected object"
                );

                continue;
            }

            MusicDefinition song;
            song.tempo =
                data.value("tempo", song.tempo);
            song.loop =
                data.value("loop", song.loop);

            if (!data.contains("channels") || !data["channels"].is_object())
            {
                Logger::warning(
                    "json",
                    "Invalid music '" + it.key() +
                    "': expected channels object"
                );

                continue;
            }

            const auto& channels =
                data["channels"];

            for (
                auto channelIt = channels.begin();
                channelIt != channels.end();
                ++channelIt
            )
            {
                if (!channelIt.value().is_object())
                {
                    Logger::warning(
                        "json",
                        "Invalid music channel '" + channelIt.key() +
                        "' in '" + it.key() + "': expected object"
                    );

                    continue;
                }

                const Json& channelData =
                    channelIt.value();

                MusicChannelDefinition channel;
                channel.id =
                    channelIt.key();
                channel.wave =
                    TextTools::toLower(
                        channelData.value("wave", channel.wave)
                    );
                channel.volume =
                    channelData.value("volume", channel.volume);

                if (
                    !channelData.contains("notes") ||
                    !channelData["notes"].is_array()
                )
                {
                    Logger::warning(
                        "json",
                        "Invalid music channel '" + channel.id +
                        "' in '" + it.key() + "': expected notes array"
                    );

                    continue;
                }

                for (const auto& note : channelData["notes"])
                {
                    if (!note.is_string())
                    {
                        Logger::warning(
                            "json",
                            "Invalid note in music channel '" + channel.id +
                            "': expected string"
                        );

                        continue;
                    }

                    channel.notes.push_back(
                        note.get<std::string>()
                    );
                }

                song.channels.push_back(channel);
            }

            definition.music[it.key()] =
                song;
        }
    }

    void parseStates(
        const Json& object,
        ObjectDefinition& definition
    )
    {
        if (!object.contains("states") || !object["states"].is_object())
        {
            return;
        }

        const auto& states =
            object["states"];

        definition.initialState =
            states.value("initial", definition.initialState);

        for (auto it = states.begin(); it != states.end(); ++it)
        {
            if (it.key() == "initial")
            {
                continue;
            }

            if (!it.value().is_object())
            {
                Logger::warning(
                    "json",
                    "Invalid state '" + it.key() + "' in '" +
                    definition.id + "': expected object"
                );

                continue;
            }

            std::vector<std::string> nextStates;

            if (it.value().contains("next"))
            {
                if (!it.value()["next"].is_array())
                {
                    Logger::warning(
                        "json",
                        "Invalid next states in '" + it.key() +
                        "': expected array"
                    );
                }
                else
                {
                    for (const auto& nextState : it.value()["next"])
                    {
                        if (!nextState.is_string())
                        {
                            Logger::warning(
                                "json",
                                "Ignoring invalid next state in '" +
                                it.key() + "'"
                            );

                            continue;
                        }

                        nextStates.push_back(
                            nextState.get<std::string>()
                        );
                    }
                }
            }

            definition.stateTransitions[it.key()] =
                nextStates;
        }

        if (
            !definition.initialState.empty() &&
            !definition.stateTransitions.contains(definition.initialState)
            )
        {
            Logger::warning(
                "json",
                "Initial state '" + definition.initialState +
                "' is not declared in '" + definition.id + "'"
            );
        }
    }

    ObjectDefinition parseDefinition(
        const Json& object,
        const std::filesystem::path& currentFile,
        const std::string& id
    );

    Json normalizeChildValue(const Json& value)
    {
        if (value.is_string())
        {
            return Json{
                { "like", value.get<std::string>() }
            };
        }

        return value;
    }

    void parseChildren(
        const Json& object,
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
            Json childData =
                normalizeChildValue(it.value());

            if (!childData.is_object())
            {
                Logger::warning(
                    "json",
                    "Invalid child '" + it.key() + "': expected object"
                );

                continue;
            }

            Json resolvedChild;

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
        const Json& object,
        const std::filesystem::path& currentFile,
        const std::string& id
    )
    {
        validateObjectRootProperties(object, id);

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

        parseShape(object, definition);
        parseMotion(object, definition);
        parseAttach(object, definition);
        parseBounds(object, definition);
        parseBehavior(object, definition);
        parseCreation(object, definition);
        parseCollision(object, definition);
        parseSounds(object, sourceFile, definition);
        parseMusic(object, sourceFile, definition);
        parseStates(object, definition);
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

    Json data;

    if (!loadJson(objectPath, data))
    {
        return ObjectDefinition{};
    }

    Json resolved;

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
