#include "CompiledProjectBinary.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace
{
    constexpr uint32_t Magic =
        0x43584C46;

    constexpr uint32_t FormatVersion =
        1;

    constexpr uint32_t MaxStringSize =
        32u * 1024u * 1024u;

    class BinaryWriter
    {
    public:
        explicit BinaryWriter(std::ostream& output)
            : output(output)
        {
        }

        template <typename T>
        void value(const T& data)
        {
            static_assert(std::is_trivially_copyable_v<T>);

            output.write(
                reinterpret_cast<const char*>(&data),
                sizeof(T)
            );
        }

        void string(const std::string& text)
        {
            const uint32_t size =
                static_cast<uint32_t>(text.size());

            value(size);

            if (size > 0)
            {
                output.write(
                    text.data(),
                    size
                );
            }
        }

    private:
        std::ostream& output;
    };

    class BinaryReader
    {
    public:
        explicit BinaryReader(std::istream& input)
            : input(input)
        {
        }

        template <typename T>
        T value()
        {
            static_assert(std::is_trivially_copyable_v<T>);

            T data{};

            input.read(
                reinterpret_cast<char*>(&data),
                sizeof(T)
            );

            if (!input)
            {
                throw std::runtime_error("Compiled project file is truncated");
            }

            return data;
        }

        std::string string()
        {
            const uint32_t size =
                value<uint32_t>();

            if (size > MaxStringSize)
            {
                throw std::runtime_error("Compiled project string is too large");
            }

            std::string text(size, '\0');

            if (size > 0)
            {
                input.read(
                    text.data(),
                    size
                );

                if (!input)
                {
                    throw std::runtime_error("Compiled project file is truncated");
                }
            }

            return text;
        }

    private:
        std::istream& input;
    };

    template <typename T>
    std::vector<std::string> sortedKeys(const std::unordered_map<std::string, T>& map)
    {
        std::vector<std::string> keys;
        keys.reserve(map.size());

        for (const auto& pair : map)
        {
            keys.push_back(pair.first);
        }

        std::sort(
            keys.begin(),
            keys.end()
        );

        return keys;
    }

    template <typename T>
    std::vector<std::string> sortedKeys(const std::map<std::string, T>& map)
    {
        std::vector<std::string> keys;
        keys.reserve(map.size());

        for (const auto& pair : map)
        {
            keys.push_back(pair.first);
        }

        return keys;
    }

    void writeColor(BinaryWriter& writer, Color color)
    {
        writer.value(color.r);
        writer.value(color.g);
        writer.value(color.b);
        writer.value(color.a);
    }

    Color readColor(BinaryReader& reader)
    {
        Color color;
        color.r = reader.value<unsigned char>();
        color.g = reader.value<unsigned char>();
        color.b = reader.value<unsigned char>();
        color.a = reader.value<unsigned char>();
        return color;
    }

    void writeVector2(BinaryWriter& writer, Vector2 value)
    {
        writer.value(value.x);
        writer.value(value.y);
    }

    Vector2 readVector2(BinaryReader& reader)
    {
        return Vector2{
            reader.value<float>(),
            reader.value<float>()
        };
    }

    void writeStringVector(BinaryWriter& writer, const std::vector<std::string>& values)
    {
        writer.value(static_cast<uint32_t>(values.size()));

        for (const std::string& value : values)
        {
            writer.string(value);
        }
    }

    std::vector<std::string> readStringVector(BinaryReader& reader)
    {
        const uint32_t size =
            reader.value<uint32_t>();

        std::vector<std::string> values;
        values.reserve(size);

        for (uint32_t i = 0; i < size; ++i)
        {
            values.push_back(reader.string());
        }

        return values;
    }

    void writeVideo(BinaryWriter& writer, const VideoChipDefinition& video)
    {
        writer.value(video.screenWidth);
        writer.value(video.screenHeight);
        writer.string(video.clearColor);
        writer.value(video.hasColorPalette);
        writer.value(static_cast<uint32_t>(video.colorPalette.size()));

        for (Color color : video.colorPalette)
        {
            writeColor(writer, color);
        }

        writer.value(video.colorLevelsRed);
        writer.value(video.colorLevelsGreen);
        writer.value(video.colorLevelsBlue);
        writer.value(video.hasColorLevels);
        writer.string(video.colorToneBase);
        writer.value(video.colorToneLevels);
        writer.value(video.hasColorTone);
        writer.value(video.colorAlpha);
        writer.value(video.planesEnabled);
        writer.value(video.objectsSprites);
        writer.value(video.outputScale);
        writer.value(video.smoothing);
    }

    VideoChipDefinition readVideo(BinaryReader& reader)
    {
        VideoChipDefinition video;
        video.screenWidth = reader.value<int>();
        video.screenHeight = reader.value<int>();
        video.clearColor = reader.string();
        video.hasColorPalette = reader.value<bool>();

        const uint32_t paletteSize =
            reader.value<uint32_t>();

        video.colorPalette.reserve(paletteSize);

        for (uint32_t i = 0; i < paletteSize; ++i)
        {
            video.colorPalette.push_back(readColor(reader));
        }

        video.colorLevelsRed = reader.value<int>();
        video.colorLevelsGreen = reader.value<int>();
        video.colorLevelsBlue = reader.value<int>();
        video.hasColorLevels = reader.value<bool>();
        video.colorToneBase = reader.string();
        video.colorToneLevels = reader.value<int>();
        video.hasColorTone = reader.value<bool>();
        video.colorAlpha = reader.value<bool>();
        video.planesEnabled = reader.value<bool>();
        video.objectsSprites = reader.value<bool>();
        video.outputScale = reader.value<int>();
        video.smoothing = reader.value<bool>();
        return video;
    }

    void writeAudio(BinaryWriter& writer, const AudioChipDefinition& audio)
    {
        writer.value(audio.voicesMusic);
        writer.value(audio.voicesSound);
        writer.string(audio.voicesMode);
        writer.string(audio.voicesOverflow);
        writer.string(audio.synthesisModel);
        writer.string(audio.synthesisTexture);
        writer.string(audio.synthesisMovement);
        writer.string(audio.synthesisNoise);
        writer.string(audio.fidelityResolution);
        writer.string(audio.fidelityDynamics);
        writer.string(audio.fidelitySpace);
        writer.value(audio.resourcesGenerated);
        writer.value(audio.resourcesSamples);
        writer.value(audio.resourcesStreams);
        writer.string(audio.fileAudioMode);
    }

    AudioChipDefinition readAudio(BinaryReader& reader)
    {
        AudioChipDefinition audio;
        audio.voicesMusic = reader.value<int>();
        audio.voicesSound = reader.value<int>();
        audio.voicesMode = reader.string();
        audio.voicesOverflow = reader.string();
        audio.synthesisModel = reader.string();
        audio.synthesisTexture = reader.string();
        audio.synthesisMovement = reader.string();
        audio.synthesisNoise = reader.string();
        audio.fidelityResolution = reader.string();
        audio.fidelityDynamics = reader.string();
        audio.fidelitySpace = reader.string();
        audio.resourcesGenerated = reader.value<bool>();
        audio.resourcesSamples = reader.value<bool>();
        audio.resourcesStreams = reader.value<bool>();
        audio.fileAudioMode = reader.string();
        return audio;
    }

    void writeInput(BinaryWriter& writer, const InputChipDefinition& input)
    {
        writer.value(input.players);
        writer.string(input.direction);
        writer.value(input.playerButtons);
        writer.value(input.systemButtons);
        writer.value(input.pointer);
        writer.value(input.text);
    }

    InputChipDefinition readInput(BinaryReader& reader)
    {
        InputChipDefinition input;
        input.players = reader.value<int>();
        input.direction = reader.string();
        input.playerButtons = reader.value<int>();
        input.systemButtons = reader.value<int>();
        input.pointer = reader.value<bool>();
        input.text = reader.value<bool>();
        return input;
    }

    void writeMachine(BinaryWriter& writer, const MachineDefinition& machine)
    {
        writeVideo(writer, machine.video);
        writeAudio(writer, machine.audio);
        writeInput(writer, machine.input);
    }

    MachineDefinition readMachine(BinaryReader& reader)
    {
        MachineDefinition machine;
        machine.video = readVideo(reader);
        machine.audio = readAudio(reader);
        machine.input = readInput(reader);
        return machine;
    }

    void writeSource(BinaryWriter& writer, const AudioSourceDefinition& source)
    {
        writer.string(source.type);
        writer.string(source.wave);
        writer.value(source.duty);
    }

    AudioSourceDefinition readSource(BinaryReader& reader)
    {
        AudioSourceDefinition source;
        source.type = reader.string();
        source.wave = reader.string();
        source.duty = reader.value<float>();
        return source;
    }

    void writeTone(BinaryWriter& writer, const AudioToneDefinition& tone)
    {
        writer.value(tone.material.brightness);
        writer.value(tone.material.roughness);
        writer.value(tone.material.noise);
        writer.value(tone.material.resonance);
        writer.value(tone.material.metal);
        writer.value(tone.envelope.attack);
        writer.value(tone.envelope.decay);
        writer.value(tone.envelope.sustain);
        writer.value(tone.envelope.release);
        writer.string(tone.space.mode);
        writer.value(tone.space.width);
        writer.value(tone.space.echo);
    }

    AudioToneDefinition readTone(BinaryReader& reader)
    {
        AudioToneDefinition tone;
        tone.material.brightness = reader.value<float>();
        tone.material.roughness = reader.value<float>();
        tone.material.noise = reader.value<float>();
        tone.material.resonance = reader.value<float>();
        tone.material.metal = reader.value<float>();
        tone.envelope.attack = reader.value<float>();
        tone.envelope.decay = reader.value<float>();
        tone.envelope.sustain = reader.value<float>();
        tone.envelope.release = reader.value<float>();
        tone.space.mode = reader.string();
        tone.space.width = reader.value<float>();
        tone.space.echo = reader.value<float>();
        return tone;
    }

    void writeSound(BinaryWriter& writer, const SoundDefinition& sound)
    {
        writeSource(writer, sound.kind.source);
        writer.value(sound.kind.noteFrequency);
        writer.value(sound.kind.slide);
        writer.string(sound.kind.movement.type);
        writer.value(sound.kind.movement.amount);
        writeTone(writer, sound.tone);
        writer.value(sound.duration);
        writer.value(sound.volume);
    }

    SoundDefinition readSound(BinaryReader& reader)
    {
        SoundDefinition sound;
        sound.kind.source = readSource(reader);
        sound.kind.noteFrequency = reader.value<float>();
        sound.kind.slide = reader.value<float>();
        sound.kind.movement.type = reader.string();
        sound.kind.movement.amount = reader.value<float>();
        sound.tone = readTone(reader);
        sound.duration = reader.value<float>();
        sound.volume = reader.value<float>();
        return sound;
    }

    void writeMusic(BinaryWriter& writer, const MusicDefinition& music)
    {
        writer.value(music.tempo);
        writer.value(music.loop);
        writer.value(static_cast<uint32_t>(music.channels.size()));

        for (const MusicChannelDefinition& channel : music.channels)
        {
            writer.string(channel.id);
            writeSource(writer, channel.instrument.source);
            writeTone(writer, channel.instrument.tone);
            writer.value(channel.instrument.play.legato);
            writer.value(channel.instrument.play.glide);
            writer.value(channel.instrument.play.vibrato);
            writer.string(channel.instrument.range.min);
            writer.string(channel.instrument.range.max);
            writer.value(channel.volume);
            writer.string(channel.length);
            writeStringVector(writer, channel.notes);
        }
    }

    MusicDefinition readMusic(BinaryReader& reader)
    {
        MusicDefinition music;
        music.tempo = reader.value<float>();
        music.loop = reader.value<bool>();

        const uint32_t channelCount =
            reader.value<uint32_t>();

        music.channels.reserve(channelCount);

        for (uint32_t i = 0; i < channelCount; ++i)
        {
            MusicChannelDefinition channel;
            channel.id = reader.string();
            channel.instrument.source = readSource(reader);
            channel.instrument.tone = readTone(reader);
            channel.instrument.play.legato = reader.value<bool>();
            channel.instrument.play.glide = reader.value<float>();
            channel.instrument.play.vibrato = reader.value<float>();
            channel.instrument.range.min = reader.string();
            channel.instrument.range.max = reader.string();
            channel.volume = reader.value<float>();
            channel.length = reader.string();
            channel.notes = readStringVector(reader);
            music.channels.push_back(channel);
        }

        return music;
    }

    void writeObject(BinaryWriter& writer, const ObjectDefinition& object)
    {
        writer.string(object.id);
        writer.string(object.sourcePath);
        writer.string(object.spawnMode);
        writeVector2(writer, object.offset);
        writer.value(object.hasOffset);
        writer.value(object.attachFollowX);
        writer.value(object.attachFollowY);
        writer.value(object.attachFollowAngle);
        writer.value(object.attachOnCreate);
        writer.value(object.visible);
        writer.value(object.hasVisual);
        writer.value(object.layer);
        writeVector2(writer, object.origin);
        writer.value(object.hasOrigin);
        writeVector2(writer, object.size);
        writeColor(writer, object.color);
        writer.string(object.shapeMode);
        writer.string(object.shapeType);
        writer.string(object.textContent);
        writer.value(object.radius);
        writer.value(static_cast<uint32_t>(object.points.size()));

        for (Vector2 point : object.points)
        {
            writeVector2(writer, point);
        }

        writer.value(object.speed);
        writer.value(object.hasSpeed);
        writer.value(object.angle);
        writer.value(object.hasAngle);
        writer.value(object.inheritParentAngle);
        writer.value(object.rotationSpeed);
        writer.value(object.acceleration);
        writer.value(object.maxSpeed);
        writer.value(object.inertia);
        writer.string(object.boundsMode);
        writer.value(object.boundsOverflow);
        writer.string(object.group);
        writer.string(object.role);
        writer.string(object.collisionType);
        writer.value(object.collisionActive);
        writer.value(object.collisionRadius);
        writeStringVector(writer, object.collisionWith);
        writeStringVector(writer, object.scripts);
        writeStringVector(writer, object.resolvedScriptPaths);

        const auto musicKeys =
            sortedKeys(object.music);

        writer.value(static_cast<uint32_t>(musicKeys.size()));

        for (const std::string& key : musicKeys)
        {
            writer.string(key);
            writeMusic(writer, object.music.at(key));
        }

        const auto soundKeys =
            sortedKeys(object.sounds);

        writer.value(static_cast<uint32_t>(soundKeys.size()));

        for (const std::string& key : soundKeys)
        {
            writer.string(key);
            writeSound(writer, object.sounds.at(key));
        }

        const auto childKeys =
            sortedKeys(object.children);

        writer.value(static_cast<uint32_t>(childKeys.size()));

        for (const std::string& key : childKeys)
        {
            writer.string(key);
            writeObject(writer, object.children.at(key));
        }

        const auto childResourceKeys =
            sortedKeys(object.childResources);

        writer.value(static_cast<uint32_t>(childResourceKeys.size()));

        for (const std::string& key : childResourceKeys)
        {
            writer.string(key);
            writer.string(object.childResources.at(key));
        }

        writer.string(object.initialState);

        const auto transitionKeys =
            sortedKeys(object.stateTransitions);

        writer.value(static_cast<uint32_t>(transitionKeys.size()));

        for (const std::string& key : transitionKeys)
        {
            writer.string(key);
            writeStringVector(writer, object.stateTransitions.at(key));
        }

        writer.string(object.creationMode);
        writer.value(object.gridRules.rows);
        writer.value(object.gridRules.columns);
        writer.value(object.gridRules.cellWidth);
        writer.value(object.gridRules.cellHeight);
        writer.value(object.gridPatternIsRows);
        writeStringVector(writer, object.gridPattern);
        writer.value(static_cast<uint32_t>(object.gridRowPattern.size()));

        for (const auto& row : object.gridRowPattern)
        {
            writeStringVector(writer, row);
        }
    }

    ObjectDefinition readObject(BinaryReader& reader)
    {
        ObjectDefinition object;
        object.id = reader.string();
        object.sourcePath = reader.string();
        object.spawnMode = reader.string();
        object.offset = readVector2(reader);
        object.hasOffset = reader.value<bool>();
        object.attachFollowX = reader.value<bool>();
        object.attachFollowY = reader.value<bool>();
        object.attachFollowAngle = reader.value<bool>();
        object.attachOnCreate = reader.value<bool>();
        object.visible = reader.value<bool>();
        object.hasVisual = reader.value<bool>();
        object.layer = reader.value<int>();
        object.origin = readVector2(reader);
        object.hasOrigin = reader.value<bool>();
        object.size = readVector2(reader);
        object.color = readColor(reader);
        object.shapeMode = reader.string();
        object.shapeType = reader.string();
        object.textContent = reader.string();
        object.radius = reader.value<float>();

        const uint32_t pointCount =
            reader.value<uint32_t>();

        object.points.reserve(pointCount);

        for (uint32_t i = 0; i < pointCount; ++i)
        {
            object.points.push_back(readVector2(reader));
        }

        object.speed = reader.value<float>();
        object.hasSpeed = reader.value<bool>();
        object.angle = reader.value<float>();
        object.hasAngle = reader.value<bool>();
        object.inheritParentAngle = reader.value<bool>();
        object.rotationSpeed = reader.value<float>();
        object.acceleration = reader.value<float>();
        object.maxSpeed = reader.value<float>();
        object.inertia = reader.value<float>();
        object.boundsMode = reader.string();
        object.boundsOverflow = reader.value<bool>();
        object.group = reader.string();
        object.role = reader.string();
        object.collisionType = reader.string();
        object.collisionActive = reader.value<bool>();
        object.collisionRadius = reader.value<float>();
        object.collisionWith = readStringVector(reader);
        object.scripts = readStringVector(reader);
        object.resolvedScriptPaths = readStringVector(reader);

        const uint32_t musicCount =
            reader.value<uint32_t>();

        for (uint32_t i = 0; i < musicCount; ++i)
        {
            const std::string key =
                reader.string();

            object.music[key] =
                readMusic(reader);
        }

        const uint32_t soundCount =
            reader.value<uint32_t>();

        for (uint32_t i = 0; i < soundCount; ++i)
        {
            const std::string key =
                reader.string();

            object.sounds[key] =
                readSound(reader);
        }

        const uint32_t childCount =
            reader.value<uint32_t>();

        for (uint32_t i = 0; i < childCount; ++i)
        {
            const std::string key =
                reader.string();

            object.children[key] =
                readObject(reader);
        }

        const uint32_t childResourceCount =
            reader.value<uint32_t>();

        for (uint32_t i = 0; i < childResourceCount; ++i)
        {
            const std::string key =
                reader.string();

            object.childResources[key] =
                reader.string();
        }

        object.initialState = reader.string();

        const uint32_t transitionCount =
            reader.value<uint32_t>();

        for (uint32_t i = 0; i < transitionCount; ++i)
        {
            const std::string key =
                reader.string();

            object.stateTransitions[key] =
                readStringVector(reader);
        }

        object.creationMode = reader.string();
        object.gridRules.rows = reader.value<int>();
        object.gridRules.columns = reader.value<int>();
        object.gridRules.cellWidth = reader.value<float>();
        object.gridRules.cellHeight = reader.value<float>();
        object.gridPatternIsRows = reader.value<bool>();
        object.gridPattern = readStringVector(reader);

        const uint32_t rowCount =
            reader.value<uint32_t>();

        object.gridRowPattern.reserve(rowCount);

        for (uint32_t i = 0; i < rowCount; ++i)
        {
            object.gridRowPattern.push_back(readStringVector(reader));
        }

        return object;
    }

    void writeContext(BinaryWriter& writer, const FlxContext& context)
    {
        writer.string(context.name);
        writer.string(context.version);
        writer.string(context.engineVersion);
        writer.string(context.notes);
        writer.string(context.root);
        writer.string(context.screenTitle);
        writer.value(context.screenWidth);
        writer.value(context.screenHeight);
        writer.value(context.screenScale);
        writer.value(context.screenWidthOverride);
        writer.value(context.screenHeightOverride);
        writer.value(context.screenScaleOverride);
        writer.value(context.debugCollisions);
        writer.value(context.debugLogs);
        writer.value(context.debugConsole);
        writer.string(context.windowMode);
        writer.string(context.inputMappingSourceName);
        writer.string(context.inputMappingContent);
        writeMachine(writer, context.machine);
    }

    FlxContext readContext(BinaryReader& reader)
    {
        FlxContext context;
        context.name = reader.string();
        context.version = reader.string();
        context.engineVersion = reader.string();
        context.notes = reader.string();
        context.root = reader.string();
        context.screenTitle = reader.string();
        context.screenWidth = reader.value<int>();
        context.screenHeight = reader.value<int>();
        context.screenScale = reader.value<int>();
        context.screenWidthOverride = reader.value<bool>();
        context.screenHeightOverride = reader.value<bool>();
        context.screenScaleOverride = reader.value<bool>();
        context.debugCollisions = reader.value<bool>();
        context.debugLogs = reader.value<bool>();
        context.debugConsole = reader.value<bool>();
        context.windowMode = reader.string();
        context.inputMappingSourceName = reader.string();
        context.inputMappingContent = reader.string();
        context.machine = readMachine(reader);
        return context;
    }
}

