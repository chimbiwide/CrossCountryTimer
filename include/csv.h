#ifndef CSV_H
#define CSV_H

#include "time.h"
#include <stdio.h>

typedef struct {
    int rank;
    int bib;
    char *time[14];
    char *school[20];
} Row;

void init_csv(FILE *file);
void write_csv(FILE *file, const Time *time, int rank);
int read_csv(FILE *file, Row buffer[100]);

#endif
