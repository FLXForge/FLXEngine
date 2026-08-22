#include "CompiledProjectCodec.h"
#include "BinaryLimits.h"
#include "FlxVersion.h"

#include <algorithm>
#include <map>
#include <unordered_map>
#include <vector>

namespace flx::binary
{
    namespace
    {
        template <typename T>
        std::vector<std::string> sortedKeys(const std::unordered_map<std::string, T>& map)
        {
            std::vector<std::string> keys;
            keys.reserve(map.size());

            for (const auto& pair : map)
            {
                keys.push_back(pair.first);
            }

            std::sort(keys.begin(), keys.end());
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
            writer.writeU8(color.r);
            writer.writeU8(color.g);
            writer.writeU8(color.b);
            writer.writeU8(color.a);
        }

        Color readColor(BinaryReader& reader)
        {
            Color color;
            color.r = reader.readU8("color.r");
            color.g = reader.readU8("color.g");
            color.b = reader.readU8("color.b");
            color.a = reader.readU8("color.a");
            return color;
        }

        void writeVector2(BinaryWriter& writer, Vector2 value)
        {
            writer.writeF32(value.x);
            writer.writeF32(value.y);
        }

        Vector2 readVector2(BinaryReader& reader)
        {
            return Vector2{
                reader.readF32("vector.x"),
                reader.readF32("vector.y")
            };
        }

        void writeStringVector(
            BinaryWriter& writer,
            const std::vector<std::string>& values
        )
        {
            writer.writeCount(
                values.size(),
                MaxCollectionCount,
                "stringVector"
            );

            for (const std::string& value : values)
            {
                writer.writeString(value);
            }
        }

        std::vector<std::string> readStringVector(BinaryReader& reader)
        {
            const uint32_t size =
                reader.readCount(
                    MaxCollectionCount,
                    "stringVector"
                );

            std::vector<std::string> values;
            values.reserve(size);

            for (uint32_t i = 0; i < size; ++i)
            {
                values.push_back(reader.readString("stringVector.item"));
            }

            return values;
        }

        void writeVideo(BinaryWriter& writer, const VideoChipDefinition& video)
        {
            writer.writeI32(video.screenWidth);
            writer.writeI32(video.screenHeight);
            writer.writeString(video.clearColor);
            writer.writeBool(video.hasColorPalette);
            writer.writeCount(
                video.colorPalette.size(),
                MaxCollectionCount,
                "video.colorPalette"
            );

            for (Color color : video.colorPalette)
            {
                writeColor(writer, color);
            }

            writer.writeI32(video.colorLevelsRed);
            writer.writeI32(video.colorLevelsGreen);
            writer.writeI32(video.colorLevelsBlue);
            writer.writeBool(video.hasColorLevels);
            writer.writeString(video.colorToneBase);
            writer.writeI32(video.colorToneLevels);
            writer.writeBool(video.hasColorTone);
            writer.writeBool(video.colorAlpha);
            writer.writeBool(video.planesEnabled);
            writer.writeBool(video.objectsSprites);
            writer.writeI32(video.outputScale);
            writer.writeBool(video.smoothing);
        }

        VideoChipDefinition readVideo(BinaryReader& reader)
        {
            VideoChipDefinition video;
            video.screenWidth = reader.readI32("video.screen.width");
            video.screenHeight = reader.readI32("video.screen.height");
            video.clearColor = reader.readString("video.screen.color");
            video.hasColorPalette = reader.readBool("video.color.palette");

            const uint32_t paletteSize =
                reader.readCount(
                    MaxCollectionCount,
                    "video.color.palette"
                );

            video.colorPalette.reserve(paletteSize);

            for (uint32_t i = 0; i < paletteSize; ++i)
            {
                video.colorPalette.push_back(readColor(reader));
            }

            video.colorLevelsRed = reader.readI32("video.color.levels.red");
            video.colorLevelsGreen = reader.readI32("video.color.levels.green");
            video.colorLevelsBlue = reader.readI32("video.color.levels.blue");
            video.hasColorLevels = reader.readBool("video.color.levels");
            video.colorToneBase = reader.readString("video.color.tone.base");
            video.colorToneLevels = reader.readI32("video.color.tone.levels");
            video.hasColorTone = reader.readBool("video.color.tone");
            video.colorAlpha = reader.readBool("video.color.alpha");
            video.planesEnabled = reader.readBool("video.planes.enabled");
            video.objectsSprites = reader.readBool("video.objects.sprites");
            video.outputScale = reader.readI32("video.output.scale");
            video.smoothing = reader.readBool("video.output.smoothing");
            return video;
        }

