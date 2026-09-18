#ifndef CSV_H
#define CSV_H

#include "time.h"
#include <stdio.h>

void init_csv(FILE *file);
void write_csv(FILE *file, const Time *time, int rank);

#endif
