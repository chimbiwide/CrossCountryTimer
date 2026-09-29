#include "raylib.h"
#include <xlsxio_read.h>

#include <pdf.h>
#include <xlsx.h>
#include <timer.h>
#include <font.h>
#include <data.h>


#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define CAPACITY 2048
#define MAX_RUNNER 200

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

    // styling
    GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, ColorToInt(BLACK));
    GuiSetStyle(LISTVIEW, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(LISTVIEW, LIST_ITEMS_HEIGHT, 40);

    // timer related stuff
    bool timerStarted = false;
    bool stopped = false;
    Timer timer = {false, 0.0};
    Time time = {0,0,0,0,0};

    // Lists
    static char lap_text[MAX_RUNNER][256];
    static char *laps[MAX_RUNNER];
    static int lap_count;
    static int scroll;
    static int active;
    static int focused;

    static Time lap_times[MAX_RUNNER];
    static Student students[MAX_RUNNER];
    static Row rows[MAX_RUNNER] = {0};

    // student index
    Student roster[CAPACITY] = {0};

    int runners = 0;
    xlsxioreader reader;
    if ((reader = xlsxioread_open("data/index.xlsx")) == NULL) 
        printf("Error opening xlsx file\n");
    else {
        runners = read_lookup(reader, "Student Lookup", roster, CAPACITY);
        xlsxioread_close(reader);
    }

    // bib text. currentIndex is the lap row the next bib is written onto.
    int bib = 0;
    int currentIndex = 0;
    bool bibEdit = false;

    // divison selector
    // 0 vasity, 1 JV, 2 Freshman, 3 Middle School
    static int division = 0;
    static bool divisionEdit = false;

    // edit mode checkbox
    bool edit_mode = false;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
        //list coordinates
        float listW = GetScreenWidth() * 0.75f;
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
        Rectangle bibBox = {GetScreenWidth() / 2.0f-200, listY - 50, 160, 40};
        Rectangle editBox = {(GetScreenWidth() / 2.0f)+50, listY - 50, 250, 40};

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
        // lap button. Name and school stay blank until a bib is entered for this row.
        if (GuiButton(lapB, "Lap") && timerStarted && lap_count < MAX_RUNNER) {
            Student none = {0};
            lap_times[lap_count] = time;
            write_row(&lap_times[lap_count], &none, lap_text[lap_count], 256, lap_count);
            laps[lap_count] = lap_text[lap_count];
            lap_count++;

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
            currentIndex = 0;
            bib = 0;
            for (int i = 0; i < MAX_RUNNER; i++) students[i] = (Student){0};

            scroll = 0;
            active = -1;
            focused = -1;
        }

        // bib value box
        if (divisionEdit) GuiLock();
        if (GuiValueBox(bibBox, "Enter Bib Number", &bib, 0, 9999, bibEdit)) {
            int row = currentIndex;
            if (edit_mode) row = active;
            if (bibEdit && bib > 0 && row >= 0 && row < lap_count) {
                Student who = {0};
                who.bib = bib;
                int found = search_name(roster, runners, bib);
                if (found >= 0) who = roster[found];
                write_student(&who, &students[row]);
                write_row(&lap_times[row], &who, lap_text[row], 256, row);
                if (!edit_mode) {
                    active = currentIndex;
                    currentIndex++;
                }
            }
            else bib = 0;
            bibEdit = !bibEdit;
        }
        // the grid list
        GuiListViewEx((Rectangle){listX, listY, listW, listH},
                      laps, lap_count, &scroll, &active, &focused);

        // the edit mode toggle
        const char *editLabel = "Edit Mode: OFF";
        if (edit_mode) editLabel = "Edit Mode: ON";
        GuiToggle(editBox, editLabel, &edit_mode);
        if (divisionEdit) GuiUnlock();


        // Drawn last so the open menu stays above the bib box and the lap list.
        int divisionW = GuiGetTextWidth("Middle School") + 62;
        if (divisionW < butW) divisionW = butW;
        Rectangle divisionB = {x, butY+butH+gap, (float)divisionW, (float)butH};
        if (GuiDropdownBox(divisionB, "Varsity;JV;Freshman;Middle School", &division, divisionEdit))
            divisionEdit = !divisionEdit;

        // Export PDF
        int pdfW = GuiGetTextWidth("Export to PDF") + 64;
        Rectangle pdfB = {x+(float)divisionW+gap, butY+butH+gap, (float)pdfW, (float)butH};
        if (GuiButton(pdfB, "Export to PDF") && !timerStarted) {
            if (lap_count > 0) {
                sync_row(rows, lap_times, students, lap_count);

                char winner[20] = "";
                char loser[20] = "";
                int winner_score = 0;
                int loser_score = 0;
                calcuate_score(rows, lap_count, winner, &winner_score, loser, &loser_score);

                const char *division_name = "Varsity";
                if (division == 1) division_name = "JV";
                if (division == 2) division_name = "Freshman";
                if (division == 3) division_name = "Middle School";

                if (write_results_pdf("results.pdf", rows, lap_count,
                                      winner, winner_score,
                                      loser, loser_score,
                                      division_name) < 0) {
                    printf("PDF save failed\n");
                }
            }
        }
        // read the time after every frame
        if (timerStarted) read_time(&timer, &time, GetTime());
        EndDrawing();
    }

    UnloadFont(font);
    CloseWindow();
    return 0;
}