        void writeAudio(BinaryWriter& writer, const AudioChipDefinition& audio)
        {
            writer.writeI32(audio.voicesMusic);
            writer.writeI32(audio.voicesSound);
            writer.writeString(audio.voicesMode);
            writer.writeString(audio.voicesOverflow);
            writer.writeString(audio.synthesisModel);
            writer.writeString(audio.synthesisTexture);
            writer.writeString(audio.synthesisMovement);
            writer.writeString(audio.synthesisNoise);
            writer.writeString(audio.fidelityResolution);
            writer.writeString(audio.fidelityDynamics);
            writer.writeString(audio.fidelitySpace);
            writer.writeBool(audio.resourcesGenerated);
            writer.writeBool(audio.resourcesSamples);
            writer.writeBool(audio.resourcesStreams);
            writer.writeString(audio.fileAudioMode);
        }

        AudioChipDefinition readAudio(BinaryReader& reader)
        {
            AudioChipDefinition audio;
            audio.voicesMusic = reader.readI32("audio.voices.music");
            audio.voicesSound = reader.readI32("audio.voices.sound");
            audio.voicesMode = reader.readString("audio.voices.mode");
            audio.voicesOverflow = reader.readString("audio.voices.overflow");
            audio.synthesisModel = reader.readString("audio.synthesis.model");
            audio.synthesisTexture = reader.readString("audio.synthesis.texture");
            audio.synthesisMovement = reader.readString("audio.synthesis.movement");
            audio.synthesisNoise = reader.readString("audio.synthesis.noise");
            audio.fidelityResolution = reader.readString("audio.fidelity.resolution");
            audio.fidelityDynamics = reader.readString("audio.fidelity.dynamics");
            audio.fidelitySpace = reader.readString("audio.fidelity.space");
            audio.resourcesGenerated = reader.readBool("audio.resources.generated");
            audio.resourcesSamples = reader.readBool("audio.resources.samples");
            audio.resourcesStreams = reader.readBool("audio.resources.streams");
            audio.fileAudioMode = reader.readString("audio.fileAudio");
            return audio;
        }

        void writeInput(BinaryWriter& writer, const InputChipDefinition& input)
        {
            writer.writeI32(input.systemButtons);
            writer.writeI32(input.players);
            writer.writeCount(
                input.directions.size(),
                MaxCollectionCount,
                "input.players.controls.directions"
            );

            for (const InputDirectionDefinition& direction : input.directions)
            {
                writer.writeString(direction.type);
                writer.writeString(direction.simultaneous);
                writer.writeF32(direction.buffer);
            }

            writer.writeI32(input.playerButtons);
        }

        InputChipDefinition readInput(BinaryReader& reader)
        {
            InputChipDefinition input;
            input.systemButtons = reader.readI32("input.system.buttons");
            input.players = reader.readI32("input.players.count");

            const uint32_t directionCount =
                reader.readCount(
                    MaxCollectionCount,
                    "input.players.controls.directions"
                );

            input.directions.clear();
            input.directions.reserve(directionCount);

            for (uint32_t i = 0; i < directionCount; ++i)
            {
                InputDirectionDefinition direction;
                direction.type = reader.readString("input.direction.type");
                direction.simultaneous = reader.readString("input.direction.simultaneous");
                direction.buffer = reader.readF32("input.direction.buffer");
                input.directions.push_back(direction);
            }

            input.playerButtons = reader.readI32("input.players.controls.buttons");
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
            writer.writeString(source.type);
            writer.writeString(source.wave);
            writer.writeF32(source.duty);
        }

