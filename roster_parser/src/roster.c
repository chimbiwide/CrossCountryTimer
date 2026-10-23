#include <roster.h>
#include <xlsx.h>     // the timer's read_lookup(), from ../src/xlsx.c
#include <xlsxio_write.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void roster_init(Roster *roster) {
    memset(roster, 0, sizeof(*roster));
    roster->total = -1;
}

// "Boston College High School (BCH)" -> "BCH"
static bool read_school(const char *line, char *code, int size) {
    int length = (int)strlen(line);
    const char *open = strrchr(line, '(');
    if (length < 3 || line[length - 1] != ')' || open == NULL) return false;

    int code_length = (int)(line + length - 1 - (open + 1));
    if (code_length < 1 || code_length >= size) return false;
    for (int i = 0; i < code_length; i++) {
        if (!isalnum((unsigned char)open[1 + i])) return false;
    }
    snprintf(code, size, "%.*s", code_length, open + 1);
    return true;
}

static bool all_digits(const char *word) {
    if (*word == '\0') return false;
    for (; *word != '\0'; word++) {
        if (!isdigit((unsigned char)*word)) return false;
    }
    return true;
}

// "12. LaMay, Ethan 1092 M 8": the row number, the name, then the bib, sex and year
static bool read_row(const char *line, Athlete *athlete) {
    char copy[256];
    char *words[32];
    int count = 0;
    snprintf(copy, sizeof(copy), "%s", line);

    char *p = copy;
    while (*p != '\0' && count < 32) {
        while (*p == ' ' || *p == '\t') *p++ = '\0';
        if (*p == '\0') break;
        words[count++] = p;
        while (*p != '\0' && *p != ' ' && *p != '\t') p++;
    }
    if (count < 5) return false;

    int length = (int)strlen(words[0]);
    if (length < 2 || words[0][length - 1] != '.') return false;
    words[0][length - 1] = '\0';
    const char *bib = words[count - 3];
    const char *sex = words[count - 2];
    const char *year = words[count - 1];
    if (!all_digits(words[0]) || !all_digits(bib) || !all_digits(year)) return false;
    if (strcmp(sex, "M") != 0 && strcmp(sex, "F") != 0) return false;

    athlete->number = atoi(words[0]);
    athlete->bib = atoll(bib);
    athlete->sex = sex[0];
    athlete->year = atoi(year);
    athlete->name[0] = '\0';
    for (int i = 1; i < count - 3; i++) {
        size_t used = strlen(athlete->name);
        int room = (int)(sizeof(athlete->name) - used);
        const char *space = " ";
        if (i == 1) space = "";
        // a name too long to keep whole is left unread, not cut (maybe mid-character)
        if (snprintf(athlete->name + used, room, "%s%s", space, words[i]) >= room) return false;
    }
    return true;
}

// Adds what the model read from one strip. Returns how many lines made no sense.
int roster_add_text(Roster *roster, const char *text) {
    int unread = 0;
    char line[256];

    while (*text != '\0') {
        int length = (int)strcspn(text, "\n");
        snprintf(line, sizeof(line), "%.*s", length, text);
        text += length;
        if (*text == '\n') text++;

        length = (int)strlen(line);
        while (length > 0 && (line[length - 1] == ' ' || line[length - 1] == '\r')) line[--length] = '\0';
        if (length == 0) continue;

        Athlete athlete = {0};
        int total = 0;
        if (strncmp(line, "Total ", 6) == 0) {
            // "Total Athletes: 298 Total Relays: 0": only the athlete count is used
            const char *athletes = strstr(line, "Total Athletes:");
            if (athletes != NULL && sscanf(athletes + strlen("Total Athletes:"), "%d", &total) == 1) {
                roster->total = total;
            }
        }
        else if (read_row(line, &athlete)) {
            if (roster->count >= MAX_ATHLETES) return unread + 1;
            snprintf(athlete.school, sizeof(athlete.school), "%s", roster->school);
            roster->athletes[roster->count] = athlete;
            roster->count++;
        }
        else if (!read_school(line, roster->school, sizeof(roster->school))) {
            unread++;
        }
    }
    return unread;
}

