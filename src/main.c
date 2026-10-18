#include "SDL3/SDL_audio.h"
#include "SDL3/SDL_filesystem.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_thread.h"
#include "whisper.h"
#include <stdio.h>
#include <stdbool.h>
#define RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT 44
#define RAYGUI_WINDOWBOX_CLOSEBUTTON_HEIGHT 30
#include "raylib.h"
#include <xlsxio_read.h>
#include <SDL3/SDL.h>

#include <pdf.h>
#include <xlsx.h>
#include <timer.h>
#include <font.h>
#include <data.h>
#include <scoreboard.h>
#include <transcribe.h>
#include <gamepad.h>

#define XBOX_ALIAS_1 "xbox"
#define XBOX_ALIAS_2 "x-box"
#define PS_ALIAS_1   "playstation"
#define PS_ALIAS_2   "sony"


#define GUI_SCROLL_DELTA GetMouseWheelMove()
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define CAPACITY 2048

static int GuiValueBoxUnset(Rectangle bounds, const char *text, int *value, int minValue, int maxValue, bool editMode)
{
    if (*value >= 0 || editMode) return GuiValueBox(bounds, text, value, minValue, maxValue, editMode);

    char unknown[] = CLIMATE_UNKNOWN;
    float unused = 0.0f;
    int result = GuiValueBoxFloat(bounds, text, unknown, &unused, false);
    if (result) *value = 0;
    return result;
}

