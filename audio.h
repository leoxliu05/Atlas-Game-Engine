#ifndef AUDIO_H
#define AUDIO_H

#include "SDL2_mixer/SDL_mixer.h"
#include <string>
#include <unordered_map>

class audio {
public:
    static bool Initialize(int frequency = 44100,
                           Uint16 format = MIX_DEFAULT_FORMAT,
                           int channels = 2,
                           int chunksize = 2048,
                           int allocated_channels = 50);
    static void Shutdown();

    static void Play(int channel, const std::string& clip_name, bool does_loop); // api
    static void Halt(int channel); // api
    static void SetVolume(int channel, float volume); // api

private:
    static Mix_Chunk* LoadClip(const std::string& clip_name);

    static inline std::unordered_map<std::string, Mix_Chunk*> clip_cache;
};

#endif
