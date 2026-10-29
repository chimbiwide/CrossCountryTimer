#include <data.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <xlsx.h>

int search_name(Student roster[], int length, int bib) {
    for (int i = 0; i < length; i++) {
        if (bib == roster[i].bib) return i;
    }
    return -1;
}

void write_student(Student *source, Student *target){
    target->bib = source->bib;
    snprintf(target->division, sizeof(target->division), "%s", source->division);
    snprintf(target->name, sizeof(target->name), "%s", source->name);
    snprintf(target->school, sizeof(target->school), "%s", source->school);
}

void sync_row(Row stats[], Time lap_times[], Student students[], int size) {
    for (int i = 0; i < size; i++) {
        stats[i].rank = i+1;
        stats[i].bib = students[i].bib;
        snprintf(stats[i].division, sizeof(stats[i].division), "%s", students[i].division);
        snprintf(stats[i].name, sizeof(stats[i].name), "%s", students[i].name);
        snprintf(stats[i].school, sizeof(stats[i].school), "%s", students[i].school);
        snprintf(stats[i].time, sizeof(stats[i].time), 
                 "%02d:%02d:%02d.%03d", 
                 lap_times[i].h, lap_times[i].min, lap_times[i].s, lap_times[i].ms);
    }
}

void write_row(const Time *time, const Student *student, char *buffer, int buff_size, int rank) {
    if (student->name[0] != '\0') {
        snprintf(buffer, (size_t)buff_size,
                 "Rank: %d | Time: %02d:%02d:%02d.%03d | Bib: %lld | %s | %s | %s",
                 rank + 1, time->h, time->min, time->s, time->ms,
                 (long long)student->bib, student->name, student->school, student->division);
    } else if (student->bib > 0) {
        snprintf(buffer, (size_t)buff_size,
                 "Rank: %d | Time: %02d:%02d:%02d.%03d | Bib: %lld",
                 rank + 1, time->h, time->min, time->s, time->ms, (long long)student->bib);
    } else {
        snprintf(buffer, (size_t)buff_size,
                 "Rank: %d | Time: %02d:%02d:%02d.%03d",
                 rank + 1, time->h, time->min, time->s, time->ms);
    }
}

int find_division(const char *division_code) {
    if (division_code[0] == 'V') return 0;
    if (division_code[0] == 'J') return 1;
    if (division_code[0] == 'F') return 2;
    if (division_code[0] == 'M') return 3;
    return -1;
}

int get_division_rows(Row complete[], int row_count, int division, Row division_rows[]) {
    int division_row_count = 0;
    for (int row = 0; row < row_count; row++) {
        if (find_division(complete[row].division) == division && complete[row].school[0] != '\0') {
            division_rows[division_row_count] = complete[row];
            division_row_count++;
        }
    }
    for (int place = 0; place < division_row_count; place++) {
        division_rows[place].rank = place+1;
    }
    return division_row_count;
}

void score_teams(Row complete[], int row_count, Result results[]) {
    Row division_rows[MAX_RUNNER];

    for (int division = 0; division < DIV_COUNT; division++) {
        results[division] = (Result){0};

        int division_row_count = get_division_rows(complete, row_count, division, division_rows);
        if (division_row_count == 0) continue;

        results[division].school_count = sync_school(division_rows, division_row_count, results[division].schools);

        snprintf(results[division].division,
                 sizeof(results[division].division),
                 "%s", division_rows[0].division);

        int place = 1;
        for (int j = 0; j < division_row_count; j++) {
            int found = search_school(results[division].schools, 
                                      results[division].school_count,
                                      division_rows[j].school);
            if (found < 0) continue;
            School *school = &results[division].schools[found];
            
            if (school->runners < 5) {
                school->cumulative[school->placed] = division_rows[j].rank;
                school->placed++;
                continue;
            }
            if (school->placed >= 7) continue;

            if (school->placed < 5) {
                school->score += place;
                school->cumulative[school->placed] = place;
            }
            else if (school->placed == 5) {
                school->sixth = place;
                school->cumulative[5] = place;
            }
            school->placed++;
            place++;
        }

        sort_schools(results[division].schools, results[division].school_count);
    }
}

static int school_beats(const School *a, const School *b) {
    int a_full = a->runners >= 5;
    int b_full = b->runners >= 5;

    if (a_full != b_full) return a_full;
    if (!a_full) return 0;

    if (a->score != b->score) return a->score < b->score;

    if (a->sixth == 0) return 0;
    if (b->sixth == 0) return 1;
    return a->sixth < b->sixth;
}


void sort_schools(School schools[], int school_count) {
    for (int i = 0; i < school_count; i++) {
        int best = i;
        for (int j = i + 1; j < school_count; j++) {
            if (school_beats(&schools[j], &schools[best])) best = j;
        }
        School temp = schools[i];
        schools[i] = schools[best];
        schools[best] = temp;
    }
}

const char *division_name(int division) {
    if (division == 1) return "JV";
    if (division == 2) return "Freshman";
    if (division == 3) return "MS";
    return "Varsity";
}

const char *wind_direction(int direction) {
    switch(direction) {
        case 1: return "S";
        case 2: return "W";
        case 3: return "E";
        case 4: return "NW";
        case 5: return "NE";
        case 6: return "SW";
        case 7: return "SE";
        default: return "N";
    }
}

void floatToStr(float input, char *output, int size){
    snprintf(output, size, "%.1f", input);
}

void beginClimateEdit(char *text, int size){
    if (strcmp(text, CLIMATE_UNKNOWN) == 0) memset(text, 0, size);
}

void endClimateEdit(float value, char *text, int size){
    int typed = text[0] != '\0';
    memset(text, 0, size);
    if (typed) floatToStr(value, text, size);
    else snprintf(text, size, "%s", CLIMATE_UNKNOWN);
}

int search_school(School schools[], int length, const char *name) {
    for (int i = 0; i < length; i++) {
        if (strcmp(name, schools[i].name) == 0) return i;
    }
    return -1;
}

int sync_school(Row stats[], int row_count, School schools[]){
    int school_count = 0;
    for (int i = 0; i < row_count; i++) {
        if (stats[i].school[0] == '\0') continue;

        int found = search_school(schools, school_count, stats[i].school);
        if (found >= 0) {
            schools[found].runners++;
        }
        else if (school_count < MAX_SCHOOLS) {
            snprintf(schools[school_count].name, sizeof(schools[school_count].name),
                     "%s", stats[i].school);
            schools[school_count].runners = 1;
            school_count++;
        }
    }
    return school_count;
}


