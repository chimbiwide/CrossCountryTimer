#ifndef ROSTER_H
#define ROSTER_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_ATHLETES 2048

typedef struct {
    int64_t bib;
    int number;         // the "12." in front of the name, counts up per school
    int year;
    char sex;
    char name[64];      // as printed, "Tassone, Nico"
    char school[16];    // code from the school line, "BCH"
} Athlete;

typedef struct {
    Athlete athletes[MAX_ATHLETES];
    int count;
    int total;          // the "Total Athletes:" line, -1 until it is read
    char school[16];    // school of the rows being read, carried from one crop to the next
} Roster;

void roster_init(Roster *roster);
int roster_add_text(Roster *roster, const char *text);
bool roster_check(const Roster *roster, char *error, int size);
void roster_name(const Athlete *athlete, char *out, int size);
const char *roster_division(const Athlete *athlete);
bool roster_write_xlsx(const Roster *roster, const char *path);

#endif
