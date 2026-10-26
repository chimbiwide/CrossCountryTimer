#ifndef RENDER_H
#define RENDER_H

#include <stdbool.h>

#define MAX_CROPS 16

typedef struct {
    int width;
    int height;
    unsigned char *rgb;
} Crop;

typedef struct {
    void *document;
    void *data;
    int pages;
} Pdf;

bool render_open(Pdf *pdf, const char *path);
int render_page(Pdf *pdf, int page, int dpi, Crop crops[], int max_crops);
void render_close(Pdf *pdf);
void crop_free(Crop *crop);

#endif
