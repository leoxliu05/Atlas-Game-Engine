#include "audio.h"
#include "Helper.h"
#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace {
bool is_autograder_mode() {
#ifdef _WIN32
    char* val = nullptr;
    size_t length = 0;
    _dupenv_s(&val, &length, "AUTOGRADER");
    if (val != nullptr) {
        free(val);
        return true;
    }
    return false;
#else
    return std::getenv("AUTOGRADER") != nullptr;
#endif
}

Mix_Chunk* mix_load_wav(const char* file) {
    if (!is_autograder_mode())
        return ::Mix_LoadWAV(file);

    static Mix_Chunk autograder_dummy_sound;
    if (std::filesystem::exists(file))
        return &autograder_dummy_sound;
    return nullptr;
}

int mix_play_channel(int channel, Mix_Chunk* chunk, int loops) {
    std::cout << "(Mix_PlayChannel(" << channel << ",?," << loops
              << ") called on frame " << Helper::GetFrameNumber() << ")" << std::endl;

    if (!is_autograder_mode())
        return ::Mix_PlayChannel(channel, chunk, loops);
    return channel;
}

int mix_open_audio(int frequency, Uint16 format, int channels, int chunksize) {
    if (!is_autograder_mode())
        return ::Mix_OpenAudio(frequency, format, channels, chunksize);
    return 0;
}

int mix_allocate_channels(int numchans) {
    if (!is_autograder_mode())
        return ::Mix_AllocateChannels(numchans);
    return numchans;
}

int mix_halt_channel(int channel) {
    std::cout << "(Mix_HaltChannel(" << channel << ") called on frame "
              << Helper::GetFrameNumber() << ")" << std::endl;

    if (!is_autograder_mode())
        return ::Mix_HaltChannel(channel);
    return 0;
}

int mix_volume(int channel, int volume) {
    std::cout << "(Mix_Volume(" << channel << "," << volume << ") called on frame "
              << Helper::GetFrameNumber() << ")" << std::endl;

    if (!is_autograder_mode())
        return ::Mix_Volume(channel, volume);
    return 0;
}

void mix_close_audio() {
    if (!is_autograder_mode())
        ::Mix_CloseAudio();
}
}

bool audio::Initialize(int frequency, Uint16 format, int channels, int chunksize, int allocated_channels) {
    if (mix_open_audio(frequency, format, channels, chunksize) != 0)
        return false;
    return mix_allocate_channels(allocated_channels) >= 0;
}

void audio::Shutdown() {
    const bool autograder_mode = is_autograder_mode();
    for (auto& entry : clip_cache) {
        if (entry.second != nullptr && !autograder_mode)
            Mix_FreeChunk(entry.second);
    }
    clip_cache.clear();
    mix_close_audio();
}

void audio::Play(int channel, const std::string& clip_name, bool does_loop) {
    if (clip_name.empty())
        return;

    Mix_Chunk* chunk = LoadClip(clip_name);
    if (chunk == nullptr) {
        std::cout << "error: failed to play audio clip " << clip_name;
        exit(0);
    }

    const int loops = does_loop ? -1 : 0;
    mix_play_channel(channel, chunk, loops);
}

void audio::Halt(int channel) {
    mix_halt_channel(channel);
}

void audio::SetVolume(int channel, float volume) {
    mix_volume(channel, static_cast<int>(volume));
}

Mix_Chunk* audio::LoadClip(const std::string& clip_name) {
    auto it = clip_cache.find(clip_name);
    if (it != clip_cache.end())
        return it->second;

    std::string audio_path = "resources/audio/" + clip_name + ".wav";
    if (!std::filesystem::exists(audio_path)) {
        audio_path = "resources/audio/" + clip_name + ".ogg";
        if (!std::filesystem::exists(audio_path))
            return nullptr;
    }

    Mix_Chunk* chunk = mix_load_wav(audio_path.c_str());
    if (chunk == nullptr)
        return nullptr;

    clip_cache.emplace(clip_name, chunk);
    return chunk;
}