// Each school's rows have to come out 1, 2, 3, ... in reading order, every bib
// once, and the count has to match the roster's own total. A skipped or
// misread row breaks one of those.
bool roster_check(const Roster *roster, char *error, int size) {
    if (roster->count == 0) {
        snprintf(error, size, "No athletes were read");
        return false;
    }
    for (int i = 0; i < roster->count; i++) {
        const Athlete *athlete = &roster->athletes[i];
        int expected = 1;
        for (int j = 0; j < i; j++) {
            if (strcmp(roster->athletes[j].school, athlete->school) == 0) expected++;
            if (roster->athletes[j].bib == athlete->bib) {
                snprintf(error, size, "Bib %lld was read twice", (long long)athlete->bib);
                return false;
            }
        }
        if (athlete->school[0] == '\0') {
            snprintf(error, size, "%s has no school line above it", athlete->name);
            return false;
        }
        if (athlete->number != expected) {
            snprintf(error, size, "%s: expected row %d, read row %d (%s)",
                     athlete->school, expected, athlete->number, athlete->name);
            return false;
        }
        if (athlete->year < 1 || athlete->year > 12) {
            snprintf(error, size, "%s has year %d", athlete->name, athlete->year);
            return false;
        }
    }
    if (roster->total < 0) {
        snprintf(error, size, "The \"Total Athletes\" line was not read");
        return false;
    }
    if (roster->total != roster->count) {
        snprintf(error, size, "The roster lists %d athletes but %d were read", roster->total, roster->count);
        return false;
    }
    return true;
}

// The lookup sheet keeps names as letters and digits only: "O'Neil, Joe" -> "ONeilJoe".
// Bytes of UTF-8 characters (accents) are kept.
void roster_name(const Athlete *athlete, char *out, int size) {
    int length = 0;
    for (const char *c = athlete->name; *c != '\0' && length < size - 1; c++) {
        if (isalnum((unsigned char)*c) || (unsigned char)*c >= 0x80) out[length++] = *c;
    }
    out[length] = '\0';
}

// The roster only has grades. JV is picked by the coaches, so it is never set here.
const char *roster_division(const Athlete *athlete) {
    if (athlete->year <= 8) return "M";
    if (athlete->year == 9) return "F";
    return "V";
}

// Same layout as the "Student Lookup" sheet the timer reads: bib, name, school,
// division. xlsxiowrite_close() reports success even when the file could not be
// written, so the sheet is loaded back the way the timer loads it.
bool roster_write_xlsx(const Roster *roster, const char *path) {
    xlsxiowriter writer = xlsxiowrite_open(path, "Student Lookup");
    if (writer == NULL) return false;

    xlsxiowrite_add_column(writer, "Bib Number", 12);
    xlsxiowrite_add_column(writer, "Student Name", 32);
    xlsxiowrite_add_column(writer, "Student School", 16);
    xlsxiowrite_add_column(writer, "Division", 10);
    xlsxiowrite_next_row(writer);

    char name[64];
    for (int i = 0; i < roster->count; i++) {
        const Athlete *athlete = &roster->athletes[i];
        roster_name(athlete, name, sizeof(name));
        xlsxiowrite_add_cell_int(writer, athlete->bib);
        xlsxiowrite_add_cell_string(writer, name);
        xlsxiowrite_add_cell_string(writer, athlete->school);
        xlsxiowrite_add_cell_string(writer, roster_division(athlete));
        xlsxiowrite_next_row(writer);
    }
    xlsxiowrite_close(writer);

    static Student check[MAX_ATHLETES];
    xlsxioreader reader = xlsxioread_open(path);
    if (reader == NULL) return false;
    int count = read_lookup(reader, "Student Lookup", check, MAX_ATHLETES);
    xlsxioread_close(reader);
    if (count != roster->count) return false;
    for (int i = 0; i < count; i++) {
        if (check[i].bib != roster->athletes[i].bib) return false;
    }
    return true;
}
