#include "raylib.h"
#include <csv.h>
#include <time.h>
#include <font.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

int main(void)
{
    // initialization
    int scWidth = 1280;
    int scHeight = 720;
    InitWindow(scWidth, scHeight, "Cross Country Timer");
    SetTargetFPS(60);

    // fonts
    Font font = LoadFontFromMemory(".ttf", google_sans, google_sans_size, 125, NULL, 0);
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    GuiSetFont(font);

    GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, ColorToInt(BLACK));

    // timer related stuff
    bool timerStarted = false;
    bool stopped = false;
    Timer timer = {false, 0.0};
    Time time = {0,0,0,0,0};

    // Lists
    static char lap_text[100][128];
    static char *laps[100];
    static int lap_count;
    static int scroll;
    static int active;
    static int focused;

    FILE *file;
    init_csv(file);

    Student roster[1024];
    FILE *lookup = fopen("data/lookup.csv", "r");
    if (lookup == NULL) printf("Error opening file\n");
    else {
        int runners = read_lookup(lookup, roster, 1024);
        fclose(lookup);
    }

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
        //list coordinates
        float listW = GetScreenWidth() / 2.0f;
        float listH = GetScreenHeight() / 2.0f;
        float listX = (((float)GetScreenWidth() - listW) / 2.0f);
        float listY = GetScreenHeight() / 2.0f;

        int butW = 120;
        int butH = 50;
        float gap = 20;
        float rowW = 4 * butW + 3 * gap;
        float x = ((float)GetScreenWidth() - rowW) / 2.0f;
        int butY = 180;

        Rectangle startB = {x, butY, butW, butH};
        Rectangle lapB = {x+(butW + gap), butY, butW, butH};
        Rectangle stopB = {x+2*(butW + gap), butY, butW, butH};
        Rectangle resetB = {x+3*(butW + gap), butY, butW, butH};

        BeginDrawing();
        ClearBackground(RAYWHITE);

        // the timer
        GuiSetStyle(DEFAULT, TEXT_SIZE, 125);
        GuiLabel((Rectangle){ 0, 50, (float)GetScreenWidth(), 125 }, 
                 TextFormat("%02d:%02d:%02d.%d", 
                            time.h,
                            time.min, 
                            time.s, 
                            time.pre_ms)
                 );
        GuiSetStyle(DEFAULT, TEXT_SIZE, 30);

        // the start button
        if (GuiButton(startB, "Start") && !timerStarted) {
            timerStarted = true;
            if (!stopped) start_timer(&timer, GetTime());
            else {
                stopped = false;
                resume_timer(&timer, GetTime());
            }
        }
        // lap button
        if (GuiButton(lapB, "Lap") && timerStarted) {
            // write the time to the buffer
            write_time(&time, lap_text[lap_count], 128, lap_count);
            // copy the pointer of lap_text to laps
            laps[lap_count] = lap_text[lap_count];
            lap_count++;
            write_csv(file, &time, lap_count);

            // scroll to bottom once per update
            int row = GuiGetStyle(LISTVIEW, LIST_ITEMS_HEIGHT) + GuiGetStyle(LISTVIEW, LIST_ITEMS_SPACING);
            int visible = (int)listH / row;
            if ((visible) < 1) visible = 1;
            if (lap_count > visible) scroll = lap_count - visible;
            else scroll = 0;
        }

        // stop button
        if (GuiButton(stopB, "Stop") && timerStarted) {
            stopped = true;
            pause_timer(&timer, GetTime());
            timerStarted = false;
        }

        // reset button
        if (GuiButton(resetB, "Reset") && !timerStarted) {
            stopped = false;
            end_timer(&timer);
            reset_time(&time);

            lap_count = 0;
            scroll = 0;
            active = -1;
            focused = -1;
        }

        // the grid list
        GuiListViewEx((Rectangle){listX, listY, listW, listH},
                      laps, lap_count, &scroll, &active, &focused);

        // read the time after every frame
        if (timerStarted) read_time(&timer, &time, GetTime());
        EndDrawing();
    }

    UnloadFont(font);
    CloseWindow();
    return 0;
}
