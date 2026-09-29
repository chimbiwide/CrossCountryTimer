#ifndef DATA_H
#define DATA_H

#include <xlsx.h>

int search_name(Student roster[], int capacity, int bib);
void write_row(const Time *time, const Student *student, char *buffer, int buff_size, int rank);
void write_student(Student *source, Student *target);
void sync_row(Row stats[], Time lap_times[], Student students[], int size);
void calcuate_score(Row stats[], int row_count, char *winner, int *winner_score, char *loser, int *loser_score);
int find_division(const char *division_code);
void score_teams(Row complete[], int row_count, Result results[]);
int get_division_rows(Row complete[], int row_count, int division, Row division_rows[]);
const char *division_name(int division);

#endif
