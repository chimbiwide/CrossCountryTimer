#ifndef PDF_H
#define PDF_H

#include "xlsx.h"
#include "data.h"
#include <pdfgen.h>

int write_results_pdf(const char *path, Row rows[], int row_count, Result results[],
                      const Climate *climate);
int write_division_pdf(const char *path, Row rows[], int row_count,
                       const char *winner, int winner_score,
                       const char *loser, int loser_score,
                       const char *division, const Climate *climate);

#endif
