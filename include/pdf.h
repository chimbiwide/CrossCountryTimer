#ifndef PDF_H
#define PDF_H

#include "xlsx.h"
#include <pdfgen.h>

int write_results_pdf(const char *path, Row rows[], int row_count, Result results[]);

#endif