bool CompiledProjectWriter::write(
    const std::string& path,
    const CompiledProject& project,
    Diagnostics& diagnostics
)
{
    const std::filesystem::path outputPath(path);

    if (!outputPath.parent_path().empty())
    {
        std::filesystem::create_directories(outputPath.parent_path());
    }

    std::ofstream file(path, std::ios::binary);

    if (!file.is_open())
    {
        diagnostics.error(
            "Compiled project file could not be created",
            path
        );

        return false;
    }

    BinaryWriter writer(file);

    writer.value(Magic);
    writer.value(FormatVersion);
    writer.string("0.2.0");
    writeContext(writer, project.context);
    writer.string(project.rootId);
    writer.string(project.rootPath.empty() ? "" : std::filesystem::path(project.rootPath).filename().generic_string());
    writeObject(writer, project.rootDefinition);

    const auto objectKeys =
        sortedKeys(project.resources.allObjects());

    writer.value(static_cast<uint32_t>(objectKeys.size()));

    for (const std::string& key : objectKeys)
    {
        writer.string(key);
        writeObject(writer, project.resources.allObjects().at(key));
    }

    const auto scriptKeys =
        sortedKeys(project.resources.allScripts());

    writer.value(static_cast<uint32_t>(scriptKeys.size()));

    for (const std::string& key : scriptKeys)
    {
        const ScriptResource& script =
            project.resources.allScripts().at(key);

        writer.string(key);
        writer.string(script.id);
        writer.string(script.sourceName);
        writer.string(script.code);
    }

    if (!file)
    {
        diagnostics.error(
            "Compiled project file could not be written",
            path
        );

        return false;
    }

    return true;
}

