#include <pdf.h>
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
    const char *titles[5] = {"Rank", "Bib", "Name", "School", "Time"};

    pdf_set_font(pdf, "Helvetica-Bold");
    for (int c = 0; c < 5; c++) {
        draw_cell(pdf, page, col_x[c], bottom, col_w[c], 18,
                  titles[c], PDF_RGB(220, 220, 220));
    }
    pdf_set_font(pdf, "Helvetica");
}

int write_results_pdf(const char *path, Row rows[], int row_count,
                      const char *winner, int winner_score,
                      const char *loser, int loser_score,
                      const char *division)
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

    float col_w[5] = {48, 56, 240, 200, 176};
    float col_x[5];
    col_x[0] = 36;
    for (int c = 1; c < 5; c++) {
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
    y = y - 28;

    char summary[128];
    snprintf(summary, sizeof(summary),
             "Win: %s    %d        Lose: %s    %d",
             winner, winner_score, loser, loser_score);
    pdf_set_font(pdf, "Helvetica");
    pdf_add_text(pdf, page, summary, 12, 36, y - 12, PDF_BLACK);
    y = y - 28;

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
        draw_cell(pdf, page, col_x[4], bottom, col_w[4], 18, rows[i].time, fill);

        y = bottom;
    }

    int saved = pdf_save(pdf, path);
    pdf_destroy(pdf);
    return saved;
}
