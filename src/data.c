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
                 "Rank: %d | Time: %02d:%02d:%02d.%03d | Bib: %ld | %s | %s | %s",
                 rank + 1, time->h, time->min, time->s, time->ms,
                 student->bib, student->name, student->school, student->division);
    } else if (student->bib > 0) {
        snprintf(buffer, (size_t)buff_size,
                 "Rank: %d | Time: %02d:%02d:%02d.%03d | Bib: %ld",
                 rank + 1, time->h, time->min, time->s, time->ms, student->bib);
    } else {
        snprintf(buffer, (size_t)buff_size,
                 "Rank: %d | Time: %02d:%02d:%02d.%03d",
                 rank + 1, time->h, time->min, time->s, time->ms);
    }
}

// this is a preliminary design, needs a adjustable number later
void calcuate_score(Row stats[], int row_count, char *winner, int *winner_score, char *loser, int *loser_score){
    char *school1 = stats[0].school;
    int school1_count = 0;
    int school1_score = 0;
    int school1_tiebreaker = INT_MAX;

    size_t school_len = sizeof(stats[0].school);

    char *school2 = "";
    int school2_count = 0;
    int school2_score = 0;
    int school2_tiebreaker = INT_MAX;

    int i = 1;
    while (i < row_count && strcmp(school1, stats[i].school) == 0) {
        i++;
    }
    if (i >= row_count) return;
    school2 = stats[i].school;

    for (int i = 0; i < row_count; i++) {
        // compare school1
        if (strcmp(stats[i].school, school1) == 0){
            if (school1_count < 5 && school2_count < 8) {
                school1_score += stats[i].rank;
                school1_count++;
            }
            else if (school1_count < 5 && school2_count > 7) {
                int diff = school2_count - 7;
                school1_score += stats[i].rank-diff;
                school1_count++;
            }
            else if (school1_count > 5) {
                school1_count++;
            }
            else if (school1_count == 5) {
                school1_tiebreaker = stats[i].rank;
                school1_count++;
            }
        }
        // compare school2
        else if (strcmp(stats[i].school, school2) == 0) {
            if (school2_count < 5 && school1_count < 8) {
                school2_score += stats[i].rank;
                school2_count++;
            }
            else if (school2_count < 5 && school1_count > 7) {
                int diff = school1_count - 7;
                school2_score += stats[i].rank-diff;
                school2_count++;
            }
            else if (school2_count > 5) {
                school2_count++;
            }
            else if (school2_count == 5) {
                school2_tiebreaker = stats[i].rank;
                school2_count++;
            }
        }
        else continue;
    }
    if (school1_score > school2_score) {
        snprintf(winner, school_len, "%s", school2);
        *winner_score = school2_score;
        snprintf(loser, school_len, "%s", school1);
        *loser_score = school1_score;
    }
    else if (school1_score < school2_score) {
        snprintf(winner, school_len, "%s", school1);
        *winner_score = school1_score;
        snprintf(loser, school_len, "%s", school2);
        *loser_score = school2_score;
    }
    else {
        // tie
        if (school1_tiebreaker < school2_tiebreaker) {
        snprintf(winner, school_len, "%s", school1);
        *winner_score = school1_score;
        snprintf(loser, school_len, "%s", school2);
        *loser_score = school2_score;
        }
        else if (school1_tiebreaker > school2_tiebreaker) {
        snprintf(winner, school_len, "%s", school2);
        *winner_score = school2_score;
        snprintf(loser, school_len, "%s", school1);
        *loser_score = school1_score;
        }
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

        snprintf(results[division].division,
                 sizeof(results[division].division),
                 "%s", division_rows[0].division);
        calcuate_score(division_rows, division_row_count,
                       results[division].winner,
                       &results[division].winnerScore,
                       results[division].loser,
                       &results[division].loserScore);
    }
}

const char *division_name(int division) {
    if (division == 1) return "JV";
    if (division == 2) return "Freshman";
    if (division == 3) return "MS";
    return "Varsity";
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
