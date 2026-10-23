#include <render.h>
#include <fpdfview.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The OCR model reads small images at full detail, and its work grows with
// the square of an image's size. A bigger image is slower per pixel, and one
// big enough gets scaled down until the "lll" in a name comes out as "ll".
#define MAX_PIXELS 300000

bool render_open(Pdf *pdf, const char *path) {
    memset(pdf, 0, sizeof(*pdf));

    FILE *file = fopen(path, "rb");
    if (file == NULL) return false;
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size <= 0) {
        fclose(file);
        return false;
    }
    pdf->data = malloc((size_t)size);
    size_t got = 0;
    if (pdf->data != NULL) got = fread(pdf->data, 1, (size_t)size, file);
    fclose(file);
    if (got != (size_t)size) {
        render_close(pdf);
        return false;
    }

    FPDF_InitLibrary();
    pdf->document = FPDF_LoadMemDocument64(pdf->data, (size_t)size, NULL);
    if (pdf->document == NULL) {
        render_close(pdf);
        return false;
    }
    pdf->pages = FPDF_GetPageCount(pdf->document);
    return true;
}

void render_close(Pdf *pdf) {
    if (pdf->document != NULL) FPDF_CloseDocument(pdf->document);
    free(pdf->data);
    memset(pdf, 0, sizeof(*pdf));
}

void crop_free(Crop *crop) {
    free(crop->rgb);
    memset(crop, 0, sizeof(*crop));
}

// pdfium bitmaps are BGRx, 4 bytes a pixel
static unsigned char *pixel(unsigned char *pixels, int stride, int x, int y) {
    return pixels + (size_t)y * stride + (size_t)x * 4;
}

static bool is_ink(unsigned char *pixels, int stride, int x, int y, int threshold) {
    const unsigned char *p = pixel(pixels, stride, x, y);
    return (p[0] + p[1] + p[2]) / 3 < threshold;
}

static bool is_rule(unsigned char *pixels, int stride, int width, int y) {
    int dark = 0;
    for (int x = 0; x < width; x++) {
        if (is_ink(pixels, stride, x, y, 128)) dark++;
    }
    return dark * 10 > width * 7;
}

// The page header (who the meet software is licensed to, the title, the
// column names) ends with a line across the page. Everything above it is
// skipped, so "Licensed To: ... High School" never reads as a school.
static int header_end(unsigned char *pixels, int stride, int width, int height) {
    for (int y = 0; y < height; y++) {
        if (!is_rule(pixels, stride, width, y)) continue;
        while (y < height && is_rule(pixels, stride, width, y)) y++;
        return y;
    }
    return 0;
}

// Paints out vertical lines (the one between the two columns): any dark run
// at least min_length long. Letters are never that tall.
static void erase_vertical_lines(unsigned char *pixels, int stride, int width, int top, int height,
                                 int min_length) {
    for (int x = 0; x < width; x++) {
        int y = top;
        while (y < height) {
            if (!is_ink(pixels, stride, x, y, 128)) {
                y++;
                continue;
            }
            int end = y;
            while (end < height && is_ink(pixels, stride, x, end, 128)) end++;
            if (end - y >= min_length) {
                for (int i = y; i < end; i++) memset(pixel(pixels, stride, x, i), 255, 4);
            }
            y = end;
        }
    }
}

// Copies the pixels at the kept columns xs and rows ys into a new RGB crop.
static bool copy_crop(unsigned char *pixels, int stride, const int *xs, int width, const int *ys, int height,
                      Crop *out) {
    out->width = width;
    out->height = height;
    out->rgb = malloc((size_t)width * height * 3);
    if (out->rgb == NULL) return false;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const unsigned char *p = pixel(pixels, stride, xs[x], ys[y]);
            unsigned char *q = out->rgb + ((size_t)y * width + x) * 3;
            q[0] = p[2];
            q[1] = p[1];
            q[2] = p[0];
        }
    }
    return true;
}

// A roster is mostly white: a wide gap between the names and the bib column,
// and space between the lines. The model's time grows with the pixels it
// reads, so each blank run between inked positions longer than gap is cut
// down to gap (half from each side). The margins at the two ends stay whole:
// they are small already, and the model scales a strip of one short line
// down further. ink[i] says whether position first + i has ink. The kept
// positions go to out; returns how many there are.
static int squeeze(const bool *ink, int count, int first, int gap, int *out) {
    int kept = 0;
    int i = 0;
    while (i < count) {
        if (ink[i]) {
            out[kept++] = first + i;
            i++;
            continue;
        }
        int end = i;
        while (end < count && !ink[end]) end++;
        int head = end - i;
        int tail = 0;
        if (head > gap && i > 0 && end < count) {
            head = gap / 2;
            tail = gap - head;
        }
        for (int k = 0; k < head; k++) out[kept++] = first + i + k;
        for (int k = 0; k < tail; k++) out[kept++] = first + end - tail + k;
        i = end;
    }
    return kept;
}

