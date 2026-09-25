#include <data.h>
#include <csv.h>

int search_name(Student roster[], int length, int bib) {
    for (int i = 0; i < length; i++) {
        if (bib == roster[i].bib) return i;
    }
    return -1;
}

void write_row(const Time *time, const Student *student, char *buffer, int buff_size, int rank) {
    if (student->name[0] != '\0') {
        snprintf(buffer, (size_t)buff_size,
                 "Rank: %d | Time: %02d:%02d:%02d.%03d | Bib: %d | %s | %s",
                 rank + 1, time->h, time->min, time->s, time->ms,
                 student->bib, student->name, student->school);
    } else if (student->bib > 0) {
        snprintf(buffer, (size_t)buff_size,
                 "Rank: %d | Time: %02d:%02d:%02d.%03d | Bib: %d",
                 rank + 1, time->h, time->min, time->s, time->ms, student->bib);
    } else {
        snprintf(buffer, (size_t)buff_size,
                 "Rank: %d | Time: %02d:%02d:%02d.%03d",
                 rank + 1, time->h, time->min, time->s, time->ms);
    }
}
