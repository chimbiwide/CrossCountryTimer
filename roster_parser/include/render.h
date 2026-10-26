#ifndef RENDER_H
#define RENDER_H

#include <stdbool.h>

#define MAX_CROPS 16        // strips a page can be cut into

typedef struct {
    int width;
    int height;
    unsigned char *rgb;     // width * height * 3 bytes
} Crop;

typedef struct {
    void *document;
    void *data;             // the file's bytes, pdfium reads them until render_close()
    int pages;
} Pdf;

bool render_open(Pdf *pdf, const char *path);
int render_page(Pdf *pdf, int page, int dpi, Crop crops[], int max_crops);
void render_close(Pdf *pdf);
void crop_free(Crop *crop);

#endif
