#include <pdf.h>
#include <data.h>
#include <stdio.h>

static void draw_cell(struct pdf_doc *pdf, struct pdf_object *page,
                      float x, float y, float width, float height,
                      const char *text, uint32_t fill)
{
    pdf_add_filled_rectangle(pdf, page, x, y, width, height,
                             0.6f, fill, PDF_BLACK);
    pdf_add_text(pdf, page, text, 10, x + 4, y + 4, PDF_BLACK);
}

static void draw_header(struct pdf_doc *pdf, struct pdf_object *page,
                        float *col_x, float *col_w, float bottom)
{
    const char *titles[6] = {"Rank", "Bib", "Name", "School", "Division", "Time"};

    pdf_set_font(pdf, "Helvetica-Bold");
    for (int c = 0; c < 6; c++) {
        draw_cell(pdf, page, col_x[c], bottom, col_w[c], 18,
                  titles[c], PDF_RGB(220, 220, 220));
    }
    pdf_set_font(pdf, "Helvetica");
}

static void draw_climate(struct pdf_doc *pdf, struct pdf_object *page,
                         const Climate *climate, float right, float top)
{
    const char *titles[5] = {"Temperature (F)", "Wind (mph)", "Wind Direction",
                             "Rainfall (in)", "Clouds (0-10)"};
    float col_w[5] = {84, 64, 80, 64, 72};
    float col_x[5];
    col_x[0] = right - (col_w[0] + col_w[1] + col_w[2] + col_w[3] + col_w[4]);
    for (int c = 1; c < 5; c++) {
        col_x[c] = col_x[c - 1] + col_w[c - 1];
    }

    // clouds is -1 until it is entered. 0 is a real reading.
    char clouds_text[16] = CLIMATE_UNKNOWN;
    if (climate->clouds >= 0) {
        snprintf(clouds_text, sizeof(clouds_text), "%d", climate->clouds);
    }
    char wind_text[20] = CLIMATE_UNKNOWN;
    snprintf(wind_text, sizeof(wind_text), "%s", climate->wind);
    const char *values[5] = {climate->tempreture, wind_text, climate->windDirection,
                             climate->precipitation, clouds_text};

    pdf_set_font(pdf, "Helvetica-Bold");
    for (int c = 0; c < 5; c++) {
        draw_cell(pdf, page, col_x[c], top - 14, col_w[c], 14,
                  titles[c], PDF_RGB(220, 220, 220));
    }
    pdf_set_font(pdf, "Helvetica");
    for (int c = 0; c < 5; c++) {
        draw_cell(pdf, page, col_x[c], top - 28, col_w[c], 14,
                  values[c], PDF_WHITE);
    }
}

// width of the widest school name, so the standings columns line up
static float school_name_width(struct pdf_doc *pdf, const Result *result)
{
    float widest = 0;
    for (int s = 0; s < result->school_count; s++) {
        float width = 0;
        pdf_get_font_text_width(pdf, "Helvetica", result->schools[s].name, 12, &width);
        if (width > widest) widest = width;
    }
    return widest;
}

static float draw_standings(struct pdf_doc *pdf, struct pdf_object **page,
                            const Result *result, const char *heading,
                            float x, float school_w, float y, float page_h)
{
    float school_x = x + 64;
    float score_x = school_x + school_w + 16;
    float places_x = score_x + 40;

    for (int s = 0; s < result->school_count; s++) {
        const School *school = &result->schools[s];

        if (y - 18 < 36) {
            *page = pdf_append_page(pdf);
            if (*page == NULL) return -1;
            y = page_h - 36;
        }

        if (s == 0 && heading != NULL) {
            pdf_set_font(pdf, "Helvetica-Bold");
            pdf_add_text(pdf, *page, heading, 12, 36, y - 12, PDF_BLACK);
        }

        const char *label = "Finisher:";
        if (s == 0) label = "Winner:";

        char score_text[16] = "";
        char places_text[64];
        if (school->runners < 5) {
            label = "No score:";
            snprintf(places_text, sizeof(places_text), "only %d finished, 5 needed",
                     school->runners);
        } else {
            snprintf(score_text, sizeof(score_text), "%d", school->score);
            snprintf(places_text, sizeof(places_text), "%d + %d + %d + %d + %d",
                     school->cumulative[0], school->cumulative[1], school->cumulative[2],
                     school->cumulative[3], school->cumulative[4]);
        }

        pdf_set_font(pdf, "Helvetica");
        pdf_add_text(pdf, *page, label, 12, x, y - 12, PDF_BLACK);
        pdf_add_text(pdf, *page, school->name, 12, school_x, y - 12, PDF_BLACK);
        pdf_add_text(pdf, *page, places_text, 12, places_x, y - 12, PDF_BLACK);
        pdf_set_font(pdf, "Helvetica-Bold");
        pdf_add_text(pdf, *page, score_text, 12, score_x, y - 12, PDF_BLACK);

        y = y - 18;
    }
    pdf_set_font(pdf, "Helvetica");
    return y;
}