        AudioSourceDefinition readSource(BinaryReader& reader)
        {
            AudioSourceDefinition source;
            source.type = reader.readString("audio.source.type");
            source.wave = reader.readString("audio.source.wave");
            source.duty = reader.readF32("audio.source.duty");
            return source;
        }

        void writeTone(BinaryWriter& writer, const AudioToneDefinition& tone)
        {
            writer.writeF32(tone.material.brightness);
            writer.writeF32(tone.material.roughness);
            writer.writeF32(tone.material.noise);
            writer.writeF32(tone.material.resonance);
            writer.writeF32(tone.material.metal);
            writer.writeF32(tone.envelope.attack);
            writer.writeF32(tone.envelope.decay);
            writer.writeF32(tone.envelope.sustain);
            writer.writeF32(tone.envelope.release);
            writer.writeString(tone.space.mode);
            writer.writeF32(tone.space.width);
            writer.writeF32(tone.space.echo);
        }

        AudioToneDefinition readTone(BinaryReader& reader)
        {
            AudioToneDefinition tone;
            tone.material.brightness = reader.readF32("audio.tone.material.brightness");
            tone.material.roughness = reader.readF32("audio.tone.material.roughness");
            tone.material.noise = reader.readF32("audio.tone.material.noise");
            tone.material.resonance = reader.readF32("audio.tone.material.resonance");
            tone.material.metal = reader.readF32("audio.tone.material.metal");
            tone.envelope.attack = reader.readF32("audio.tone.envelope.attack");
            tone.envelope.decay = reader.readF32("audio.tone.envelope.decay");
            tone.envelope.sustain = reader.readF32("audio.tone.envelope.sustain");
            tone.envelope.release = reader.readF32("audio.tone.envelope.release");
            tone.space.mode = reader.readString("audio.tone.space.mode");
            tone.space.width = reader.readF32("audio.tone.space.width");
            tone.space.echo = reader.readF32("audio.tone.space.echo");
            return tone;
        }

        void writeSound(BinaryWriter& writer, const SoundDefinition& sound)
        {
            writeSource(writer, sound.kind.source);
            writer.writeF32(sound.kind.noteFrequency);
            writer.writeF32(sound.kind.slide);
            writer.writeString(sound.kind.movement.type);
            writer.writeF32(sound.kind.movement.amount);
            writeTone(writer, sound.tone);
            writer.writeF32(sound.duration);
            writer.writeF32(sound.volume);
        }

        SoundDefinition readSound(BinaryReader& reader)
        {
            SoundDefinition sound;
            sound.kind.source = readSource(reader);
            sound.kind.noteFrequency = reader.readF32("sound.noteFrequency");
            sound.kind.slide = reader.readF32("sound.slide");
            sound.kind.movement.type = reader.readString("sound.movement.type");
            sound.kind.movement.amount = reader.readF32("sound.movement.amount");
            sound.tone = readTone(reader);
            sound.duration = reader.readF32("sound.duration");
            sound.volume = reader.readF32("sound.volume");
            return sound;
        }

        void writeMusic(BinaryWriter& writer, const MusicDefinition& music)
        {
            writer.writeF32(music.tempo);
            writer.writeBool(music.loop);
            writer.writeCount(
                music.channels.size(),
                MaxMusicChannelCount,
                "music.channels"
            );

            for (const MusicChannelDefinition& channel : music.channels)
            {
                writer.writeString(channel.id);
                writeSource(writer, channel.instrument.source);
                writeTone(writer, channel.instrument.tone);
                writer.writeBool(channel.instrument.play.legato);
                writer.writeF32(channel.instrument.play.glide);
                writer.writeF32(channel.instrument.play.vibrato);
                writer.writeString(channel.instrument.range.min);
                writer.writeString(channel.instrument.range.max);
                writer.writeF32(channel.volume);
                writer.writeString(channel.length);
                writeStringVector(writer, channel.notes);
            }
        }

