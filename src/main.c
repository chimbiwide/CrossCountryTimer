#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

int main(void)
{
    InitWindow(1280, 720, "Cross Country Timer");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        GuiLabel((Rectangle){ 20, 20, 400, 24 }, "Cross Country Timer");
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
