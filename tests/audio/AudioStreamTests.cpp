#include "../../engine/audio/AudioSystem.h"
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>

// Deterministic model of raylib 6.0's two half-buffers, including Play's reset.
// This executable links the production AudioSystem without raylib's audio device.
struct rAudioBuffer
{
    std::array<bool, 2> processed{ true, true };
    bool playing = false;
    bool paused = false;
    int cursor = 0;
};
namespace
{
    rAudioBuffer* buffer = nullptr;
    int streams = 0, starts = 0, resumes = 0, refills = 0;
    bool failLoad = false, deviceReady = false;
    void require(bool value, const char* message)
    {
        if (!value) throw std::runtime_error(message);
    }
    void consume()
    {
        require(buffer && buffer->playing && !buffer->paused, "consume only while playing");
        buffer->processed[buffer->cursor] = true;
        buffer->cursor = 1 - buffer->cursor;
    }
    MusicDefinition score(int notes, bool loop = false)
    {
        MusicDefinition music;
        music.tempo = 300.0f;
        music.loop = loop;
        MusicChannelDefinition channel;
        channel.length = "1/16";
        channel.notes.assign(notes, "C4");
        music.channels.push_back(channel);
        return music;
    }
}
extern "C"
{
void InitAudioDevice() { deviceReady = true; }
bool IsAudioDeviceReady() { return deviceReady; }
void CloseAudioDevice() { deviceReady = false; }
void SetAudioStreamBufferSizeDefault(int) {}
AudioStream LoadAudioStream(unsigned int rate, unsigned int size, unsigned int channels)
{
    AudioStream stream{};
    if (failLoad) return stream;
    buffer = new rAudioBuffer;
    ++streams;
    stream.buffer = buffer;
    stream.sampleRate = rate; stream.sampleSize = size; stream.channels = channels;
    return stream;
}
bool IsAudioStreamValid(AudioStream stream) { return stream.buffer != nullptr; }
void UnloadAudioStream(AudioStream stream) { delete stream.buffer; buffer = nullptr; --streams; }
void PlayAudioStream(AudioStream stream)
{
    ++starts; stream.buffer->playing = true; stream.buffer->paused = false;
    stream.buffer->cursor = 0; stream.buffer->processed = { true, true };
}
void PauseAudioStream(AudioStream stream) { stream.buffer->paused = true; }
void ResumeAudioStream(AudioStream stream) { ++resumes; stream.buffer->paused = false; }
void StopAudioStream(AudioStream stream) { stream.buffer->playing = false; }
bool IsAudioStreamPlaying(AudioStream stream) { return stream.buffer->playing && !stream.buffer->paused; }
bool IsAudioStreamProcessed(AudioStream stream)
{
    return stream.buffer->processed[0] || stream.buffer->processed[1];
}
void UpdateAudioStream(AudioStream stream, const void*, int frames)
{
    require(frames == 4096, "fixed frame count");
    auto& state = *stream.buffer;
    require(IsAudioStreamProcessed(stream), "never overwrite pending PCM");
    int slot = state.processed[0] ? 0 : 1;
    if (state.processed[0] && state.processed[1]) state.cursor = 0;
    state.processed[slot] = false;
    ++refills;
}
double GetTime() { return 0.0; }
int GetRandomValue(int min, int) { return min; }
Wave WaveCopy(Wave wave)
{
    Wave copy = wave;
    const size_t bytes = wave.frameCount * wave.channels * wave.sampleSize / 8;
    copy.data = std::malloc(bytes); std::memcpy(copy.data, wave.data, bytes); return copy;
}
Sound LoadSoundFromWave(Wave) { return {}; }
void SetSoundVolume(Sound, float) {}
void PlaySound(Sound) {}
void StopSound(Sound) {}
void UnloadSound(Sound) {}
bool IsSoundPlaying(Sound) { return false; }
void UnloadWave(Wave wave) { std::free(wave.data); }
}
int main()
{
    try
    {
        AudioSystem audio;
        audio.init();
        audio.playMusic(score(3)); // 6615 frames: two halves, final half padded.
        require(streams == 1 && refills == 2, "start primes exactly two halves");
        require(!buffer->processed[0] && !buffer->processed[1], "Play must precede priming");
        audio.update();
        require(audio.isMusicActive(), "synthesized final frame is not playback end");
        consume(); audio.update();
        require(audio.isMusicActive(), "last audio half still pending");
        audio.togglePauseMusic();
        require(buffer->paused && audio.isMusicPaused(), "pause backend and logical state");
        const int beforePause = refills;
        audio.update(); require(refills == beforePause, "no synthesis while paused");
        const int beforeStart = starts;
        audio.togglePauseMusic();
        require(starts == beforeStart && !buffer->paused, "resume never restarts");
        consume(); audio.update();
        require(!audio.isMusicActive() && streams == 0, "natural end releases stream after drain");

        audio.playMusic(score(1)); // shorter than one half
        consume(); audio.update();
        require(!audio.isMusicActive(), "short score drains without an extra audio half");
        audio.playMusic(score(8));
        consume(); consume(); // both halves processed after engine stall
        audio.update();
        require(audio.isMusicActive() && !buffer->processed[0] && !buffer->processed[1], "refill both halves after underrun");
        audio.stopMusic();

        audio.playMusic(score(1, true));
        const int loopStarts = starts;
        for (int i = 0; i < 12; ++i) { consume(); audio.update(); }
        require(audio.isMusicActive() && starts == loopStarts, "loop reuses stream without Play");
        audio.togglePauseMusic();
        audio.playMusic(score(3));
        require(streams == 1 && !audio.isMusicPaused(), "replacement unloads old and clears pause");

        AudioChipDefinition chip;
        chip.voicesMusic = 1; chip.voicesSound = 0; chip.voicesOverflow = "steal_from_music";
        audio.configure(chip);
        require(audio.testPlayLogicalSound(1), "effect borrows music capacity");
        require(buffer->paused && audio.isMusicActive() && !audio.isMusicPaused(), "suspension preserves logical state");
        audio.togglePauseMusic(); audio.testFinishAllSounds();
        require(buffer->paused && audio.isMusicPaused(), "logical pause survives voice release");
        audio.togglePauseMusic();
        require(!buffer->paused, "resume after release");
        require(audio.testPlayLogicalSound(2), "second suspension");
        audio.togglePauseMusic(); audio.togglePauseMusic();
        require(buffer->paused, "logical resume waits for effect");
        audio.testFinishAllSounds(); require(!buffer->paused, "release resumes backend");
        audio.stopMusic(); audio.stopMusic(); audio.togglePauseMusic();
        require(streams == 0 && !audio.isMusicActive(), "repeated stop is harmless");
        failLoad = true; audio.playMusic(score(1));
        require(!audio.isMusicActive() && streams == 0, "failed allocation leaves no music");
        failLoad = false; audio.playMusic(score(1, true)); audio.shutdown(); audio.shutdown();
        require(streams == 0 && !audio.isMusicActive(), "shutdown releases stream once");
        std::cout << "[PASS] stream lifecycle, drain, underrun, loops, voices and failures\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "[FAIL] " << error.what() << '\n'; return 1;
    }
}
