#include "raylib.h"
#include <csv.h>
#include <time.h>

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
    Font font = LoadFontEx("data/Google-Sans-Mono-Regular.ttf", 125, NULL, 0);
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

    float listW = 500;
    float listH = 350;
    float listX = (((float)GetScreenWidth() - listW) / 2.0f);
    float listY = 350;

    FILE *file;

    init_csv(file);

    while (!WindowShouldClose()) {
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
        if (GuiButton((Rectangle){370, 180, 120, 50}, "Start") && !timerStarted) {
            timerStarted = true;
            if (!stopped) start_timer(&timer, GetTime());
            else {
                stopped = false;
                resume_timer(&timer, GetTime());
            }
        }
        // lap button
        if (GuiButton((Rectangle){510, 180, 120, 50}, "Lap") && timerStarted) {
            // write the time to the buffer
            write_time(&time, lap_text[lap_count], 128, lap_count);
            // copy the pointer of lap_text to laps
            laps[lap_count] = lap_text[lap_count];
            lap_count++;
            write_csv(file, &time, lap_count);
        }

        // stop button
        if (GuiButton((Rectangle){650, 180, 120, 50}, "Stop") && timerStarted) {
            stopped = true;
            pause_timer(&timer, GetTime());
            timerStarted = false;
        }

        // reset button
        if (GuiButton((Rectangle){790, 180, 120, 50}, "Reset") && !timerStarted) {
            stopped = false;
            end_timer(&timer);
            reset_time(&time);
        }

        // the grid list
        if (lap_count > 11) scroll = lap_count - 11;
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
