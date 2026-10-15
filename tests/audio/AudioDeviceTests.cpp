#include "../../engine/audio/AudioSystem.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace
{
    void require(bool value, const char* message)
    {
        if (!value) throw std::runtime_error(message);
    }
    void pump(AudioSystem& audio, int milliseconds)
    {
        const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
        do
        {
            audio.update();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        } while (std::chrono::steady_clock::now() < end);
    }
}
int main()
{
    try
    {
        AudioSystem audio;
        audio.init();
        if (!IsAudioDeviceReady())
        {
            std::cout << "[SKIP] no raylib audio device available\n";
            return 77;
        }
        MusicDefinition music;
        music.tempo = 300.0f;
        MusicChannelDefinition channel;
        channel.length = "1/16";
        channel.notes = { "C4", "G4", "E4" };
        music.channels.push_back(channel);
        audio.playMusic(music);
        require(audio.isMusicActive(), "start active");
        audio.togglePauseMusic();
        pump(audio, 300);
        require(audio.isMusicActive() && audio.isMusicPaused(), "pause retains short score");
        audio.togglePauseMusic();
        pump(audio, 400);
        require(!audio.isMusicActive(), "short score drains naturally");
        music.loop = true;
        audio.playMusic(music);
        pump(audio, 700);
        require(audio.isMusicActive(), "loop remains active");
        audio.playMusic(music);
        require(audio.isMusicActive() && !audio.isMusicPaused(), "replace music");
        AudioChipDefinition chip;
        chip.voicesMusic = 1; chip.voicesSound = 0; chip.voicesOverflow = "steal_from_music";
        audio.configure(chip);
        require(audio.testPlayLogicalSound(1), "borrow music voices");
        audio.togglePauseMusic(); audio.testFinishAllSounds();
        pump(audio, 100);
        require(audio.isMusicActive() && audio.isMusicPaused(), "logical pause survives suspension");
        audio.togglePauseMusic(); pump(audio, 200);
        audio.stopMusic(); audio.stopMusic();
        require(!audio.isMusicActive(), "stop inactive");

        // Same score storage, very different musical duration. PCM stays bounded.
        music.loop = false;
        music.channels[0].notes.assign(1000, "C4");
        music.tempo = 20.0f;
        auto start = std::chrono::steady_clock::now();
        audio.playMusic(music);
        const auto streamMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        audio.stopMusic();
        music.tempo = 300.0f;
        start = std::chrono::steady_clock::now();
        Wave reference = audio.testCreateMusicWave(music, 1);
        const auto legacyMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        UnloadWave(reference);
        start = std::chrono::steady_clock::now();
        audio.playMusic(music);
        const auto shortStreamMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        audio.shutdown();
        require(!audio.isMusicActive(), "shutdown inactive");
        std::cout << "[PASS] raylib device lifecycle\n"
                  << "playMusic 750-second score: " << streamMs << " ms\n"
                  << "playMusic 50-second score: " << shortStreamMs << " ms\n"
                  << "legacy synthesis 50-second score: " << legacyMs << " ms\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "[FAIL] " << error.what() << '\n'; return 1;
    }
}
