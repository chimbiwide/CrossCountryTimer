#ifndef XLSX_H
#define XLSX_H

#include <xlsxio_read.h>
#include <timer.h>

typedef struct {
    int rank;
    int64_t bib;
    char name[50];
    char school[20];
    char division[4];
    char time[20];
} Row;

typedef struct {
    int64_t bib;
    char division[4];
    char name[50];
    char school[20];
} Student;

int read_lookup(xlsxioreader reader, const char *sheetname, Student index[], int capacity);

#endif
