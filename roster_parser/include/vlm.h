#ifndef VLM_H
#define VLM_H

#include <render.h>
#include <llama.h>
#include <mtmd.h>
#include <stdatomic.h>
#include <stdbool.h>

// Strips the model reads at the same time
#define READ_GROUP 4

typedef struct {
    struct llama_model *model;
    struct llama_context *context;
    mtmd_context *vision;
    const struct llama_vocab *vocab;
    int vocab_size;
    struct llama_batch batch;
    char prompt[256];
    float lowest_lead[READ_GROUP];  // per strip of the last read: the least certain token, how far it beat the runner-up, in logits
    const atomic_bool *stop;    // another thread sets it to cut a read short
} Vlm;

int vlm_threads(void);
bool vlm_load(Vlm *vlm, const char *model_path, const char *mmproj_path, int threads);
int vlm_read(Vlm *vlm, const Crop *crops, int count, char *texts[], int size);
void vlm_free(Vlm *vlm);

#endif