        MusicDefinition readMusic(BinaryReader& reader)
        {
            MusicDefinition music;
            music.tempo = reader.readF32("music.tempo");
            music.loop = reader.readBool("music.loop");

            const uint32_t channelCount =
                reader.readCount(
                    MaxMusicChannelCount,
                    "music.channels"
                );

            music.channels.reserve(channelCount);

            for (uint32_t i = 0; i < channelCount; ++i)
            {
                MusicChannelDefinition channel;
                channel.id = reader.readString("music.channels.id");
                channel.instrument.source = readSource(reader);
                channel.instrument.tone = readTone(reader);
                channel.instrument.play.legato = reader.readBool("music.instrument.play.legato");
                channel.instrument.play.glide = reader.readF32("music.instrument.play.glide");
                channel.instrument.play.vibrato = reader.readF32("music.instrument.play.vibrato");
                channel.instrument.range.min = reader.readString("music.instrument.range.min");
                channel.instrument.range.max = reader.readString("music.instrument.range.max");
                channel.volume = reader.readF32("music.channels.volume");
                channel.length = reader.readString("music.channels.length");
                channel.notes = readStringVector(reader);
                music.channels.push_back(channel);
            }

            return music;
        }

        void writeMechanicsSpeed(BinaryWriter& writer, const MechanicsSpeedDefinition& speed)
        {
            writer.writeF32(speed.start);
            writer.writeF32(speed.limit);
        }

        MechanicsSpeedDefinition readMechanicsSpeed(BinaryReader& reader, const std::string& field)
        {
            MechanicsSpeedDefinition speed;
            speed.start = reader.readF32(field + ".start");
            speed.limit = reader.readF32(field + ".limit");
            return speed;
        }

        void writeMechanicsAxis(BinaryWriter& writer, const MechanicsAxisDefinition& axis)
        {
            writeMechanicsSpeed(writer, axis.speed);
            writer.writeF32(axis.acceleration);
            writer.writeF32(axis.inertia);
            writer.writeF32(axis.step);
            writer.writeBool(axis.hasSpeed);
            writer.writeBool(axis.hasAcceleration);
            writer.writeBool(axis.hasInertia);
            writer.writeBool(axis.hasStep);
        }

        MechanicsAxisDefinition readMechanicsAxis(BinaryReader& reader, const std::string& field)
        {
            MechanicsAxisDefinition axis;
            axis.speed = readMechanicsSpeed(reader, field + ".speed");
            axis.acceleration = reader.readF32(field + ".acceleration");
            axis.inertia = reader.readF32(field + ".inertia");
            axis.step = reader.readF32(field + ".step");
            axis.hasSpeed = reader.readBool(field + ".hasSpeed");
            axis.hasAcceleration = reader.readBool(field + ".hasAcceleration");
            axis.hasInertia = reader.readBool(field + ".hasInertia");
            axis.hasStep = reader.readBool(field + ".hasStep");
            return axis;
        }

        void writeMechanics(BinaryWriter& writer, const MechanicsDefinition& mechanics)
        {
            writer.writeU8(
                mechanics.type == MechanicsType::Polar
                    ? 1
                    : 0
            );
            writeMechanicsSpeed(writer, mechanics.motion.speed);
            writer.writeF32(mechanics.motion.acceleration);
            writer.writeF32(mechanics.motion.inertia);
            writer.writeF32(mechanics.motion.step);
            writer.writeU8(
                mechanics.motion.diagonal == MechanicsDiagonalMode::Vector
                    ? 1
                    : 0
            );
            writeMechanicsAxis(writer, mechanics.motion.horizontal);
            writeMechanicsAxis(writer, mechanics.motion.vertical);
            writer.writeF32(mechanics.rotation.angle);
            writeMechanicsSpeed(writer, mechanics.rotation.speed);
            writer.writeF32(mechanics.rotation.acceleration);
            writer.writeF32(mechanics.rotation.inertia);
            writer.writeF32(mechanics.rotation.step);
        }

