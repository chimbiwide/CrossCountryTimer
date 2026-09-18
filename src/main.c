#include "raylib.h"
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

    // timer related stuff
    bool timerStarted = false;
    bool stopped = false;
    Timer timer = {false, 0.0};
    Time time = {0,0,0,0,0};

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // the timer
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
        GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, ColorToInt(BLACK));
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
            else stopped = false;
        }
        // lap button
        if (GuiButton((Rectangle){510, 180, 120, 50}, "Lap") && timerStarted) {
            // prints the curent time to the textbox
        }

        // stop button
        if (GuiButton((Rectangle){650, 180, 120, 50}, "Stop") && timerStarted) {
            stopped = true;
            timerStarted = false;
        }

        // reset button
        if (GuiButton((Rectangle){790, 180, 120, 50}, "Reset") && !timerStarted) {
            stopped = false;
            end_timer(&timer);
            reset_time(&time);
        }

        // read the time after every frame
        if (timerStarted) read_time(&timer, &time, GetTime());
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
