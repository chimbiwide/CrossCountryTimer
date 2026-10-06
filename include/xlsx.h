#ifndef XLSX_H
#define XLSX_H

#include <xlsxio_read.h>
#include <timer.h>

#define DIV_COUNT 4
#define MAX_RUNNER 512
#define MAX_SCHOOLS 256

typedef struct {
    int rank;
    int64_t bib;
    char name[50];
    char school[20];
    char division[4];
    char time[20];
} Row;

typedef struct {
    char name[20];
    int cumulative[6];
    int runners;
    int placed;
    int score;
    int sixth;
} School;

typedef struct {
    char division[4];
    School schools[MAX_SCHOOLS];
    int school_count;
} Result;

typedef struct {
    int64_t bib;
    char division[4];
    char name[50];
    char school[20];
} Student;


int read_lookup(xlsxioreader reader, const char *sheetname, Student index[], int capacity);

int read_schools(xlsxioreader reader, const char *sheetname, School list[], int capacity);

#endif