        MechanicsDefinition readMechanics(BinaryReader& reader)
        {
            MechanicsDefinition mechanics;
            const uint8_t type =
                reader.readU8("object.mechanics.type");

            mechanics.type =
                type == 1
                    ? MechanicsType::Polar
                    : MechanicsType::Direct;

            mechanics.motion.speed =
                readMechanicsSpeed(reader, "object.mechanics.motion.speed");
            mechanics.motion.acceleration =
                reader.readF32("object.mechanics.motion.acceleration");
            mechanics.motion.inertia =
                reader.readF32("object.mechanics.motion.inertia");
            mechanics.motion.step =
                reader.readF32("object.mechanics.motion.step");

            const uint8_t diagonalValue =
                reader.readU8("object.mechanics.motion.diagonal");

            mechanics.motion.diagonal =
                diagonalValue == 1
                    ? MechanicsDiagonalMode::Vector
                    : MechanicsDiagonalMode::Independent;

            mechanics.motion.horizontal =
                readMechanicsAxis(reader, "object.mechanics.motion.horizontal");
            mechanics.motion.vertical =
                readMechanicsAxis(reader, "object.mechanics.motion.vertical");
            mechanics.rotation.angle =
                reader.readF32("object.mechanics.rotation.angle");
            mechanics.rotation.speed =
                readMechanicsSpeed(reader, "object.mechanics.rotation.speed");
            mechanics.rotation.acceleration =
                reader.readF32("object.mechanics.rotation.acceleration");
            mechanics.rotation.inertia =
                reader.readF32("object.mechanics.rotation.inertia");
            mechanics.rotation.step =
                reader.readF32("object.mechanics.rotation.step");

            return mechanics;
        }

        void writeInherit(BinaryWriter& writer, const InheritDefinition& inherit)
        {
            writer.writeU8(static_cast<uint8_t>(inherit.creationAngle));
            writer.writeU8(static_cast<uint8_t>(inherit.creationVelocity));
            writer.writeU8(static_cast<uint8_t>(inherit.liveAngle));
        }

        InheritDefinition readInherit(BinaryReader& reader)
        {
            InheritDefinition inherit;
            inherit.creationAngle =
                static_cast<InheritCreationMode>(
                    reader.readU8("object.inherit.creation.angle")
                );
            inherit.creationVelocity =
                static_cast<InheritCreationMode>(
                    reader.readU8("object.inherit.creation.velocity")
                );
            inherit.liveAngle =
                static_cast<InheritLiveMode>(
                    reader.readU8("object.inherit.live.angle")
                );
            return inherit;
        }

