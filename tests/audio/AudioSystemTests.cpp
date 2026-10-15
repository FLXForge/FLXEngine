#include "../support/TestSupport.h"
#include "../../engine/audio/AudioSystem.h"
#include "../../engine/machine/MachineDefinition.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace flx::test;

namespace
{
    const ObjectDefinition& rootObject(const CompiledProject& project)
    {
        const ObjectDefinition* root =
            project.resources.findObject(project.rootId);

        require(root != nullptr, "root should exist");
        return *root;
    }

    AudioChipDefinition chip(
        int musicVoices,
        int soundVoices,
        const std::string& mode,
        const std::string& overflow
    )
    {
        AudioChipDefinition value;
        value.voicesMusic = musicVoices;
        value.voicesSound = soundVoices;
        value.voicesMode = mode;
        value.voicesOverflow = overflow;
        return value;
    }

    void requirePriorities(
        const AudioSystem& audio,
        const std::vector<int>& expected,
        const std::string& message
    )
    {
        const std::vector<int> priorities =
            audio.testActiveSoundPriorities();

        require(priorities == expected, message);
    }

    void testIncrementalSynthesisMatchesLegacy()
    {
        for (float tempo : { 73.0f, 137.0f, 300.0f })
        for (const std::string wave : { "sine", "square", "triangle", "saw", "pulse", "noise" })
        for (const std::string resolution : { "high", "low" })
        {
            MusicDefinition music;
            music.tempo = tempo;
            MusicChannelDefinition channel;
            channel.notes = { "C4", "-", "G4", "A4", "", "D4" };
            channel.length = "1/16";
            channel.instrument.source.wave = wave;
            channel.instrument.source.duty = 0.3f;
            channel.instrument.play.vibrato = 0.7f;
            channel.instrument.tone.envelope = { 0.01f, 0.025f, 0.6f, 0.02f };
            channel.instrument.tone.material = { 0.8f, 0.4f, wave == "noise" ? 0.2f : 0.0f, 0.2f, 0.3f };
            channel.instrument.tone.space.echo = 0.8f;
            music.channels.push_back(channel);
            channel.length = "1/8";
            channel.notes = { "G3", "F3", "-", "C3" };
            channel.volume = 0.3f;
            music.channels.push_back(channel);
            AudioChipDefinition config;
            config.fidelityResolution = resolution;
            AudioSystem audio;
            audio.configure(config);
            SetRandomSeed(1234);
            Wave reference = audio.testCreateMusicWave(music, 2);
            const std::vector<short> expected(static_cast<short*>(reference.data),
                static_cast<short*>(reference.data) + reference.frameCount);
            UnloadWave(reference);
            for (size_t blockSize : { 1, 127, 1984, 4096, 8191 })
            {
                SetRandomSeed(1234);
                MusicSequencer sequencer(music, 2, config);
                std::vector<short> actual, block(blockSize);
                while (!sequencer.finished())
                {
                    const size_t count = sequencer.generate(block.data(), block.size(), false);
                    actual.insert(actual.end(), block.begin(), block.begin() + count);
                }
                require(actual == expected, "incremental PCM must exactly match legacy PCM: " + wave);
                require(sequencer.generate(block.data(), block.size(), false) == 0, "end must be stable");
                MusicSequencer looping(music, 2, config);
                if (wave != "noise")
                {
                    std::vector<short> repeated(expected.size() * 2 + 17);
                    require(looping.generate(repeated.data(), repeated.size(), true) == repeated.size(), "loop fills block");
                    for (size_t i = 0; i < repeated.size(); ++i)
                        require(repeated[i] == expected[i % expected.size()], "loop restarts score and echo exactly");
                }
            }
        }
    }

