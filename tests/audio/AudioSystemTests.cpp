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

    void testStealFromMusicPausesAndRestoresMusic()
    {
        AudioSystem audio;
        audio.configure(chip(1, 0, "shared", "steal_from_music"));
        audio.testStartLogicalMusic(1);

        require(audio.testPlayLogicalSound(7), "steal_from_music should let sound borrow music capacity");
        require(audio.testMusicLoaded(), "steal_from_music should keep music loaded");
        require(audio.testMusicPaused(), "steal_from_music should pause music while sound uses capacity");
        require(audio.testMusicPausedBySound(), "music pause should be marked as sound-driven");

        audio.testFinishAllSounds();

        require(audio.testMusicLoaded(), "music should remain loaded after sound finishes");
        require(!audio.testMusicPaused(), "music should resume after stolen capacity is released");
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
}

int main()
{
    const std::vector<std::pair<std::string, void(*)()>> tests = {
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
        { "steal_from_music pauses and restores music", testStealFromMusicPausesAndRestoresMusic },
        { "pause_music remains toggle", testPauseMusicRemainsToggle }
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