int write_division_pdf(const char *path, Row rows[], int row_count,
                       const Result *result,
                       const char *division, const Climate *climate)
{
    struct pdf_info info = {
        .creator = "chimbiwide",
        .producer = "SJ",
        .title = "Meet Results",
        .author = "chimbiwide",
        .subject = "Race results",
        .date = ""
    };

    /* Sideways letter: wide, like a spreadsheet. */
    struct pdf_doc *pdf = pdf_create(PDF_LETTER_HEIGHT, PDF_LETTER_WIDTH, &info);
    if (pdf == NULL) return -1;

    float col_w[6] = {48, 56, 240, 200, 56, 120};
    float col_x[6];
    col_x[0] = 36;
    for (int c = 1; c < 6; c++) {
        col_x[c] = col_x[c - 1] + col_w[c - 1];
    }

    float page_h = PDF_LETTER_WIDTH;
    float y = page_h - 36;

    struct pdf_object *page = pdf_append_page(pdf);
    if (page == NULL) {
        pdf_destroy(pdf);
        return -1;
    }

    char title[64];
    snprintf(title, sizeof(title), "%s Results", division);
    pdf_set_font(pdf, "Helvetica-Bold");
    pdf_add_text(pdf, page, title, 16, 36, y - 16, PDF_BLACK);
    draw_climate(pdf, page, climate, col_x[5] + col_w[5], y);
    y = y - 34;

    y = draw_standings(pdf, &page, result, NULL, 36,
                       school_name_width(pdf, result), y, page_h);
    if (y < 0) {
        pdf_destroy(pdf);
        return -1;
    }
    y = y - 10;

    // the table header and its first row have to fit under the standings
    if (y - 36 < 36) {
        page = pdf_append_page(pdf);
        if (page == NULL) {
            pdf_destroy(pdf);
            return -1;
        }
        y = page_h - 36;
    }

    draw_header(pdf, page, col_x, col_w, y - 18);
    y = y - 18;

    for (int i = 0; i < row_count; i++) {
        float bottom = y - 18;
        if (bottom < 36) {
            page = pdf_append_page(pdf);
            if (page == NULL) {
                pdf_destroy(pdf);
                return -1;
            }
            y = page_h - 36;
            draw_header(pdf, page, col_x, col_w, y - 18);
            y = y - 18;
            bottom = y - 18;
        }

        char rank_text[16];
        char bib_text[16];
        snprintf(rank_text, sizeof(rank_text), "%d", rows[i].rank);
        if (rows[i].bib > 0) {
            snprintf(bib_text, sizeof(bib_text), "%ld", (long)rows[i].bib);
        } else {
            bib_text[0] = '\0';
        }

        uint32_t fill = PDF_WHITE;
        if ((i % 2) == 1) fill = PDF_RGB(245, 245, 245);

        draw_cell(pdf, page, col_x[0], bottom, col_w[0], 18, rank_text, fill);
        draw_cell(pdf, page, col_x[1], bottom, col_w[1], 18, bib_text, fill);
        draw_cell(pdf, page, col_x[2], bottom, col_w[2], 18, rows[i].name, fill);
        draw_cell(pdf, page, col_x[3], bottom, col_w[3], 18, rows[i].school, fill);
        draw_cell(pdf, page, col_x[4], bottom, col_w[4], 18, rows[i].division, fill);
        draw_cell(pdf, page, col_x[5], bottom, col_w[5], 18, rows[i].time, fill);

        y = bottom;
    }

    int saved = pdf_save(pdf, path);
    pdf_destroy(pdf);
    return saved;
}

