#include "SDL3/SDL_audio.h"
#include <transcribe.h>

const unsigned char whisper_tiny[] = {
    #embed "../models/ggml-tiny.en.bin"
};

const size_t whisper_tiny_size = sizeof(whisper_tiny);


void start_recording(SDL_AudioStream *stream) {
    SDL_ClearAudioStream(stream);
    SDL_ResumeAudioStreamDevice(stream);
}

int record(SDL_AudioStream *stream, float *buffer, int len) {
    int received = SDL_GetAudioStreamData(stream, buffer, len * sizeof(float));
    if (received >= 0) {
        return received / sizeof(float);
    }
    else return -1;
}

void pause_recording(SDL_AudioStream *stream) {
    SDL_PauseAudioStreamDevice(stream);
}

int transcribe(void *data) {
    return 0;
}
