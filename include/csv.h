#ifndef CSV_H
#define CSV_H

#include "time.h"
#include <stdio.h>

typedef struct {
    int rank;
    int bib;
    char time[20];
    char school[32];
} Row;

typedef struct {
    int bib;
    char name[50];
    char school[20];
} Student;

void init_csv(FILE *file);
void write_csv(FILE *file, const Time *time, int rank);
int read_lookup(FILE *lookup, Student index[], int capacity);

#endif
