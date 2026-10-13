#include <scoreboard.h>
#include <stddef.h>
#include <raylib.h>
#include <raygui.h>
#include <font.h>
#include <xlsx.h>
#include <stdio.h>

#define ROWS_PER_PAGE 10

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

int write_laps(char *path, int lap_count, char *laps) {
    FILE *file = fopen(path, "w");
    if (file == NULL) return -1;

    return 0;
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

    // Lists. Every pointer is set up front, so filling lap_text and
    // lap_count is all it takes to show rows.
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

        GuiListViewEx(listB, laps, lap_count, &scroll, &active, &focused);

        EndDrawing();
    }

    UnloadFont(font);
    CloseWindow();
    return 0;
}