int main(int argc, char *argv[])
{
    if (argc == 2 && (strcmp(argv[1], "--scoreboard") == 0)) {
        scoreboard();
        return 0;
    }
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
    int lap_count;
    int scroll;
    int active;
    int focused;

    static Time lap_times[MAX_RUNNER];
    static Student students[MAX_RUNNER];
    static Row rows[MAX_RUNNER] = {0};

    // student index
    static Student roster[CAPACITY] = {0};

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

    // scoreboard toggle
    bool boardEdit = false;
    bool boardStarted = false;
    bool boardStop = false;
    int boardExitCode;
    SDL_Process *scoreboard_process = NULL;

    // edit mode checkbox
    bool edit_mode = false;
    
    // settings box
    bool showSettings = false;

    Climate climate = {CLIMATE_UNKNOWN, CLIMATE_UNKNOWN, CLIMATE_UNKNOWN, CLIMATE_UNKNOWN, -1};
    float temp = 0.0;
    float precep = 0.0;
    int windDirection = 0;
    bool tempEdit = false;
    bool windEdit = false;
    bool directionEdit = false;
    bool cloudEdit = false;
    bool precepEdit = false;

    // gamepad
    int gamepad = 0;
    GamepadState padState;
    zero_state(&padState);

    // scoreboard
    char score_path[1024];
    snprintf(score_path, sizeof(score_path), "%sCrossCountryTimer", SDL_GetBasePath());
    const char *args[] = {score_path, "--scoreboard", NULL};
    init_score_file(SCORE_PATH);

    // whisper 
    struct whisper_context_params cparams = whisper_context_default_params();
    struct whisper_context *ctx = whisper_init_from_buffer_with_params((void *)whisper_tiny, whisper_tiny_size, cparams);
    if (ctx == NULL) printf("Failed to load whipser from memory\n");

    struct whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.print_progress = false;
    params.single_segment = true;
    params.no_timestamps = true;
    params.initial_prompt = "1042, 873, 2210, 1300, 907, 4015, 9057";
    set_grammar(&params);

    // allocate memory for the audio buffer
    float *audio_buffer = malloc(BUFF_SIZE * sizeof(float));
    int audio_buffer_size = 0;

    Audio_data data = {
        .context = ctx,
        .params = &params,
        .buffer = audio_buffer,
        .buffer_size = audio_buffer_size,
    };

    bool transcribing = false;
    SDL_Thread *whisper_thread = NULL;

    // audio
    bool recordingStarted = false;
    bool isRecording = false;
    bool stopRecording = false;
    bool isReady = false;
    SDL_InitSubSystem(SDL_INIT_AUDIO);
    SDL_AudioSpec spec = {
        .format = SDL_AUDIO_F32,
        .channels = 1,
        .freq = 16000,
    };
    SDL_AudioStream *mic = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_RECORDING, &spec, NULL, NULL);
    if (mic == NULL) printf("Failed to start audio device\n");

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();

        // spawn scoreboard
        if (boardEdit && !boardStarted) {
            scoreboard_process = SDL_CreateProcess(args, false);
            if (scoreboard_process == NULL) {
                boardEdit = false;
                boardStarted = false;
            } else {boardStarted = true;}
        }
        if (boardStarted && SDL_WaitProcess(scoreboard_process, false, &boardExitCode)) {
            SDL_DestroyProcess(scoreboard_process);
            scoreboard_process = NULL;
            boardEdit = false;
            boardStarted = false;
            boardStop = false;
        }
        if (boardEdit == false && boardStarted && !boardStop) {
            SDL_KillProcess(scoreboard_process, false);
            boardStop = true;
        }

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
        Rectangle settingsB = {10, 10, butH, butH};

        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (IsGamepadAvailable(gamepad)) {
            update_pad_state(gamepad, &padState);
            if (IsGamepadButtonPressed(gamepad, CROSS)) {
                edit_mode = !edit_mode;
            }
            if (IsGamepadButtonDown(gamepad, RIGHT_TRIGGER)) {
                isRecording = true;
            }
            if (IsGamepadButtonReleased(gamepad, RIGHT_TRIGGER)) {
                stopRecording = true;
                isRecording = false;
            }
        }

        // the timer
        GuiSetStyle(DEFAULT, TEXT_SIZE, 125);
        GuiLabel((Rectangle){ 0, 50, (float)GetScreenWidth(), 125 }, 
                 TextFormat("%02d:%02d:%02d.%d", 
                            time.h,
                            time.min, 
                            time.s, 
                            time.pre_ms)
                 );

        if (showSettings) GuiLock();
        GuiSetStyle(DEFAULT, TEXT_SIZE, 30);

        // settings gear
        GuiSetIconScale(2);
        if (GuiButton(settingsB, GuiIconText(ICON_GEAR, NULL))) showSettings = true;
        GuiSetIconScale(1);

        // the start button
        if ((GuiButton(startB, "Start") || padState.rightShoulder) && !timerStarted) {
            if (padState.rightShoulder) {
                SetGamepadVibration(gamepad, 0.0f, 0.75f, 0.5f);
                padState.rightShoulder = false;
            }
            timerStarted = true;
            if (!stopped) start_timer(&timer, GetTime());
            else {
                stopped = false;
                resume_timer(&timer, GetTime());
            }
        }

        // lap button. Name and school stay blank until a bib is entered for this row.
        if ((GuiButton(lapB, "Lap") ||padState.rightShoulder) && timerStarted && lap_count < MAX_RUNNER) {
            if (padState.rightShoulder) SetGamepadVibration(gamepad, 0.5f, 0.5f, 0.25f);
            Student none = {0};
            lap_times[lap_count] = time;
            write_row(&lap_times[lap_count], &none, lap_text[lap_count], 256, lap_count);
            laps[lap_count] = lap_text[lap_count];
            lap_count++;
            write_laps(SCORE_PATH, lap_count, laps);

            // scroll to bottom once per update
            int row = GuiGetStyle(LISTVIEW, LIST_ITEMS_HEIGHT) + GuiGetStyle(LISTVIEW, LIST_ITEMS_SPACING);
            int visible = (int)listH / row;
            if ((visible) < 1) visible = 1;
            if (lap_count > visible) scroll = lap_count - visible;
            else scroll = 0;
        }

        // stop button
        if ((GuiButton(stopB, "Stop") || padState.leftShoulder) && timerStarted) {
            if (padState.leftShoulder) {
                SetGamepadVibration(gamepad, 0.75f, 0.0f, 0.5f);
                padState.leftShoulder = false;
            }
            stopped = true;
            pause_timer(&timer, GetTime());
            timerStarted = false;
        }

        // reset button
        if (GuiButton(resetB, "Reset") && !timerStarted) {
            if (padState.leftTrigger) SetGamepadVibration(gamepad, 0.75f, 0.75f, 0.5f);
            stopped = false;
            end_timer(&timer);
            reset_time(&time);

            lap_count = 0;
            currentIndex = 0;
            bib = 0;
            for (int i = 0; i < MAX_RUNNER; i++) students[i] = (Student){0};
            write_laps(SCORE_PATH, lap_count, laps);

            scroll = 0;
            active = -1;
            focused = -1;
        }

        // bib value box
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
                write_laps(SCORE_PATH, lap_count, laps);
                if (!edit_mode) {
                    active = currentIndex;
                    currentIndex++;
                }
            }
            else bib = 0;
            bibEdit = !bibEdit;
        }

        dpad_scroll(&padState, &lap_count, &scroll, &active);

        // the grid list
        GuiListViewEx((Rectangle){listX, listY, listW, listH},
                      laps, lap_count, &scroll, &active, &focused);

        // the edit mode toggle
        const char *editLabel = "Edit Mode: OFF";
        if (edit_mode) editLabel = "Edit Mode: ON";
        GuiToggle(editBox, editLabel, &edit_mode);

        if (IsKeyPressed(KEY_DELETE) && edit_mode && active >= 0 && active < lap_count && !showSettings) {
            int currentActive = active;
            for (int i = currentActive; i < lap_count-1; i++) {
                lap_times[i] = lap_times[i+1];
                students[i] = students[i+1];
            }
            lap_count--;
            students[lap_count] = (Student){0};
            for (int i = currentActive; i < lap_count; i++) {
                write_row(&lap_times[i], &students[i], lap_text[i], 256, i);
            }
            write_laps(SCORE_PATH, lap_count, laps);
            if (currentActive < currentIndex) currentIndex--;
            active = -1;

            int row = GuiGetStyle(LISTVIEW, LIST_ITEMS_HEIGHT) + GuiGetStyle(LISTVIEW, LIST_ITEMS_SPACING);
            int visible =(int)listH / row;
            if (visible < 1) visible = 1;
            int maxScroll = lap_count -visible;
            if (maxScroll < 0) maxScroll = 0;
            if (scroll > maxScroll) scroll = maxScroll;
        }

        // opens a pop-up window
        int boardW = GuiGetTextWidth("Middle School") + 62;
        if (boardW < butW) boardW = butW;
        Rectangle boardB = {x, butY+butH+gap, (float)boardW, (float)butH};
        GuiToggle(boardB, "Scoreboard", &boardEdit);

        // Export PDF
        int pdfW = GuiGetTextWidth("Export to PDF") + 64;
        Rectangle pdfB = {x+(float)boardW+gap, butY+butH+gap, (float)pdfW, (float)butH};
        if (GuiButton(pdfB, "Export to PDF") && !timerStarted) {
            if (lap_count > 0) {
                sync_row(rows, lap_times, students, lap_count);

                Result results[DIV_COUNT] = {0};
                score_teams(rows, lap_count, results);

                if (write_results_pdf("results.pdf", rows, lap_count, results, &climate) < 0) {
                    printf("PDF SAVE FAILED: results.pdf\n");
                }

                // one PDF per division that ran
                Row division_rows[MAX_RUNNER];
                for (int div_slot = 0; div_slot < DIV_COUNT; div_slot++) {
                    if (results[div_slot].division[0] == '\0') continue;

                    int division_row_count = get_division_rows(
                        rows, lap_count, div_slot, division_rows);

                    char path[64];
                    snprintf(path, sizeof(path), "results_%s.pdf",
                             division_name(div_slot));

                    if (write_division_pdf(path, division_rows,
                                           division_row_count,
                                           &results[div_slot],
                                           division_name(div_slot), &climate) < 0) {
                        printf("PDF SAVE FAILED: %s\n", path);
                    }
                }
            }
        }

        //settings
        if (showSettings) {
            GuiUnlock();
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.4f));

            float panelW = GetScreenWidth() * 0.7071f;
            float panelH = GetScreenHeight() * 0.7071f;
            Rectangle panel = {(GetScreenWidth()-panelW)/2.0f, (GetScreenHeight()-panelH)/2.0f, panelW, panelH};

            float tempLabelW = GuiGetTextWidth("Enter Temperature (F)") + 2;
            float windLabelW = GuiGetTextWidth("Enter Wind Speed (mph)") + 2;
            float directionLabelW = GuiGetTextWidth("Enter Wind Direction") + 2;
            float precepLabelW = GuiGetTextWidth("Enter Rainfall (inches)") + 2;
            float cloudLabelW = GuiGetTextWidth("Enter Cloud Level (0-10)") + 2;

            // the widest label decides where the box column starts
            float labelW = tempLabelW;
            if (windLabelW > labelW) labelW = windLabelW;
            if (directionLabelW > labelW) labelW = directionLabelW;
            if (precepLabelW > labelW) labelW = precepLabelW;
            if (cloudLabelW > labelW) labelW = cloudLabelW;
            float boxX = panel.x + gap + labelW;
            float rowY = panel.y + RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT + gap;
            float rowStep = 40 + gap;
            Rectangle tempBox = {boxX, rowY, 160, 40};
            // GuiTextBox and GuiDropdownBox draw no label, so these sit where the value boxes put theirs
            Rectangle windLabel = {boxX - labelW, rowY + rowStep, labelW + GuiGetStyle(LABEL, BORDER_WIDTH), 40};
            Rectangle windInputBox = {boxX, rowY + rowStep, 160, 40};
            Rectangle directionLabel = {boxX - labelW, rowY + 2*rowStep, labelW + GuiGetStyle(LABEL, BORDER_WIDTH), 40};
            Rectangle directionBox = {boxX, rowY + 2*rowStep, 160, 40};
            Rectangle precepBox = {boxX, rowY + 3*rowStep, 160, 40};
            Rectangle cloudBox = {boxX, rowY + 4*rowStep, 160, 40};
            // the open direction list covers the rows below it
            if (directionEdit) GuiLock();
            if (GuiWindowBox(panel, "Settings")) showSettings = false;

            if (GuiValueBoxFloat(tempBox, "Enter Temperature (F) ", climate.tempreture, &temp, tempEdit)) {
                if (tempEdit) endClimateEdit(temp, climate.tempreture, sizeof(climate.tempreture));
                else beginClimateEdit(climate.tempreture, sizeof(climate.tempreture));
                tempEdit = !tempEdit;
            }

            GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_RIGHT);
            GuiLabel(windLabel, "Enter Wind Speed (mph) ");
            GuiLabel(directionLabel, "Enter Wind Direction ");
            GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
            if (GuiTextBox(windInputBox, climate.wind, sizeof(climate.wind), windEdit)) {
                if (!windEdit) beginClimateEdit(climate.wind, sizeof(climate.wind));
                if (windEdit && climate.wind[0] == '\0') snprintf(climate.wind, sizeof(climate.wind), CLIMATE_UNKNOWN);
                windEdit = !windEdit;
            }

            if (GuiValueBoxFloat(precepBox, "Enter Rainfall (inches) ", climate.precipitation, &precep, precepEdit)) {
                if (precepEdit) endClimateEdit(precep, climate.precipitation, sizeof(climate.precipitation));
                else beginClimateEdit(climate.precipitation, sizeof(climate.precipitation));
                precepEdit = !precepEdit;
            }

            if (GuiValueBoxUnset(cloudBox, "Enter Cloud Level (0-10) ", &climate.clouds, 0, 10, cloudEdit)) {
                cloudEdit = !cloudEdit;
            }

            if (directionEdit) GuiUnlock();
            if (GuiDropdownBox(directionBox, "N;S;W;E;NW;NE;SW;SE", &windDirection, directionEdit)) {
                snprintf(climate.windDirection, sizeof(climate.windDirection), "%s", wind_direction(windDirection));
                directionEdit = !directionEdit;
            }
        }

        if (!transcribing && padState.rightTrigger) {
            audio_buffer_size = 0;
            start_recording(mic);
            recordingStarted = true;
        }
        // a press that came in while transcribing never started, so it records nothing
        if (recordingStarted && isRecording) {
            SetGamepadVibration(gamepad, 0.1f, 0.1f, 0.1f);
            int result = record(mic, &audio_buffer[audio_buffer_size], BUFF_SIZE-audio_buffer_size);
            if (result > -1) {
                audio_buffer_size += result;
            }
        }
        if (recordingStarted && stopRecording) {
            pause_recording(mic);
            int result = record(mic, &audio_buffer[audio_buffer_size], BUFF_SIZE-audio_buffer_size);
            if (result > -1) {
                audio_buffer_size += result;
            }
            recordingStarted = false;
            if (audio_buffer_size > (WHISPER_SAMPLE_RATE / 10)) isReady = true;
        }

        if (isReady) {
            isReady = false;
            data.buffer_size = audio_buffer_size;
            whisper_thread = SDL_CreateThread(transcribe, "whisper", &data);
            if (whisper_thread != NULL) transcribing = true;
        }
        // data.result is safe to read once SDL_WaitThread returns
        if (transcribing && SDL_GetThreadState(whisper_thread) == SDL_THREAD_COMPLETE) {
            SDL_WaitThread(whisper_thread, NULL);
            whisper_thread = NULL;

            int row = currentIndex;
            if (edit_mode) row = active;
            if (data.result > -1 && row >= 0 && row < lap_count) {
                Student who = {0};
                who.bib = data.result;
                int found = search_name(roster, runners, data.result);
                if (found >= 0) who = roster[found];
                write_student(&who, &students[row]);
                write_row(&lap_times[row], &who, lap_text[row], 256, row);
                write_laps(SCORE_PATH, lap_count, laps);
                if (!edit_mode) {
                    active = currentIndex;
                    currentIndex++;
                }
            }
            else SetGamepadVibration(gamepad, 1.0f, 1.0f, 1.0f);
            transcribing = false;
        }

        // read the time after every frame
        if (timerStarted) read_time(&timer, &time, GetTime());

        zero_state(&padState);
        stopRecording = false;
        EndDrawing();
    }

    if (boardStarted) {
        SDL_KillProcess(scoreboard_process, true);
        SDL_DestroyProcess(scoreboard_process);
    }
    remove(SCORE_PATH);
    SDL_WaitThread(whisper_thread, NULL);
    free(audio_buffer);
    whisper_free(ctx);
    SDL_DestroyAudioStream(mic);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);

    UnloadFont(font);
    CloseWindow();
    return 0;
}