    void testSequencerEdgeCases()
    {
        MusicDefinition music;
        MusicChannelDefinition channel;
        channel.notes = { "invalid", "C0", "x", "-", "G4" };
        channel.instrument.range.min = "C4";
        channel.instrument.range.max = "C5";
        music.channels.push_back(channel);
        music.channels.push_back(channel);
        AudioChipDefinition config;
        config.synthesisModel = "pulse";
        config.fidelityDynamics = "limited";
        config.fidelityResolution = "very_low";
        AudioSystem audio;
        audio.configure(config);
        Wave reference = audio.testCreateMusicWave(music, 1);
        MusicSequencer sequencer(music, 1, config);
        std::vector<short> actual(reference.frameCount + 4, 123);
        require(sequencer.generate(actual.data(), 0, false) == 0, "zero capacity does not advance");
        require(sequencer.generate(actual.data(), actual.size(), false) == reference.frameCount, "only admitted channels determine duration");
        require(std::equal(actual.begin(), actual.begin() + reference.frameCount,
            static_cast<short*>(reference.data)), "invalid notes and range match legacy");
        require(actual[reference.frameCount] == 123, "partial block never writes beyond produced frames");
        UnloadWave(reference);
        MusicSequencer empty(MusicDefinition{}, 10, config);
        require(empty.frameCount() == 441, "empty score retains minimum duration");
        MusicSequencer noChannels(music, -1, config);
        require(noChannels.frameCount() == 441, "negative channel count is bounded");
    }

    void testRepeatedMusicOperations()
    {
        AudioSystem audio;
        for (int i = 0; i < 3; ++i)
        {
            audio.stopMusic();
            audio.togglePauseMusic();
            require(!audio.isMusicActive() && !audio.isMusicPaused(), "empty operations are harmless");
            audio.testStartLogicalMusic(2);
            audio.togglePauseMusic();
            audio.stopMusic();
        }
    }