CompiledProjectBinaryResult CompiledProjectReader::read(
    const std::string& path
)
{
    CompiledProjectBinaryResult result;

    std::ifstream file(path, std::ios::binary);

    if (!file.is_open())
    {
        result.diagnostics.error(
            "Compiled project file could not be opened",
            path
        );

        return result;
    }

    try
    {
        BinaryReader reader(file);

        if (reader.value<uint32_t>() != Magic)
        {
            result.diagnostics.error(
                "Invalid compiled project magic",
                path
            );

            return result;
        }

        const uint32_t version =
            reader.value<uint32_t>();

        if (version != FormatVersion)
        {
            result.diagnostics.error(
                "Unsupported compiled project version",
                path,
                std::to_string(version)
            );

            return result;
        }

        const std::string engineVersion =
            reader.string();

        result.project.context =
            readContext(reader);

        result.project.context.engineVersion =
            engineVersion;

        result.project.rootId =
            reader.string();

        result.project.rootPath =
            reader.string();

        result.project.rootDefinition =
            readObject(reader);

        const uint32_t objectCount =
            reader.value<uint32_t>();

        for (uint32_t i = 0; i < objectCount; ++i)
        {
            const ResourceId id =
                reader.string();

            result.project.resources.addObject(
                id,
                readObject(reader)
            );
        }

        const uint32_t scriptCount =
            reader.value<uint32_t>();

        for (uint32_t i = 0; i < scriptCount; ++i)
        {
            const ResourceId key =
                reader.string();

            ScriptResource script;
            script.id = reader.string();
            script.sourceName = reader.string();
            script.code = reader.string();

            result.project.resources.addScript(
                key,
                script
            );
        }
    }
    catch (const std::exception& exception)
    {
        result.diagnostics.error(
            exception.what(),
            path
        );

        return result;
    }

    if (result.project.rootId.empty() ||
        result.project.resources.findObject(result.project.rootId) == nullptr)
    {
        result.diagnostics.error(
            "Compiled project root resource is missing",
            path
        );

        return result;
    }

    result.success =
        !result.diagnostics.hasErrors();

    return result;
}