        void writeObject(BinaryWriter& writer, const ObjectDefinition& object)
        {
            writer.writeString(object.id);
            writer.writeString(object.sourcePath);
            writer.writeString(object.spawnMode);
            writeVector2(writer, object.offset);
            writer.writeBool(object.hasOffset);
            writer.writeBool(object.attachFollowX);
            writer.writeBool(object.attachFollowY);
            writer.writeBool(object.attachFollowAngle);
            writer.writeBool(object.attachOnCreate);
            writer.writeBool(object.visible);
            writer.writeBool(object.hasVisual);
            writer.writeI32(object.layer);
            writeVector2(writer, object.origin);
            writer.writeBool(object.hasOrigin);
            writeVector2(writer, object.size);
            writeColor(writer, object.color);
            writer.writeString(object.shapeMode);
            writer.writeString(object.shapeType);
            writer.writeString(object.textContent);
            writer.writeF32(object.radius);
            writer.writeCount(
                object.points.size(),
                MaxPointCount,
                "object.points"
            );

            for (Vector2 point : object.points)
            {
                writeVector2(writer, point);
            }

            writeMechanics(writer, object.mechanics);
            writeInherit(writer, object.inherit);
            writer.writeString(object.boundsMode);
            writer.writeBool(object.boundsOverflow);
            writer.writeString(object.group);
            writer.writeString(object.role);
            writer.writeI32(object.controlPlayer);
            writer.writeString(object.collisionType);
            writer.writeBool(object.collisionActive);
            writer.writeF32(object.collisionRadius);
            writeStringVector(writer, object.collisionWith);
            writeStringVector(writer, object.scripts);
            writeStringVector(writer, object.resolvedScriptPaths);

            const auto musicKeys =
                sortedKeys(object.music);

            writer.writeCount(
                musicKeys.size(),
                MaxCollectionCount,
                "object.music"
            );

            for (const std::string& key : musicKeys)
            {
                writer.writeString(key);
                writeMusic(writer, object.music.at(key));
            }

            const auto soundKeys =
                sortedKeys(object.sounds);

            writer.writeCount(
                soundKeys.size(),
                MaxCollectionCount,
                "object.sounds"
            );

            for (const std::string& key : soundKeys)
            {
                writer.writeString(key);
                writeSound(writer, object.sounds.at(key));
            }

            const auto childResourceKeys =
                sortedKeys(object.childResources);

            writer.writeCount(
                childResourceKeys.size(),
                MaxCollectionCount,
                "object.childResources"
            );

            for (const std::string& key : childResourceKeys)
            {
                writer.writeString(key);
                writer.writeString(object.childResources.at(key));
            }

            writer.writeString(object.initialState);

            const auto transitionKeys =
                sortedKeys(object.stateTransitions);

            writer.writeCount(
                transitionKeys.size(),
                MaxCollectionCount,
                "object.states"
            );

            for (const std::string& key : transitionKeys)
            {
                writer.writeString(key);
                writeStringVector(writer, object.stateTransitions.at(key));
            }

            writer.writeString(object.creationMode);
            writer.writeI32(object.gridRules.rows);
            writer.writeI32(object.gridRules.columns);
            writer.writeF32(object.gridRules.cellWidth);
            writer.writeF32(object.gridRules.cellHeight);
            writer.writeBool(object.gridPatternIsRows);
            writeStringVector(writer, object.gridPattern);
            writer.writeCount(
                object.gridRowPattern.size(),
                MaxGridRowCount,
                "object.creation.grid.rows"
            );

            for (const auto& row : object.gridRowPattern)
            {
                writeStringVector(writer, row);
            }
        }

