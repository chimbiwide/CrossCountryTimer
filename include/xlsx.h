#ifndef XLSX_H
#define XLSX_H

#include <xlsxio_read.h>
#include <timer.h>

#define DIV_COUNT 4
#define MAX_RUNNER 200

typedef struct {
    int rank;
    int64_t bib;
    char name[50];
    char school[20];
    char division[4];
    char time[20];
} Row;

typedef struct {
    char division[4];
    char winner[20];
    int winnerScore;
    char loser[20];
    int loserScore;
} Result;

typedef struct {
    int64_t bib;
    char division[4];
    char name[50];
    char school[20];
} Student;

int read_lookup(xlsxioreader reader, const char *sheetname, Student index[], int capacity);

#endif
