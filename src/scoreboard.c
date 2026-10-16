#include <scoreboard.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <raylib.h>
#include <raygui.h>
#include <font.h>
#include <xlsx.h>
#include <stdio.h>

#define ROWS_PER_PAGE 15

static void fit_rows(Rectangle bounds, char **rows, int count, int perPage) {
    int border = GuiGetStyle(DEFAULT, BORDER_WIDTH);
    int spacing = GuiGetStyle(LISTVIEW, LIST_ITEMS_SPACING);

    int itemH = ((int)bounds.height - 2 * border) / perPage - spacing;
    if (itemH < 1) itemH = 1;
    GuiSetStyle(LISTVIEW, LIST_ITEMS_HEIGHT, itemH);

    int textSize = itemH * 3 / 4;
    if (textSize < 1) textSize = 1;
    GuiSetStyle(DEFAULT, TEXT_SIZE, textSize);

    float textW = bounds.width - 2 * spacing - border
                  - 2 * GuiGetStyle(LISTVIEW, BORDER_WIDTH) - 2 * GuiGetStyle(LISTVIEW, TEXT_PADDING);
    if (count > perPage) textW -= GuiGetStyle(LISTVIEW, SCROLLBAR_WIDTH);

    int widest = -1;
    int widestW = 0;
    for (int i = 0; i < count; i++) {
        int w = GuiGetTextWidth(rows[i]);
        if (w > widestW) {
            widestW = w;
            widest = i;
        }
    }
    if (widest < 0 || widestW <= textW) return;

    textSize = (int)(textSize * textW / widestW);
    if (textSize < 1) textSize = 1;
    GuiSetStyle(DEFAULT, TEXT_SIZE, textSize);
    while (textSize > 1 && GuiGetTextWidth(rows[widest]) > textW) {
        textSize--;
        GuiSetStyle(DEFAULT, TEXT_SIZE, textSize);
    }
}

int init_score_file(const char *path) {
    FILE *file = fopen(path, "w");
    if (file == NULL) return -1;
    fprintf(file, "%d\n", 0);
    fclose(file);
    return 0;
}

int write_laps(const char *path, int lap_count, char *laps[]) {
    FILE *file = fopen(path, "w");
    if (file == NULL) return -1;

    fprintf(file, "%d\n", lap_count);
    
    for (int i = 0; i < lap_count; i++) {
        if (fprintf(file, "%s\n", laps[i]) < 0) {
            fclose(file);
            return -1;
        }
    }
    fclose(file);
    return lap_count;
}

static int read_laps(char *path, int row_size, char *laps[]) {
    FILE *file = fopen(path, "r");
    if (file == NULL) return -1;

    char count_text[10];
    int count = 0;
    if (fgets(count_text, sizeof(count_text), file) == NULL) goto cleanup;
    count = atoi(count_text);

    int read_lines = 0;
    while (read_lines < MAX_RUNNER && fgets(laps[read_lines], row_size, file) != NULL) {
        size_t len = strlen(laps[read_lines]);
        if (len > 0 && laps[read_lines][len-1] == '\n') {
            laps[read_lines][len-1] = '\0';
        }
        read_lines++;
    }
    if (read_lines < count) goto cleanup;
    fclose(file);
    return read_lines;
cleanup: 
    fclose(file);
    return -1;
}

int scoreboard(void) {
    // initialization
    int scWidth = 1280;
    int scHeight = 720;
    // resizable window
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(scWidth, scHeight, "Scoreboard");
    SetTargetFPS(60);

    // fonts
    Font font = LoadFontFromMemory(".ttf", google_sans, google_sans_size, 125, NULL, 0);
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    GuiSetFont(font);

    GuiSetStyle(LISTVIEW, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);

    // Timer
    double last_read_time = 0.0;
    double last_scroll_time = 0.0;
    bool ready_score = true;

    static char lap_text[MAX_RUNNER][256];
    static char *laps[MAX_RUNNER];
    static int lap_count;
    static int scroll;
    static int active = -1;
    static int focused;
    for (int i = 0; i < MAX_RUNNER; i++) laps[i] = lap_text[i];

    while (!WindowShouldClose()) {
        Rectangle listB = {0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()};
        fit_rows(listB, laps, lap_count, ROWS_PER_PAGE);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        // re-read every half a second
        if (ready_score) {
            int result = read_laps(SCORE_PATH, 256, laps);
            if (result > -1) lap_count = result;
        }
        if ((GetTime() - last_read_time) > 0.5) {
            ready_score = true;
            last_read_time = GetTime();
        }
        else ready_score = false;

        if ((GetTime() - last_scroll_time) > 1) {
            active++;
            last_scroll_time = GetTime();
            if (active >= lap_count) {
                active = 0;
            }
            if (active % ROWS_PER_PAGE == 0) {
                scroll = active;
                int last_page = lap_count - ROWS_PER_PAGE;
                if (last_page < 0) last_page = 0;
                if (scroll > last_page) scroll = last_page;
            }
        }

        GuiListViewEx(listB, laps, lap_count, &scroll, &active, &focused);

        EndDrawing();
    }

    UnloadFont(font);
    CloseWindow();
    return 0;
}