        ObjectDefinition readObject(BinaryReader& reader)
        {
            ObjectDefinition object;
            object.id = reader.readString("object.id");
            object.sourcePath = reader.readString("object.sourcePath");
            object.spawnMode = reader.readString("object.spawn");
            object.offset = readVector2(reader);
            object.hasOffset = reader.readBool("object.offset");
            object.attachFollowX = reader.readBool("object.attach.x");
            object.attachFollowY = reader.readBool("object.attach.y");
            object.attachFollowAngle = reader.readBool("object.attach.angle");
            object.attachOnCreate = reader.readBool("object.attach.born");
            object.visible = reader.readBool("object.visible");
            object.hasVisual = reader.readBool("object.shape");
            object.layer = reader.readI32("object.layer");
            object.origin = readVector2(reader);
            object.hasOrigin = reader.readBool("object.origin");
            object.size = readVector2(reader);
            object.color = readColor(reader);
            object.shapeMode = reader.readString("object.shape.mode");
            object.shapeType = reader.readString("object.shape.type");
            object.textContent = reader.readString("object.shape.content");
            object.radius = reader.readF32("object.shape.radius");

            const uint32_t pointCount =
                reader.readCount(
                    MaxPointCount,
                    "object.shape.points"
                );

            object.points.reserve(pointCount);

            for (uint32_t i = 0; i < pointCount; ++i)
            {
                object.points.push_back(readVector2(reader));
            }

            object.mechanics = readMechanics(reader);
            object.inherit = readInherit(reader);
            object.boundsMode = reader.readString("object.bounds.mode");
            object.boundsOverflow = reader.readBool("object.bounds.overflow");
            object.group = reader.readString("object.group");
            object.role = reader.readString("object.role");
            object.controlPlayer = reader.readI32("object.control.player");
            object.collisionType = reader.readString("object.collision.type");
            object.collisionActive = reader.readBool("object.collision.active");
            object.collisionRadius = reader.readF32("object.collision.radius");
            object.collisionWith = readStringVector(reader);
            object.scripts = readStringVector(reader);
            object.resolvedScriptPaths = readStringVector(reader);

            const uint32_t musicCount =
                reader.readCount(
                    MaxCollectionCount,
                    "object.music"
                );

            for (uint32_t i = 0; i < musicCount; ++i)
            {
                const std::string key =
                    reader.readString("object.music.id");

                auto inserted =
                    object.music.insert(
                        { key, readMusic(reader) }
                    );

                if (!inserted.second)
                {
                    throw BinaryException(
                        DiagnosticCode::DuplicateCompiledEntry,
                        "Duplicate music entry in compiled object",
                        "object.music." + key
                    );
                }
            }

            const uint32_t soundCount =
                reader.readCount(
                    MaxCollectionCount,
                    "object.sounds"
                );

            for (uint32_t i = 0; i < soundCount; ++i)
            {
                const std::string key =
                    reader.readString("object.sounds.id");

                auto inserted =
                    object.sounds.insert(
                        { key, readSound(reader) }
                    );

                if (!inserted.second)
                {
                    throw BinaryException(
                        DiagnosticCode::DuplicateCompiledEntry,
                        "Duplicate sound entry in compiled object",
                        "object.sounds." + key
                    );
                }
            }

            const uint32_t childResourceCount =
                reader.readCount(
                    MaxCollectionCount,
                    "object.childResources"
                );

            for (uint32_t i = 0; i < childResourceCount; ++i)
            {
                const std::string key =
                    reader.readString("object.childResources.id");

                const std::string resourceId =
                    reader.readString("object.childResources.resourceId");

                auto inserted =
                    object.childResources.insert(
                        { key, resourceId }
                    );

                if (!inserted.second)
                {
                    throw BinaryException(
                        DiagnosticCode::DuplicateCompiledEntry,
                        "Duplicate child resource entry in compiled object",
                        "object.childResources." + key
                    );
                }
            }

            object.initialState = reader.readString("object.states.initial");

            const uint32_t transitionCount =
                reader.readCount(
                    MaxCollectionCount,
                    "object.states"
                );

            for (uint32_t i = 0; i < transitionCount; ++i)
            {
                const std::string key =
                    reader.readString("object.states.id");

                auto inserted =
                    object.stateTransitions.insert(
                        { key, readStringVector(reader) }
                    );

                if (!inserted.second)
                {
                    throw BinaryException(
                        DiagnosticCode::DuplicateCompiledEntry,
                        "Duplicate state transition entry in compiled object",
                        "object.states." + key
                    );
                }
            }

            object.creationMode = reader.readString("object.creation.mode");
            object.gridRules.rows = reader.readI32("object.creation.grid.rows");
            object.gridRules.columns = reader.readI32("object.creation.grid.columns");
            object.gridRules.cellWidth = reader.readF32("object.creation.grid.cellWidth");
            object.gridRules.cellHeight = reader.readF32("object.creation.grid.cellHeight");
            object.gridPatternIsRows = reader.readBool("object.creation.pattern.rows");
            object.gridPattern = readStringVector(reader);

            const uint32_t rowCount =
                reader.readCount(
                    MaxGridRowCount,
                    "object.creation.grid.rowPattern"
                );

            object.gridRowPattern.reserve(rowCount);

            for (uint32_t i = 0; i < rowCount; ++i)
            {
                object.gridRowPattern.push_back(readStringVector(reader));
            }

            return object;
        }

        void writeContext(BinaryWriter& writer, const FlxContext& context)
        {
            writer.writeString(context.name);
            writer.writeString(context.version);
            writer.writeString(context.engineRequirement);
            writer.writeString(context.notes);
            writer.writeString(context.title);
            writer.writeString(context.inputMappingSourceName);
            writer.writeString(context.inputMappingContent);
            writeMachine(writer, context.machine);
        }

        FlxContext readContext(BinaryReader& reader)
        {
            FlxContext context;
            context.name = reader.readString("context.name");
            context.version = reader.readString("context.version");
            context.engineRequirement = reader.readString("context.engine");
            context.notes = reader.readString("context.notes");
            context.title = reader.readString("context.title");
            context.inputMappingSourceName = reader.readString("context.input.source");
            context.inputMappingContent = reader.readString("context.input.content");
            context.machine = readMachine(reader);
            return context;
        }
    }

