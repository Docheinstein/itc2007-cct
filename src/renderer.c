#include "renderer.h"
#include "cairo.h"
#include "str_utils.h"
#include "debug.h"

#define WHITE 1, 1, 1
#define BLACK 0, 0, 0
#define LIGHT_GRAY 0.9, 0.9, 0.9
#define DARK_GRAY 0.6, 0.6, 0.6

void renderer_config_init(renderer_config *config) {
    config->output = NULL;
}

void renderer_config_destroy(renderer_config *config) {

}

void renderer_init(renderer *renderer) {
    renderer->error = NULL;
}

void renderer_destroy(renderer *renderer) {
    free(renderer->error);
}

void do_draw_box_text(
        renderer *renderer, cairo_t *cr,
        double x, double y, double width, double height,
        bool fill_box,
        double bg_r, double bg_g, double bg_b,
        double font_size, bool italic, bool bold,
        double text_r, double text_g, double text_b,
        const char *text) {
    if (fill_box) {
        cairo_rectangle(cr, x, y, width, height);
        cairo_set_source_rgb(cr, bg_r, bg_g, bg_b);
        cairo_fill(cr);
    }

    if (!strempty(text)) {
        cairo_text_extents_t extents;

        cairo_select_font_face(cr, "Sans",
                           italic ? CAIRO_FONT_SLANT_ITALIC : CAIRO_FONT_SLANT_NORMAL,
                           bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, font_size);
        cairo_set_source_rgb(cr, text_r, text_g, text_b);

        cairo_text_extents(cr, text, &extents);
//        double xx = x + (width - extents.width) / 2;
//        double yy = y + (height - extents.height) / 2;
        double xx = x + (width - extents.width) / 2;
        double yy = y + (height + extents.height) / 2;
        cairo_move_to(cr, xx, yy);
        cairo_show_text(cr, text);
    }
}

void draw_box(renderer *renderer, cairo_t *cr,
              double x, double y, double width, double height,
              double bg_r, double bg_g, double bg_b) {
    do_draw_box_text(
        renderer, cr,
        x, y, width, height,
        true,
        bg_r, bg_g, bg_b,
        0, false, false,
        0, 0, 0,
        NULL);
}

void draw_text(renderer *renderer, cairo_t *cr,
               double x, double y, double width, double height,
               double font_size, bool italic, bool bold,
               double text_r, double text_g, double text_b,
               const char *text, ...) {
    static const int BUFLEN = 64;
    char buffer[BUFLEN];

    va_list args;
    va_start(args, text);
    vsnprintf(buffer, BUFLEN, text, args);

    do_draw_box_text(
        renderer, cr,
        x, y, width, height,
        false,
        0, 0, 0,
        font_size, italic, bold,
        text_r, text_g, text_b,
        buffer);

    va_end(args);
}

void draw_box_text(
        renderer *renderer, cairo_t *cr,
        double x, double y, double width, double height,
        double bg_r, double bg_g, double bg_b,
        double font_size, bool italic, bool bold,
        double text_r, double text_g, double text_b,
        const char *text, ...) {
    static const int BUFLEN = 64;
    char buffer[BUFLEN];

    va_list args;
    va_start(args, text);
    vsnprintf(buffer, BUFLEN, text, args);

    do_draw_box_text(
        renderer, cr,
        x, y, width, height,
        true,
        bg_r, bg_g, bg_b,
        font_size, italic, bold,
        text_r, text_g, text_b,
        buffer);

    va_end(args);
}

void draw_line(renderer *renderer, cairo_t *cr,
               double x1, double y1, double x2, double y2,
               double line_width,
               double r, double g, double b) {
    cairo_set_source_rgb(cr, r, g, b);
    cairo_set_line_width(cr, line_width);

    cairo_move_to(cr, x1, y1);
    cairo_line_to(cr, x2, y2);
    cairo_stroke(cr);
}

void draw_hline(renderer *renderer, cairo_t *cr,
               double x, double y, double width,
               double line_width,
               double r, double g, double b) {
    draw_line(renderer, cr, x, y, x + width, y, line_width, r, g, b);
}

void draw_vline(renderer *renderer, cairo_t *cr,
               double x, double y, double height,
               double line_width,
               double r, double g, double b) {
    draw_line(renderer, cr, x, y, x, y + height, line_width, r, g, b);
}

void draw_frame(renderer *renderer, cairo_t *cr,
                int width, int height,
                int cols, int rows,
                double font_size) {
    const int col_width = width / cols;
    const int row_height = height / rows;

    // Background
    draw_box(renderer, cr, 0, 0, width, height, WHITE);

    // Days
    for (int c = 1; c < cols; c++) {
        int x = c * col_width;

        draw_box_text(renderer, cr,
                      x, 0, col_width, row_height, LIGHT_GRAY,
                      font_size, false, true,
                      BLACK,
                      "Day %d", c - 1);
    }

    // Slots
    for (int r = 1; r < rows; r++) {
        int y = r * row_height;

        draw_box_text(renderer, cr,
                      0, y, col_width, row_height, LIGHT_GRAY,
                      font_size, false, true,
                      BLACK,
                      "Slot %d", r - 1);
    }

    // Grid
    debug("Creating %dx%d grid", cols, rows);

    // Grid's columns
    for (int c = 0; c < cols; c++) {
        int x = c * col_width;
        draw_vline(renderer, cr, x, 0, height, 1.0, BLACK);
    }

    // Grid's rows
    for (int r = 0; r < rows; r++) {
        int y = r * row_height;
        draw_hline(renderer, cr, 0, y, width, 1.0, BLACK);
    }

}
bool renderer_render(renderer *renderer, const renderer_config *config,
                     const model *model, const solution *solution) {
    if (strempty(config->output)) {
        renderer->error = strmake("output file not specified");
        return false;
    }

    const char *filename = strmake("%s%s%s",
                                   config->output,
                                   strends(config->output, '/') ? "" : "/",
                                   "drawing.png");

    debug("Renderer output file: '%s'", filename);

    static const double FONT_SIZE = 22.0;
    static const int WIDTH = 1200;
    static const int HEIGHT = 800;
    const int COLS = model->n_days + 1;
    const int ROWS = model->n_slots + 1;

    cairo_surface_t *surface;
    cairo_t *cr;

    surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, WIDTH, HEIGHT);
    cr = cairo_create(surface);

    draw_frame(renderer, cr, WIDTH, HEIGHT, COLS, ROWS, FONT_SIZE);

    cairo_status_t status = (cairo_surface_write_to_png(surface, filename));

    if (status != CAIRO_STATUS_SUCCESS) {
        debug("Cairo error occurred [%d]: %s", status, cairo_status_to_string(status));
        renderer->error = strdup(cairo_status_to_string(status));
    }

    cairo_destroy(cr);

    return strempty(renderer->error);
}

const char *renderer_get_error(renderer *renderer) {
    return renderer->error;
}