int write_results_pdf(const char *path, Row rows[], int row_count, Result results[],
                      const Climate *climate) {
    struct pdf_info info = {
        .creator = "chimbiwide",
        .producer = "SJ",
        .title = "Meet Results",
        .author = "chimbiwide",
        .subject = "Race results",
        .date = ""
    };

    /* Sideways letter: wide, like a spreadsheet. */
    struct pdf_doc *pdf = pdf_create(PDF_LETTER_HEIGHT, PDF_LETTER_WIDTH, &info);
    if (pdf == NULL) return -1;

    float col_w[6] = {48, 56, 240, 200, 56, 120};
    float col_x[6];
    col_x[0] = 36;
    for (int c = 1; c < 6; c++) {
        col_x[c] = col_x[c - 1] + col_w[c - 1];
    }

    float page_h = PDF_LETTER_WIDTH;
    float y = page_h - 36;

    struct pdf_object *page = pdf_append_page(pdf);
    if (page == NULL) {
        pdf_destroy(pdf);
        return -1;
    }

    pdf_set_font(pdf, "Helvetica-Bold");
    pdf_add_text(pdf, page, "Meet Results", 16, 36, y - 16, PDF_BLACK);
    draw_climate(pdf, page, climate, col_x[5] + col_w[5], y);
    y = y - 34;

    // the same school column width for every division, so they all line up
    float school_w = 0;
    for (int division = 0; division < DIV_COUNT; division++) {
        float width = school_name_width(pdf, &results[division]);
        if (width > school_w) school_w = width;
    }

    // the standings of each division that ran, with its name beside them
    for (int division = 0; division < DIV_COUNT; division++) {
        if (results[division].division[0] == '\0') continue;

        y = draw_standings(pdf, &page, &results[division], division_name(division),
                           36 + 72, school_w, y, page_h);
        if (y < 0) {
            pdf_destroy(pdf);
            return -1;
        }
        y = y - 6;
    }
    y = y - 4;

    // the table header and its first row have to fit under the standings
    if (y - 36 < 36) {
        page = pdf_append_page(pdf);
        if (page == NULL) {
            pdf_destroy(pdf);
            return -1;
        }
        y = page_h - 36;
    }

    draw_header(pdf, page, col_x, col_w, y - 18);
    y = y - 18;

    for (int i = 0; i < row_count; i++) {
        float bottom = y - 18;
        if (bottom < 36) {
            page = pdf_append_page(pdf);
            if (page == NULL) {
                pdf_destroy(pdf);
                return -1;
            }
            y = page_h - 36;
            draw_header(pdf, page, col_x, col_w, y - 18);
            y = y - 18;
            bottom = y - 18;
        }

        char rank_text[16];
        char bib_text[16];
        snprintf(rank_text, sizeof(rank_text), "%d", rows[i].rank);
        if (rows[i].bib > 0) {
            snprintf(bib_text, sizeof(bib_text), "%ld", (long)rows[i].bib);
        } else {
            bib_text[0] = '\0';
        }

        uint32_t fill = PDF_WHITE;
        if ((i % 2) == 1) fill = PDF_RGB(245, 245, 245);

        draw_cell(pdf, page, col_x[0], bottom, col_w[0], 18, rank_text, fill);
        draw_cell(pdf, page, col_x[1], bottom, col_w[1], 18, bib_text, fill);
        draw_cell(pdf, page, col_x[2], bottom, col_w[2], 18, rows[i].name, fill);
        draw_cell(pdf, page, col_x[3], bottom, col_w[3], 18, rows[i].school, fill);
        draw_cell(pdf, page, col_x[4], bottom, col_w[4], 18, rows[i].division, fill);
        draw_cell(pdf, page, col_x[5], bottom, col_w[5], 18, rows[i].time, fill);

        y = bottom;
    }

    int saved = pdf_save(pdf, path);
    pdf_destroy(pdf);
    return saved;
}
