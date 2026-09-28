#include <data.h>
#include <stdio.h>

int search_name(Student roster[], int length, int bib) {
    for (int i = 0; i < length; i++) {
        if (bib == roster[i].bib) return i;
    }
    return -1;
}

void write_student(Student *source, Student *target){
    target->bib = source->bib;
    snprintf(target->name, sizeof(target->name), "%s", source->name);
    snprintf(target->school, sizeof(target->school), "%s", source->school);
}

void sync_row(Row stats[], Time lap_times[], Student students[], int size) {
    for (int i = 0; i < size; i++) {
        stats[i].rank = i+1;
        stats[i].bib = students[i].bib;
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
                 "Rank: %d | Time: %02d:%02d:%02d.%03d | Bib: %ld | %s | %s",
                 rank + 1, time->h, time->min, time->s, time->ms,
                 student->bib, student->name, student->school);
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
