#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <SDL3/SDL_thread.h>
#include "raylib.h"

#include <font.h>
#include <render.h>
#include <roster.h>
#include <vlm.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define DPI 200

// relative path
#define MODEL_PATH "models/" RP_MODEL_FILE
#define MMPROJ_PATH "models/" RP_MMPROJ_FILE

enum { STAGE_IDLE, STAGE_LOADING, STAGE_READING };

typedef struct {
    char pdf_path[1024];
    Roster roster;
    atomic_int stage;
    atomic_int page;
    atomic_int pages;
    atomic_bool stop;
    bool ok;
    char message[1280];
} Job;

static Vlm vlm;
static bool vlm_loaded = false;

// Reads every strip of every page into job->roster and checks the result.
// All pages are cut into strips first, so the model can read READ_GROUP
// strips at a time even across pages.
static bool parse_pdf(Job *job) {
    static char text_space[READ_GROUP][16384];
    char *texts[READ_GROUP];
    for (int i = 0; i < READ_GROUP; i++) texts[i] = text_space[i];
    Pdf pdf;

    roster_init(&job->roster);
    if (!render_open(&pdf, job->pdf_path)) {
        snprintf(job->message, sizeof(job->message), "Could not open %s", job->pdf_path);
        return false;
    }
    atomic_store(&job->pages, pdf.pages);
    atomic_store(&job->stage, STAGE_READING);

    int room = pdf.pages * MAX_CROPS;
    if (room < 1) room = 1;
    Crop *strips = calloc((size_t)room, sizeof(Crop));
    int *page_of = calloc((size_t)room, sizeof(int));   // the page each strip is from
    int total = 0;
    bool ok = strips != NULL && page_of != NULL;
    if (!ok) snprintf(job->message, sizeof(job->message), "Out of memory");
    for (int page = 0; page < pdf.pages && ok; page++) {
        if (atomic_load(&job->stop)) {
            snprintf(job->message, sizeof(job->message), "Stopped");
            ok = false;
            break;
        }
        int count = render_page(&pdf, page, DPI, strips + total, MAX_CROPS);
        if (count < 0) {
            snprintf(job->message, sizeof(job->message), "Could not render page %d", page + 1);
            ok = false;
            break;
        }
        for (int i = 0; i < count; i++) page_of[total + i] = page;
        total += count;
    }

    for (int first = 0; first < total && ok; first += READ_GROUP) {
        if (atomic_load(&job->stop)) {
            snprintf(job->message, sizeof(job->message), "Stopped");
            ok = false;
            break;
        }
        atomic_store(&job->page, page_of[first]);
        int count = total - first;
        if (count > READ_GROUP) count = READ_GROUP;
        int strips_read = vlm_read(&vlm, strips + first, count, texts, sizeof(text_space[0]));
        for (int i = 0; i < count && ok; i++) {
            if (i == strips_read) {
                int strip = first + i;
                int page = page_of[strip];
                int part = 1;
                while (strip - part >= 0 && page_of[strip - part] == page) part++;
                snprintf(job->message, sizeof(job->message), "The model could not read page %d (part %d)",
                         page + 1, part);
                ok = false;
                break;
            }
            roster_add_text(&job->roster, texts[i]);
        }
    }
    atomic_store(&job->page, pdf.pages);
    for (int i = 0; i < total; i++) crop_free(&strips[i]);
    free(strips);
    free(page_of);
    render_close(&pdf);

    if (ok) ok = roster_check(&job->roster, job->message, sizeof(job->message));
    return ok;
}

static int parse_thread(void *arg) {
    Job *job = arg;
    job->ok = false;
    if (!vlm_loaded) {
        atomic_store(&job->stage, STAGE_LOADING);
        vlm_loaded = vlm_load(&vlm, MODEL_PATH, MMPROJ_PATH, vlm_threads());
        if (!vlm_loaded) {
            vlm_free(&vlm);
            snprintf(job->message, sizeof(job->message), "Could not load the model %s", MODEL_PATH);
        }
    }
    vlm.stop = &job->stop;
    if (vlm_loaded) job->ok = parse_pdf(job);
    atomic_store(&job->stage, STAGE_IDLE);
    return 0;
}