// Cuts the inked part of one column into strips of at most MAX_PIXELS, top to
// bottom, with the blank space squeezed out. Each cut is moved up to a blank
// row so no line of text is split. Returns how many strips were added to
// crops, -1 when they don't fit.
static int cut_column(unsigned char *pixels, int stride, int x0, int y0, int x1, int y1, int dpi,
                      Crop *crops, int room) {
    int left = x1;
    int right = -1;
    int top = y1;
    int bottom = -1;
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            if (!is_ink(pixels, stride, x, y, 200)) continue;
            if (x < left) left = x;
            if (x > right) right = x;
            if (y < top) top = y;
            if (y > bottom) bottom = y;
        }
    }
    if (right < 0) return 0;

    int margin = dpi / 12;
    left -= margin;
    top -= margin;
    right += margin;
    bottom += margin;
    if (left < x0) left = x0;
    if (top < y0) top = y0;
    if (right > x1 - 1) right = x1 - 1;
    if (bottom > y1 - 1) bottom = y1 - 1;

    int width = right - left + 1;
    int height = bottom - top + 1;
    bool *ink_x = calloc((size_t)width, sizeof(bool));
    bool *ink_y = calloc((size_t)height, sizeof(bool));
    int *xs = malloc((size_t)width * sizeof(int));
    int *ys = malloc((size_t)height * sizeof(int));
    int count = -1;
    if (ink_x != NULL && ink_y != NULL && xs != NULL && ys != NULL) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (!is_ink(pixels, stride, left + x, top + y, 200)) continue;
                ink_x[x] = true;
                ink_y[y] = true;
            }
        }
        // keeps the space between columns and words, and a few blank rows between lines
        int kept_x = squeeze(ink_x, width, left, dpi / 8, xs);
        int kept_y = squeeze(ink_y, height, top, dpi / 25, ys);

        int strips = 1;
        while ((long long)kept_x * ((kept_y + strips - 1) / strips) > MAX_PIXELS) strips++;
        if (strips <= room) {
            count = strips;
            int start = 0;
            for (int i = 1; i <= strips && count > 0; i++) {
                int end = kept_y - 1;
                if (i < strips) {
                    end = kept_y * i / strips;
                    while (end > start && ink_y[ys[end] - top]) end--;
                }
                if (!copy_crop(pixels, stride, xs, kept_x, ys + start, end - start + 1, &crops[i - 1])) count = -1;
                start = end + 1;
            }
        }
    }
    free(ink_x);
    free(ink_y);
    free(xs);
    free(ys);
    return count;
}

// Renders a page and cuts it into strips the model reads one at a time: the
// left column top to bottom, then the right one, all below the header.
// crops must start zeroed. Returns how many strips there are (0 for an empty
// page), or -1 on error with nothing left allocated.
int render_page(Pdf *pdf, int page_index, int dpi, Crop crops[], int max_crops) {
    FPDF_PAGE page = FPDF_LoadPage(pdf->document, page_index);
    if (page == NULL) return -1;

    int width = (int)(FPDF_GetPageWidthF(page) * dpi / 72.0f);
    int height = (int)(FPDF_GetPageHeightF(page) * dpi / 72.0f);
    FPDF_BITMAP bitmap = FPDFBitmap_Create(width, height, 0);
    if (bitmap == NULL) {
        FPDF_ClosePage(page);
        return -1;
    }
    FPDFBitmap_FillRect(bitmap, 0, 0, width, height, 0xFFFFFFFF);
    FPDF_RenderPageBitmap(bitmap, page, 0, 0, width, height, 0, FPDF_ANNOT);

    unsigned char *pixels = FPDFBitmap_GetBuffer(bitmap);
    int stride = FPDFBitmap_GetStride(bitmap);
    int top = header_end(pixels, stride, width, height);
    erase_vertical_lines(pixels, stride, width, top, height, dpi / 2);

    int middle = width / 2;
    int count = cut_column(pixels, stride, 0, top, middle, height, dpi, crops, max_crops);
    if (count >= 0) {
        int right = cut_column(pixels, stride, middle, top, width, height, dpi,
                               crops + count, max_crops - count);
        if (right < 0) count = -1;
        else count += right;
    }
    if (count < 0) {
        for (int i = 0; i < max_crops; i++) crop_free(&crops[i]);
    }

    FPDFBitmap_Destroy(bitmap);
    FPDF_ClosePage(page);
    return count;
}
