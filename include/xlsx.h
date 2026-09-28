#ifndef CSV_H
#define CSV_H

#include <xlsxio_read.h>
#include <timer.h>
#include <stdio.h>

typedef struct {
    int rank;
    int bib;
    char time[20];
    char school[32];
} Row;

typedef struct {
    int64_t bib;
    char name[50];
    char school[20];
} Student;

int read_lookup(xlsxioreader reader, const char *sheetname, Student index[], int capacity);

#endif