    void CompiledProjectCodec::write(
        BinaryWriter& writer,
        const CompiledProject& project
    )
    {
        writer.writeU32(Magic);
        writer.writeU32(FormatVersion);
        writer.writeString(std::string(FlxVersion::Text));
        writeContext(writer, project.context);
        writer.writeString(project.rootId);

        const auto objectKeys =
            sortedKeys(project.resources.allObjects());

        writer.writeCount(
            objectKeys.size(),
            MaxResourceCount,
            "resources.objects"
        );

        for (const std::string& key : objectKeys)
        {
            writer.accountResource("resources.objects");
            writer.writeString(key);
            writeObject(writer, project.resources.allObjects().at(key));
        }

        const auto scriptKeys =
            sortedKeys(project.resources.allScripts());

        writer.writeCount(
            scriptKeys.size(),
            MaxResourceCount,
            "resources.scripts"
        );

        for (const std::string& key : scriptKeys)
        {
            writer.accountResource("resources.scripts");
            const ScriptResource& script =
                project.resources.allScripts().at(key);

            writer.writeString(key);
            writer.writeString(script.id);
            writer.writeString(script.sourceName);
            writer.writeString(script.code);
        }
    }

    DecodedCompiledProject CompiledProjectCodec::read(BinaryReader& reader)
    {
        DecodedCompiledProject decoded;

        if (reader.readU32("magic") != Magic)
        {
            throw BinaryException(
                DiagnosticCode::InvalidCompiledProjectMagic,
                "Invalid compiled project magic",
                "magic"
            );
        }

        decoded.metadata.formatVersion =
            reader.readU32("formatVersion");

        if (decoded.metadata.formatVersion != FormatVersion)
        {
            throw BinaryException(
                DiagnosticCode::UnsupportedCompiledProjectFormat,
                "Unsupported compiled project format",
                "formatVersion"
            );
        }

        decoded.metadata.producerVersion =
            reader.readString("producerVersion");

        decoded.project.context =
            readContext(reader);

        decoded.project.rootId =
            reader.readString("rootId");

        const uint32_t objectCount =
            reader.readCount(
                MaxResourceCount,
                "resources.objects"
            );

        for (uint32_t i = 0; i < objectCount; ++i)
        {
            reader.accountResource("resources.objects");
            const ResourceId id =
                reader.readString("resources.objects.id");

            if (decoded.project.resources.hasObject(id))
            {
                throw BinaryException(
                    DiagnosticCode::DuplicateCompiledResource,
                    "Duplicate object resource in compiled project",
                    id
                );
            }

            if (!decoded.project.resources.addObject(
                id,
                readObject(reader)
            ))
            {
                throw BinaryException(
                    DiagnosticCode::DuplicateCompiledResource,
                    "Duplicate object resource in compiled project",
                    id
                );
            }
        }

        const uint32_t scriptCount =
            reader.readCount(
                MaxResourceCount,
                "resources.scripts"
            );

        for (uint32_t i = 0; i < scriptCount; ++i)
        {
            reader.accountResource("resources.scripts");
            const ResourceId key =
                reader.readString("resources.scripts.id");

            if (decoded.project.resources.hasScript(key))
            {
                throw BinaryException(
                    DiagnosticCode::DuplicateCompiledResource,
                    "Duplicate script resource in compiled project",
                    key
                );
            }

            ScriptResource script;
            script.id = reader.readString("resources.scripts.resourceId");
            script.sourceName = reader.readString("resources.scripts.sourceName");
            script.code = reader.readString("resources.scripts.code");

            if (!decoded.project.resources.addScript(
                key,
                script
            ))
            {
                throw BinaryException(
                    DiagnosticCode::DuplicateCompiledResource,
                    "Duplicate script resource in compiled project",
                    key
                );
            }
        }

        if (reader.hasTrailingData())
        {
            throw BinaryException(
                DiagnosticCode::TrailingCompiledProjectData,
                "Compiled project contains trailing data",
                "eof"
            );
        }

        return decoded;
    }
}
