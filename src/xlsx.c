#include "xlsxio_read.h"
#include <xlsx.h>
#include <raylib.h>
#include <stdio.h>
#include <string.h>

int read_lookup(xlsxioreader reader, const char *sheetname, Student index[], int capacity) {
    xlsxioreadersheet sheet;
    int count = 0;
    bool header = true;

    if ((sheet = xlsxioread_sheet_open(reader, sheetname, XLSXIOREAD_SKIP_EMPTY_ROWS)) != NULL) {
        while (xlsxioread_sheet_next_row(sheet)){
            if (header){
                header = false;
                continue;
            }
            if (count >= capacity) break;

            int64_t bib = 0;
            char *name = NULL;
            char *school = NULL;
            int got_bib = xlsxioread_sheet_next_cell_int(sheet, &bib);
            int got_name = xlsxioread_sheet_next_cell_string(sheet, &name);
            int got_school = xlsxioread_sheet_next_cell_string(sheet, &school);
            if (got_bib && got_name && got_school) {
                index[count].bib = bib;
                snprintf(index[count].name, sizeof(index[count].name), "%s", name);
                snprintf(index[count].school, sizeof(index[count].school), "%s", school);
                count++;
            }
            xlsxioread_free(name);
            xlsxioread_free(school);
        }
        xlsxioread_sheet_close(sheet);
    }
    return count;
}