    void testSoundPriorityLoadsFromJson()
    {
        const std::filesystem::path root =
            testRoot() / "audio_priority_loading";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=AudioPriority\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{\n"
            "  \"sounds\": {\n"
            "    \"default\": { \"duration\": 0.1 },\n"
            "    \"important\": { \"duration\": 0.1, \"priority\": 7 }\n"
            "  }\n"
            "}\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(result.success, "sound priority project should compile");

        const ObjectDefinition& object =
            rootObject(result.project);

        require(
            object.sounds.at("default").priority == 0,
            "sound priority should default to zero, got " +
            std::to_string(object.sounds.at("default").priority)
        );

        require(
            object.sounds.at("important").priority == 7,
            "declared sound priority should load, got " +
            std::to_string(object.sounds.at("important").priority)
        );
    }

    void testAudioSystemUsesSoundDefinitionPriority()
    {
        AudioSystem audio;
        audio.configure(chip(0, 2, "reserved", "replace_lowest_priority"));

        SoundDefinition low;
        low.priority = 1;
        low.duration = 1000.0f;

        SoundDefinition high;
        high.priority = 9;
        high.duration = 1000.0f;

        require(audio.testPlayLogicalSound(low), "low-priority definition should play");
        require(audio.testPlayLogicalSound(high), "high-priority definition should play");

        requirePriorities(audio, { 1, 9 }, "logical sound playback should keep definition priority");
    }

    void testIgnoreKeepsExistingSounds()
    {
        AudioSystem audio;
        audio.configure(chip(0, 2, "reserved", "ignore"));

        require(audio.testPlayLogicalSound(10), "A should play");
        require(audio.testPlayLogicalSound(20), "B should play");
        require(!audio.testPlayLogicalSound(30), "C should be rejected");

        requirePriorities(audio, { 10, 20 }, "ignore should keep A and B");
    }

    void testReplaceOldestKeepsNewerAndIncoming()
    {
        AudioSystem audio;
        audio.configure(chip(0, 2, "reserved", "replace_oldest"));

        require(audio.testPlayLogicalSound(10), "A should play");
        require(audio.testPlayLogicalSound(20), "B should play");
        require(audio.testPlayLogicalSound(30), "C should replace oldest");

        requirePriorities(audio, { 20, 30 }, "replace_oldest should keep B and C");
    }

    void testReplaceNewestKeepsOlderAndIncoming()
    {
        AudioSystem audio;
        audio.configure(chip(0, 2, "reserved", "replace_newest"));

        require(audio.testPlayLogicalSound(10), "A should play");
        require(audio.testPlayLogicalSound(20), "B should play");
        require(audio.testPlayLogicalSound(30), "C should replace newest active sound");

        requirePriorities(audio, { 10, 30 }, "replace_newest should keep A and C");
    }

    void testReplaceLowestPriorityUsesDeclaredPriority()
    {
        AudioSystem audio;
        audio.configure(chip(0, 2, "reserved", "replace_lowest_priority"));

        require(audio.testPlayLogicalSound(5), "low priority sound should play");
        require(audio.testPlayLogicalSound(10), "higher priority sound should play");
        require(audio.testPlayLogicalSound(99), "incoming sound should replace lowest priority active sound");

        requirePriorities(audio, { 10, 99 }, "replace_lowest_priority should remove the lowest active priority");
    }

    void testReplaceLowestPriorityTieUsesOldestSound()
    {
        AudioSystem audio;
        audio.configure(chip(0, 2, "reserved", "replace_lowest_priority"));

        require(audio.testPlayLogicalSound(1), "A should play");
        require(audio.testPlayLogicalSound(1), "B should play");

        const std::vector<uint64_t> before =
            audio.testActiveSoundOrders();

        require(audio.testPlayLogicalSound(9), "C should replace oldest tied active sound");

        const std::vector<uint64_t> after =
            audio.testActiveSoundOrders();

        require(after.size() == 2, "two sounds should remain");
        require(after[0] == before[1], "oldest tied sound should be selected deterministically");
        requirePriorities(audio, { 1, 9 }, "tie should keep newer tied sound and incoming sound");
    }

    void testReservedKeepsSoundAndMusicPoolsSeparate()
    {
        AudioSystem audio;
        audio.configure(chip(1, 0, "reserved", "replace_oldest"));
        audio.testStartLogicalMusic(1);

        require(!audio.testPlayLogicalSound(10), "reserved sound pool with zero voices should reject sound");
        require(audio.testMusicLoaded(), "reserved sound rejection should not stop music");
        require(!audio.testMusicPaused(), "reserved sound rejection should not pause music");
    }

    void testSharedUsesCommonCapacity()
    {
        AudioSystem audio;
        audio.configure(chip(1, 0, "shared", "ignore"));

        require(audio.testPlayLogicalSound(10), "shared pool should let sound use free music capacity");
        requirePriorities(audio, { 10 }, "sound should occupy the shared voice");

        AudioSystem fullAudio;
        fullAudio.configure(chip(1, 0, "shared", "ignore"));
        fullAudio.testStartLogicalMusic(1);

        require(!fullAudio.testPlayLogicalSound(20), "shared full pool should reject sound without overflow help");
        require(fullAudio.testMusicLoaded(), "shared rejection should leave music loaded");
    }

    void testSharedReplaceOldestDoesNotReplaceMusic()
    {
        AudioSystem audio;
        audio.configure(chip(1, 1, "shared", "replace_oldest"));
        audio.testStartLogicalMusic(1);

        require(audio.testPlayLogicalSound(10), "sound should fill remaining shared voice");
        require(audio.testPlayLogicalSound(20), "replace_oldest should replace an active sound, not music");

        require(audio.testMusicLoaded(), "replace_oldest must not stop music");
        require(!audio.testMusicPaused(), "replace_oldest must not pause music");
        requirePriorities(audio, { 20 }, "replace_oldest should keep incoming sound");
    }

    void testSharedReplaceNewestDoesNotReplaceMusic()
    {
        AudioSystem audio;
        audio.configure(chip(1, 1, "shared", "replace_newest"));
        audio.testStartLogicalMusic(1);

        require(audio.testPlayLogicalSound(10), "sound should fill remaining shared voice");
        require(audio.testPlayLogicalSound(20), "replace_newest should replace an active sound, not music");

        require(audio.testMusicLoaded(), "replace_newest must not stop music");
        require(!audio.testMusicPaused(), "replace_newest must not pause music");
        requirePriorities(audio, { 20 }, "replace_newest should keep incoming sound");
    }

    void testSharedReplaceLowestPriorityDoesNotReplaceMusic()
    {
        AudioSystem audio;
        audio.configure(chip(1, 1, "shared", "replace_lowest_priority"));
        audio.testStartLogicalMusic(1);

        require(audio.testPlayLogicalSound(1), "sound should fill remaining shared voice");
        require(audio.testPlayLogicalSound(9), "replace_lowest_priority should replace an active sound, not music");

        require(audio.testMusicLoaded(), "replace_lowest_priority must not stop music");
        require(!audio.testMusicPaused(), "replace_lowest_priority must not pause music");
        requirePriorities(audio, { 9 }, "replace_lowest_priority should keep incoming sound");
    }

    void testStealFromMusicSuspendsAndRestoresMusicMaterialization()
    {
        AudioSystem audio;
        audio.configure(chip(1, 0, "shared", "steal_from_music"));
        audio.testStartLogicalMusic(1);

        require(audio.isMusicActive(), "music should start active");
        require(!audio.isMusicPaused(), "music should start logically playing");

        require(audio.testPlayLogicalSound(7), "steal_from_music should let sound borrow music capacity");
        require(audio.testMusicLoaded(), "steal_from_music should keep music loaded");
        require(audio.isMusicActive(), "machine-suspended music should remain logically active");
        require(!audio.isMusicPaused(), "steal_from_music must not change logical pause state");
        require(!audio.testMusicPaused(), "test logical paused flag should remain false during steal");
        require(audio.testMusicSuspendedBySound(), "music should be marked as machine-suspended");

        audio.testFinishAllSounds();

        require(audio.testMusicLoaded(), "music should remain loaded after sound finishes");
        require(!audio.testMusicPaused(), "music should remain logically playing after stolen capacity is released");
        require(!audio.testMusicSuspendedBySound(), "music suspension should clear after sound finishes");
        require(audio.isMusicActive(), "music should remain active after stolen capacity is released");
        require(!audio.isMusicPaused(), "music_paused should remain false after automatic resume");
    }

    void testPauseMusicRemainsToggle()
    {
        AudioSystem audio;
        audio.configure(chip(1, 0, "shared", "ignore"));
        audio.testStartLogicalMusic(1);

        require(audio.isMusicActive(), "synthetic music should be active");
        require(!audio.isMusicPaused(), "music should start unpaused");

        audio.togglePauseMusic();

        require(audio.isMusicActive(), "paused music should still be active");
        require(audio.isMusicPaused(), "first toggle should pause music");

        audio.togglePauseMusic();

        require(audio.isMusicActive(), "resumed music should be active");
        require(!audio.isMusicPaused(), "second toggle should resume music");
    }

    void testPauseDuringStealSurvivesSoundFinish()
    {
        AudioSystem audio;
        audio.configure(chip(1, 0, "shared", "steal_from_music"));
        audio.testStartLogicalMusic(1);

        require(audio.testPlayLogicalSound(7), "sound should steal music capacity");
        require(audio.testMusicSuspendedBySound(), "music should be suspended while sound is active");
        require(!audio.isMusicPaused(), "machine suspension should not appear as logical pause");

        audio.togglePauseMusic();

        require(audio.isMusicActive(), "paused music should remain active during steal");
        require(audio.isMusicPaused(), "pause_music should set logical pause during steal");
        require(audio.testMusicSuspendedBySound(), "machine suspension should remain while sound is active");

        audio.testFinishAllSounds();

        require(audio.isMusicActive(), "logically paused music should remain active after steal release");
        require(audio.isMusicPaused(), "logical pause should survive steal release");
        require(!audio.testMusicSuspendedBySound(), "machine suspension should clear after sound finishes");
    }

    void testResumeDuringStealWaitsForMachineRelease()
    {
        AudioSystem audio;
        audio.configure(chip(1, 0, "shared", "steal_from_music"));
        audio.testStartLogicalMusic(1);

        require(audio.testPlayLogicalSound(7), "sound should steal music capacity");

        audio.togglePauseMusic();
        require(audio.isMusicPaused(), "first toggle during steal should logically pause music");

        audio.togglePauseMusic();
        require(!audio.isMusicPaused(), "second toggle during steal should return to logical playing");
        require(audio.testMusicSuspendedBySound(), "music should stay machine-suspended until sound finishes");
        require(audio.isMusicActive(), "music should remain logically active while waiting for capacity");

        audio.testFinishAllSounds();

        require(!audio.isMusicPaused(), "music should remain logically playing after machine release");
        require(!audio.testMusicSuspendedBySound(), "machine suspension should clear after release");
        require(audio.isMusicActive(), "music should remain active after release");
    }

    void testStopAndReplaceMusicResetLogicalAndMachineState()
    {
        AudioSystem audio;
        audio.configure(chip(1, 0, "shared", "steal_from_music"));
        audio.testStartLogicalMusic(1);

        require(audio.testPlayLogicalSound(7), "sound should steal music capacity");
        audio.togglePauseMusic();

        audio.stopMusic();

        require(!audio.isMusicActive(), "stop_music should clear active music");
        require(!audio.isMusicPaused(), "stop_music should clear logical pause");
        require(!audio.testMusicLoaded(), "stop_music should unload music");
        require(!audio.testMusicSuspendedBySound(), "stop_music should clear machine suspension");

        audio.testStartLogicalMusic(1);

        require(audio.isMusicActive(), "new music should start active");
        require(!audio.isMusicPaused(), "new music should start logically playing");
        require(!audio.testMusicSuspendedBySound(), "new music should not inherit machine suspension");
    }
}

int main()
{
    const std::vector<std::pair<std::string, void(*)()>> tests = {
        { "incremental synthesis matches legacy", testIncrementalSynthesisMatchesLegacy },
        { "sequencer edge cases", testSequencerEdgeCases },
        { "repeated music operations", testRepeatedMusicOperations },
        { "sound priority loads from JSON", testSoundPriorityLoadsFromJson },
        { "AudioSystem uses SoundDefinition priority", testAudioSystemUsesSoundDefinitionPriority },
        { "ignore keeps existing sounds", testIgnoreKeepsExistingSounds },
        { "replace_oldest keeps newer and incoming", testReplaceOldestKeepsNewerAndIncoming },
        { "replace_newest keeps older and incoming", testReplaceNewestKeepsOlderAndIncoming },
        { "replace_lowest_priority uses declared priority", testReplaceLowestPriorityUsesDeclaredPriority },
        { "replace_lowest_priority tie uses oldest sound", testReplaceLowestPriorityTieUsesOldestSound },
        { "reserved keeps sound and music pools separate", testReservedKeepsSoundAndMusicPoolsSeparate },
        { "shared uses common capacity", testSharedUsesCommonCapacity },
        { "shared replace_oldest does not replace music", testSharedReplaceOldestDoesNotReplaceMusic },
        { "shared replace_newest does not replace music", testSharedReplaceNewestDoesNotReplaceMusic },
        { "shared replace_lowest_priority does not replace music", testSharedReplaceLowestPriorityDoesNotReplaceMusic },
        { "steal_from_music suspends and restores music materialization", testStealFromMusicSuspendsAndRestoresMusicMaterialization },
        { "pause_music remains toggle", testPauseMusicRemainsToggle },
        { "pause during steal survives sound finish", testPauseDuringStealSurvivesSoundFinish },
        { "resume during steal waits for machine release", testResumeDuringStealWaitsForMachineRelease },
        { "stop and replace music reset logical and machine state", testStopAndReplaceMusicResetLogicalAndMachineState }
    };

    for (const auto& test : tests)
    {
        try
        {
            std::cout << "[RUN] " << test.first << std::endl;
            test.second();
            std::cout << "[PASS] " << test.first << std::endl;
        }
        catch (const std::exception& exception)
        {
            std::cerr << "[FAIL] " << test.first << ": " << exception.what() << "\n";
            return 1;
        }
    }

    return 0;
}
