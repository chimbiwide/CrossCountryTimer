#include "SDL3/SDL_audio.h"
#include <transcribe.h>

const unsigned char whisper_tiny[] = {
    #embed "../models/ggml-tiny.en.bin"
};

const size_t whisper_tiny_size = sizeof(whisper_tiny);

// 1 to 4 digits, the bib box's range. As GBNF text:
//   root      ::= opt_space [0-9] opt_digit opt_digit opt_digit
//   opt_space ::= " " |
//   opt_digit ::= [0-9] |
// whisper's text usually starts with a space (" 1042"), so the root allows one.
static const whisper_grammar_element bib_root[] = {
    {WHISPER_GRETYPE_RULE_REF, 1},
    {WHISPER_GRETYPE_CHAR, '0'}, {WHISPER_GRETYPE_CHAR_RNG_UPPER, '9'},
    {WHISPER_GRETYPE_RULE_REF, 2},
    {WHISPER_GRETYPE_RULE_REF, 2},
    {WHISPER_GRETYPE_RULE_REF, 2},
    {WHISPER_GRETYPE_END, 0},
};
static const whisper_grammar_element bib_space[] = {
    {WHISPER_GRETYPE_CHAR, ' '},
    {WHISPER_GRETYPE_ALT, 0},
    {WHISPER_GRETYPE_END, 0},
};
static const whisper_grammar_element bib_digit[] = {
    {WHISPER_GRETYPE_CHAR, '0'}, {WHISPER_GRETYPE_CHAR_RNG_UPPER, '9'},
    {WHISPER_GRETYPE_ALT, 0},
    {WHISPER_GRETYPE_END, 0},
};
// RULE_REF values are indexes into this array
static const whisper_grammar_element *bib_rules[] = {bib_root, bib_space, bib_digit};

void set_grammar(struct whisper_full_params *params) {
    params->grammar_rules = bib_rules;
    params->n_grammar_rules = sizeof(bib_rules) / sizeof(bib_rules[0]);
    params->i_start_rule = 0;
    params->grammar_penalty = 100.0f;
}

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
