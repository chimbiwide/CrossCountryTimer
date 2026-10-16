#ifndef TRANSCRIBE_H
#define TRANSCRIBE_H

#include <stdio.h>
#include <SDL3/SDL.h>
#include <whisper.h>

#define BUFF_SIZE (16000*10)

typedef struct {
    struct whisper_context *context;
    struct whisper_full_params *params;
    float *buffer;
    int buffer_size;
    int result;
} Audio_data;

extern const unsigned char whisper_tiny[];
extern const size_t whisper_tiny_size;

void start_recording(SDL_AudioStream *stream);
int record(SDL_AudioStream *stream, float *buffer, int len);
void pause_recording(SDL_AudioStream *stream);

void set_grammar(struct whisper_full_params *params);
int transcribe(void *data);

#endif