int main(void)
{
    InitWindow(1280, 720, "Roster Parser");
    SetTargetFPS(60);

    Font font = LoadFontFromMemory(".ttf", google_sans, google_sans_size, 125, NULL, 0);
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    GuiSetFont(font);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 30);
    GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, ColorToInt(BLACK));
    GuiSetStyle(LISTVIEW, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(LISTVIEW, LIST_ITEMS_HEIGHT, 40);

    static Job job = {0};
    SDL_Thread *thread = NULL;
    bool busy = false;
    bool parsed = false;

    char pdf_path[1024] = "./data/roster.pdf";
    char xlsx_path[1024] = "index.xlsx";
    bool pdf_edit = false;
    bool xlsx_edit = false;
    char status[1536] = "Drop a roster PDF on the window, or type its path";

    static char row_text[MAX_ATHLETES][96];
    static char *rows[MAX_ATHLETES];
    int row_count = 0;
    int scroll = 0;
    int active = -1;
    int focused = -1;

    while (!WindowShouldClose()) {
        if (IsFileDropped()) {
            FilePathList dropped = LoadDroppedFiles();
            if (dropped.count > 0 && !busy) snprintf(pdf_path, sizeof(pdf_path), "%s", dropped.paths[0]);
            UnloadDroppedFiles(dropped);
        }

        // the worker is done: show what it read (job is safe to read once SDL_WaitThread returns)
        if (busy && SDL_GetThreadState(thread) == SDL_THREAD_COMPLETE) {
            SDL_WaitThread(thread, NULL);
            thread = NULL;
            busy = false;
            parsed = job.ok;
            row_count = 0;
            for (int i = 0; i < job.roster.count; i++) {
                const Athlete *athlete = &job.roster.athletes[i];
                char name[64];
                roster_name(athlete, name, sizeof(name));
                snprintf(row_text[i], sizeof(row_text[i]), "%5lld  %-28s %-6s %s",
                         (long long)athlete->bib, name, athlete->school, roster_division(athlete));
                rows[i] = row_text[i];
                row_count++;
            }
            if (parsed) snprintf(status, sizeof(status), "Read %d athletes. Check them, then save.", row_count);
            else snprintf(status, sizeof(status), "Failed: %s", job.message);
        }
        if (busy) {
            int stage = atomic_load(&job.stage);
            if (stage == STAGE_LOADING) snprintf(status, sizeof(status), "Loading the model...");
            if (stage == STAGE_READING) {
                snprintf(status, sizeof(status), "Reading page %d of %d...",
                         atomic_load(&job.page) + 1, atomic_load(&job.pages));
            }
        }

        BeginDrawing();
        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        if (busy) GuiLock();

        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_RIGHT);
        GuiLabel((Rectangle){ 20, 30, 140, 50 }, "PDF");
        GuiLabel((Rectangle){ 20, 100, 140, 50 }, "Save to");
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);

        if (GuiTextBox((Rectangle){ 180, 30, 820, 50 }, pdf_path, sizeof(pdf_path), pdf_edit)) pdf_edit = !pdf_edit;
        if (GuiTextBox((Rectangle){ 180, 100, 820, 50 }, xlsx_path, sizeof(xlsx_path), xlsx_edit)) xlsx_edit = !xlsx_edit;

        if (GuiButton((Rectangle){ 1020, 30, 240, 50 }, "Parse") && pdf_path[0] != '\0') {
            snprintf(job.pdf_path, sizeof(job.pdf_path), "%s", pdf_path);
            atomic_store(&job.stop, false);
            atomic_store(&job.page, 0);
            atomic_store(&job.pages, 0);
            parsed = false;
            row_count = 0;
            thread = SDL_CreateThread(parse_thread, "parse", &job);
            if (thread != NULL) busy = true;
            else snprintf(status, sizeof(status), "Could not start the parser");
        }

        if (!parsed) GuiDisable();
        if (GuiButton((Rectangle){ 1020, 100, 240, 50 }, "Save")) {
            if (roster_write_xlsx(&job.roster, xlsx_path)) {
                snprintf(status, sizeof(status), "Saved %d athletes to %s", job.roster.count, xlsx_path);
            }
            else snprintf(status, sizeof(status), "Could not write %s", xlsx_path);
        }
        GuiEnable();

        float progress = 0.0f;
        if (atomic_load(&job.pages) > 0) progress = (float)atomic_load(&job.page) / atomic_load(&job.pages);
        GuiProgressBar((Rectangle){ 180, 170, 820, 30 }, NULL, NULL, &progress, 0.0f, 1.0f);
        GuiLabel((Rectangle){ 180, 205, 1080, 40 }, status);

        GuiListViewEx((Rectangle){ 20, 260, 1240, 440 }, rows, row_count, &scroll, &active, &focused);

        GuiUnlock();
        EndDrawing();
    }

    // a parse still running is told to stop, and ends after the step it is in
    if (busy) {
        atomic_store(&job.stop, true);
        SDL_WaitThread(thread, NULL);
    }
    if (vlm_loaded) vlm_free(&vlm);
    UnloadFont(font);
    CloseWindow();
    return 0;
}
