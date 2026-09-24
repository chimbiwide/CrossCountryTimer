#include <csv.h>
#include <raylib.h>
#include <stdlib.h>
#include <string.h>

void init_csv(FILE *file) {
    file = fopen("result.csv", "w");
    if (file == NULL) {
        printf("Error Opening File\n");
        return;
    }
    fprintf(file, "Rank,Bib,Time,School\n");
    fclose(file);
    return;
}
// need to add a the divsion name later
// for now keep at result.csv
void write_csv(FILE *file, const Time *time, int rank) {
    file = fopen("result.csv", "a");
    if (file == NULL) {
        printf("Error Opening File\n");
        return;
    }
    fprintf(file, "%d,%s,%s,%s\n", 
            rank+1, 
            "",
            TextFormat("%02d:%02d:%02d.%03d", time->h, time->min, time->s, time->ms),
            "");
    fclose(file);
    return;
}

static int split_csv(char *line, char *fields[], int max_fields) {
    size_t len = strlen(line);
    while (len > 0 && (line[len-1] == '\n') || line[len-1] == '\r') {
        line[--len] = '\0';
    }

    int n = 0;
    char *p = line;
    while (*p && n < max_fields) {
        if (*p == '"') {
            fields[n++] = ++p;
            while (*p && *p != '"') p++;
            if (*p == '"') *p++ = '\0';
            if (*p == ',') p++;
        }
        else {
            fields[n++] = p;
            while (*p && *p != ',') p++;
            if (*p == ',') *p++ = '\0';
        }
    }
    return n;
}

int read_lookup(FILE *lookup, Student index[], int capacity) {
    char buffer[128];
    int count = 0;
    bool header = true;

    while (fgets(buffer, sizeof(buffer), lookup)) {
        if (header) {
            header = false;
            continue;
        }
        if (count >= capacity) break;

        char *fields[4];
        if (split_csv(buffer, fields, 4) < 3) continue;

        index[count].bib = atoi(fields[0]);
        snprintf(index[count].name, sizeof(index[count].name), "%s", fields[1]);
        snprintf(index[count].school, sizeof(index[count].school), "%s", fields[2]);
        count++;
    }
    return count;
}
