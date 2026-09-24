#include <csv.h>
#include <raylib.h>

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

void read_lookup(FILE *lookup, Student index[]) {
    int lines = 0;
    int c;
    while ((c = fgetc(lookup)) != EOF) {
        if (c == '\n') lines++;
    }
    int runners = lines - 1;
    if (runners < 0) runners = 0;

    rewind(lookup);

    // back to the start, read the file
}
