#include "renderer.h"
#include "cairo.h"
#include "str_utils.h"
#include "debug.h"
#include "os_utils.h"
#include "def_utils.h"
#include "array_utils.h"
#include "verbose.h"
#include "mem_utils.h"

#define WHITE 1, 1, 1
#define BLACK 0, 0, 0
#define LIGHT_GRAY 0.9, 0.9, 0.9
#define DARK_GRAY 0.6, 0.6, 0.6

#define DARK_THRESHOLD 0.2
#define LIGHT_THRESHOLD 0.8

void renderer_config_init(renderer_config *config) {
    config->output_dir = NULL;
    config->output_file = NULL;
    config->width = 1200;
    config->height = 800;
    config->font_size_medium = config->width / 70;
    config->font_size_small = config->width / 80;
}

void renderer_config_destroy(renderer_config *config) {

}

void renderer_init(renderer *renderer) {
    renderer->error = NULL;
}

void renderer_destroy(renderer *renderer) {
    free(renderer->error);
}

static void random_color(double *r, double *g, double *b) {
    double r0 = (double) rand() / RAND_MAX;
    double g0 = (double) rand() / RAND_MAX;
    double b0 = (double) rand() / RAND_MAX;
    *r = RANGIFY(DARK_THRESHOLD, r0, LIGHT_THRESHOLD);
    *g = RANGIFY(DARK_THRESHOLD, g0, LIGHT_THRESHOLD);
    *b = RANGIFY(DARK_THRESHOLD, b0, LIGHT_THRESHOLD);
}

static void random_colors(int n_colors, double **r, double **g, double **b) {
    if (!n_colors)
        return;

    *r = mallocx(n_colors, sizeof(double));
    *g = mallocx(n_colors, sizeof(double));
    *b = mallocx(n_colors, sizeof(double));

    for (int i = 0; i < n_colors; i++)
        random_color(&((*r)[i]), &((*g)[i]), &((*b)[i]));
}

static void do_draw_box_text(
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
        double xx = x + (width - extents.width) / 2;
        double yy = y + (height + extents.height) / 2;
        cairo_move_to(cr, xx, yy);
        cairo_show_text(cr, text);
    }
}

static void draw_box(renderer *renderer, cairo_t *cr,
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

static void draw_text(renderer *renderer, cairo_t *cr,
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

static void draw_box_text(
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

static void draw_line(renderer *renderer, cairo_t *cr,
               double x1, double y1, double x2, double y2,
               double line_width,
               double r, double g, double b) {
    cairo_set_source_rgb(cr, r, g, b);
    cairo_set_line_width(cr, line_width);

    cairo_move_to(cr, x1, y1);
    cairo_line_to(cr, x2, y2);
    cairo_stroke(cr);
}

static void draw_hline(renderer *renderer, cairo_t *cr,
               double x, double y, double width,
               double line_width,
               double r, double g, double b) {
    draw_line(renderer, cr, x, y, x + width, y, line_width, r, g, b);
}

static void draw_vline(renderer *renderer, cairo_t *cr,
               double x, double y, double height,
               double line_width,
               double r, double g, double b) {
    draw_line(renderer, cr, x, y, x, y + height, line_width, r, g, b);
}

static void draw_grid(renderer *renderer, cairo_t *cr,
               int width, int height,
               int cols, int rows,
               double font_size) {
    const int col_width = width / cols;
    const int row_height = height / rows;

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

static bool prologue(
    renderer *renderer, const renderer_config *config,
    const model *model, const solution *solution,
    const char *folder, const char *drawing_name,
    cairo_t **cr, cairo_surface_t **surface,
    char **path,
    int *cols, int *rows, int *col_width, int *row_height,
    int n_colors, double **r, double **g, double **b) {

    if (strempty(config->output_dir) && strempty(config->output_file)) {
        renderer->error = strmake("output directory or output file must be specified");
        return false;
    }

    if (!strempty(config->output_dir) && !strempty(config->output_file)) {
        renderer->error = strmake("only one between output directory and output file must be specified");
        return false;
    }

    if (!strempty(config->output_file)) {
        *path = config->output_file;
    } else {
        char filename[256];
        snprintf(filename, 256, "%s.png", drawing_name);

        if (folder) {
            const char * parent = pathjoin(config->output_dir, folder);

            if (!mkdirs(parent)) {
                renderer->error = strmake("directory creation failed (%s)", strerror(errno));
                return false;
            }

            *path = pathjoin(config->output_dir, folder, filename);
        } else {
            *path = pathjoin(config->output_dir, filename);
        }
    }


    *cols = model->n_days + 1;
    *rows = model->n_slots + 1;
    *col_width = config->width / *cols;
    *row_height = config->height / *rows;


    *surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
                                         config->width, config->height);
    *cr = cairo_create(*surface);

    // Background
    draw_box(renderer, *cr, 0, 0, config->width, config->height, WHITE);

    random_colors(n_colors, r, g, b);

    return true;
}

static bool epilogue(
        renderer *renderer, const renderer_config *config,
        const model *model, const solution *solution,
        cairo_t *cr, cairo_surface_t *surface,
        const char *path,
        int cols, int rows, int col_width, int row_height,
        double *r, double *g, double *b) {

    draw_grid(renderer, cr, config->width, config->height, cols, rows,
              config->font_size_medium);

    cairo_status_t status = (cairo_surface_write_to_png(surface, path));

    if (status != CAIRO_STATUS_SUCCESS) {
        debug("Cairo error occurred [%d]: %s", status, cairo_status_to_string(status));
        renderer->error = strdup(cairo_status_to_string(status));
    }

    cairo_surface_destroy(surface);
    cairo_destroy(cr);

    free(r);
    free(g);
    free(b);

    return true;
}

bool renderer_render_curriculum_timetable(renderer *renderer, const renderer_config *config,
                                          const model *model, const solution *solution,
                                          const curricula *q) {
    cairo_t *cr;
    cairo_surface_t *surface;
    int cols, rows, col_width, row_height;
    char *path;
    double *red, *green, *blue;

    if (!prologue(renderer, config, model, solution,
                  "curriculums", q->id,
                  &cr, &surface,
                  &path,
                  &cols, &rows, &col_width, &row_height,
                  model->n_courses, &red, &green, &blue))
        return false;

    debug("renderer_render_curriculum_timetable(%s) -> '%s'", q->id, path);

    int max_slot_assignments = 0;

    for (int d = 0; d < model->n_days; d++) {
        for (int s = 0; s < model->n_slots; s++) {
            int slot_assignments = 0;
            for (int c = 0; c < model->n_courses; c++) {
                if (!model_course_belongs_to_curricula(model, c, q->index))
                    continue;

                for (int r = 0; r < model->n_rooms; r++) {
                    slot_assignments += solution->timetable[
                            INDEX4(c, model->n_courses,
                                   r, model->n_rooms,
                                   d, model->n_days,
                                   s, model->n_slots)];
                }
            }
            max_slot_assignments = MAX(max_slot_assignments, slot_assignments);
        }
    }

    debug("renderer_render_curriculum_timetable max_slot_assignments = %d",
          max_slot_assignments);


    double H = ((double) row_height / max_slot_assignments);

    for (int d = 0; d < model->n_days; d++) {
        for (int s = 0; s < model->n_slots; s++) {
            int slot_index = 0;
            for (int c = 0; c < model->n_courses; c++) {
                if (!model_course_belongs_to_curricula(model, c, q->index))
                    continue;

                for (int r = 0; r < model->n_rooms; r++) {
                    if(solution->timetable[
                            INDEX4(c, model->n_courses,
                                   r, model->n_rooms,
                                   d, model->n_days,
                                   s, model->n_slots)]) {
                        double text_r, text_g, text_b;
                        if (slot_index == 0) {
                            text_r = text_g = text_b = 0;
                        } else {
                            text_r = 1;  text_g = 0; text_b = 0;
                        }
                        draw_box_text(
                                renderer, cr,
                                (d + 1) * col_width,
                                (s + 1) * row_height + H * slot_index,
                                col_width,
                                H,
                                red[c], green[c], blue[c],
                                config->font_size_small, false, false,
                                text_r, text_g, text_b,
                                "%s (%s)", model->courses[c].id, model->rooms[r].id);
                        slot_index++;
                    }
                }
            }
        }
    }

    return epilogue(renderer, config, model, solution,
                    cr, surface,
                    path,
                    cols, rows, col_width, row_height,
                    red, green, blue);
}


bool renderer_render_course_timetable(renderer *renderer, const renderer_config *config,
                                      const model *model, const solution *solution,
                                      const course *course) {
    cairo_t *cr;
    cairo_surface_t *surface;
    int cols, rows, col_width, row_height;
    char *path;
    double *red, *green, *blue;

    if (!prologue(renderer, config, model, solution,
                  "courses", course->id,
                  &cr, &surface,
                  &path,
                  &cols, &rows, &col_width, &row_height,
                  model->n_rooms, &red, &green, &blue))
        return false;

    debug("renderer_render_course_timetable(%s) -> '%s'", course->id, path);

    int max_slot_assignments = 0;

    for (int d = 0; d < model->n_days; d++) {
        for (int s = 0; s < model->n_slots; s++) {
            int slot_assignments = 0;

            for (int r = 0; r < model->n_rooms; r++) {
                slot_assignments += solution->timetable[
                        INDEX4(course->index, model->n_courses,
                               r, model->n_rooms,
                               d, model->n_days,
                               s, model->n_slots)];
            }
            max_slot_assignments = MAX(max_slot_assignments, slot_assignments);
        }
    }

    debug("renderer_render_course_timetable max_slot_assignments = %d",
          max_slot_assignments);


    double H = ((double) row_height / max_slot_assignments);

    for (int d = 0; d < model->n_days; d++) {
        for (int s = 0; s < model->n_slots; s++) {
            int slot_index = 0;

            for (int r = 0; r < model->n_rooms; r++) {
                if(solution->timetable[
                        INDEX4(course->index, model->n_courses,
                               r, model->n_rooms,
                               d, model->n_days,
                               s, model->n_slots)]) {
                    double text_r, text_g, text_b;
                    if (slot_index == 0) {
                        text_r = text_g = text_b = 0;
                    } else {
                        text_r = 1;  text_g = 0; text_b = 0;
                    }
                    draw_box_text(
                            renderer, cr,
                            (d + 1) * col_width,
                            (s + 1) * row_height + H * slot_index,
                            col_width,
                            H,
                            red[r], green[r], blue[r],
                            config->font_size_small, false, false,
                            text_r, text_g, text_b,
                            "%s", model->rooms[r].id);
                    slot_index++;
                }
            }
        }
    }

    return epilogue(renderer, config, model, solution,
                    cr, surface,
                    path,
                    cols, rows, col_width, row_height,
                    red, green, blue);
}

bool renderer_render_room_timetable(renderer *renderer, const renderer_config *config,
                                    const model *model, const solution *solution,
                                    const room *room) {
    cairo_t *cr;
    cairo_surface_t *surface;
    int cols, rows, col_width, row_height;
    char *path;
    double *red, *green, *blue;

    if (!prologue(renderer, config, model, solution,
                  "rooms", room->id,
                  &cr, &surface,
                  &path,
                  &cols, &rows, &col_width, &row_height,
                  model->n_courses, &red, &green, &blue))
        return false;

    debug("renderer_render_room_timetable(%s) -> '%s'", room->id, path);

    int max_slot_assignments = 0;

    for (int d = 0; d < model->n_days; d++) {
        for (int s = 0; s < model->n_slots; s++) {
            int slot_assignments = 0;
            for (int c = 0; c < model->n_courses; c++) {

                slot_assignments += solution->timetable[
                        INDEX4(c, model->n_courses,
                               room->index, model->n_rooms,
                               d, model->n_days,
                               s, model->n_slots)];
            }
            max_slot_assignments = MAX(max_slot_assignments, slot_assignments);
        }
    }

    debug("renderer_render_room_timetable max_slot_assignments = %d",
          max_slot_assignments);


    double H = ((double) row_height / max_slot_assignments);

    for (int d = 0; d < model->n_days; d++) {
        for (int s = 0; s < model->n_slots; s++) {
            int slot_index = 0;
            for (int c = 0; c < model->n_courses; c++) {

                if(solution->timetable[
                        INDEX4(c, model->n_courses,
                               room->index, model->n_rooms,
                               d, model->n_days,
                               s, model->n_slots)]) {
                    double text_r, text_g, text_b;
                    if (slot_index == 0) {
                        text_r = text_g = text_b = 0;
                    } else {
                        text_r = 1;  text_g = 0; text_b = 0;
                    }
                    draw_box_text(
                            renderer, cr,
                            (d + 1) * col_width,
                            (s + 1) * row_height + H * slot_index,
                            col_width,
                            H,
                            red[c], green[c], blue[c],
                            config->font_size_small, false, false,
                            text_r, text_g, text_b,
                            "%s", model->courses[c].id);
                    slot_index++;
                }
            }
        }
    }

    return epilogue(renderer, config, model, solution,
                    cr, surface,
                    path,
                    cols, rows, col_width, row_height,
                    red, green, blue);
}

bool renderer_render_teacher_timetable(renderer *renderer, const renderer_config *config,
                                       const model *model, const solution *solution,
                                       const teacher *teacher) {
    cairo_t *cr;
    cairo_surface_t *surface;
    int cols, rows, col_width, row_height;
    char *path;
    double *red, *green, *blue;

    if (!prologue(renderer, config, model, solution,
                  "teachers", teacher->id,
                  &cr, &surface,
                  &path,
                  &cols, &rows, &col_width, &row_height,
                  model->n_courses, &red, &green, &blue))
        return false;

    debug("renderer_render_teacher_timetable(%s) -> '%s'", teacher->id, path);

    int max_slot_assignments = 0;

    for (int d = 0; d < model->n_days; d++) {
        for (int s = 0; s < model->n_slots; s++) {
            int slot_assignments = 0;
            for (int c = 0; c < model->n_courses; c++) {
                if (!model_course_is_taught_by_teacher(model, c, teacher->index))
                    continue;

                for (int r = 0; r < model->n_rooms; r++) {
                    slot_assignments += solution->timetable[
                            INDEX4(c, model->n_courses,
                                   r, model->n_rooms,
                                   d, model->n_days,
                                   s, model->n_slots)];
                }
            }
            max_slot_assignments = MAX(max_slot_assignments, slot_assignments);
        }
    }

    debug("renderer_render_teacher_timetable max_slot_assignments = %d",
          max_slot_assignments);


    double H = ((double) row_height / max_slot_assignments);

    for (int d = 0; d < model->n_days; d++) {
        for (int s = 0; s < model->n_slots; s++) {
            int slot_index = 0;
            for (int c = 0; c < model->n_courses; c++) {
                if (!model_course_is_taught_by_teacher(model, c, teacher->index))
                    continue;

                for (int r = 0; r < model->n_rooms; r++) {
                    if (solution->timetable[
                            INDEX4(c, model->n_courses,
                                   r, model->n_rooms,
                                   d, model->n_days,
                                   s, model->n_slots)]) {
                        double text_r, text_g, text_b;
                        if (slot_index == 0) {
                            text_r = text_g = text_b = 0;
                        } else {
                            text_r = 1; text_g = 0; text_b = 0;
                        }
                        draw_box_text(
                                renderer, cr,
                                (d + 1) * col_width,
                                (s + 1) * row_height + H * slot_index,
                                col_width,
                                H,
                                red[c], green[c], blue[c],
                                config->font_size_small, false, false,
                                text_r, text_g, text_b,
                                "%s (%s)", model->courses[c].id, model->rooms[r].id);
                        slot_index++;
                    }
                }
            }
        }
    }

    return epilogue(renderer, config, model, solution,
                    cr, surface,
                    path,
                    cols, rows, col_width, row_height,
                    red, green, blue);
}


bool renderer_render_overview_timetable(renderer *renderer, const renderer_config *config,
                                        const model *model, const solution *solution) {
    cairo_t *cr;
    cairo_surface_t *surface;
    int cols, rows, col_width, row_height;
    char *path;
    double *red, *green, *blue;

    if (!prologue(renderer, config, model, solution,
                  NULL, "overview",
                  &cr, &surface,
                  &path,
                  &cols, &rows, &col_width, &row_height,
                  model->n_courses, &red, &green, &blue))
        return false;

    debug("renderer_render_overview_timetable -> '%s'", path);

    int max_slot_assignments = 0;

    for (int d = 0; d < model->n_days; d++) {
        for (int s = 0; s < model->n_slots; s++) {
            int slot_assignments = 0;
            for (int c = 0; c < model->n_courses; c++) {
                for (int r = 0; r < model->n_rooms; r++) {
                    slot_assignments += solution_get_at(solution, c, r, d, s);
                }
            }
            max_slot_assignments = MAX(max_slot_assignments, slot_assignments);
        }
    }

    debug("renderer_render_overview_timetable max_slot_assignments = %d",
          max_slot_assignments);
    double H = ((double) row_height / max_slot_assignments);

    for (int d = 0; d < model->n_days; d++) {
        for (int s = 0; s < model->n_slots; s++) {
            int slot_index = 0;
            for (int c = 0; c < model->n_courses; c++) {
                for (int r = 0; r < model->n_rooms; r++) {
                    if(solution->timetable[
                            INDEX4(c, model->n_courses,
                                   r, model->n_rooms,
                                   d, model->n_days,
                                   s, model->n_slots)]) {
                        draw_box_text(
                                renderer, cr,
                                (d + 1) * col_width,
                                (s + 1) * row_height + H * slot_index,
                                col_width,
                                H,
                                red[c], green[c], blue[c],
                                config->font_size_small, false, false,
                                BLACK,
                                "%s (%s)", model->courses[c].id, model->rooms[r].id);
                        slot_index++;
                    }
                }
            }
        }
    }

    return epilogue(renderer, config, model, solution,
                    cr, surface,
                    path,
                    cols, rows, col_width, row_height,
                    red, green, blue);
}


bool renderer_render(renderer *renderer, const renderer_config *config,
                     const model *model, const solution *solution) {
    if (strempty(config->output_dir) && strempty(config->output_file)) {
        renderer->error = strmake("output directory or output file must be specified");
        return false;
    }

    if (!strempty(config->output_dir) && !strempty(config->output_file)) {
        renderer->error = strmake("only one between output directory and output file must be specified");
        return false;
    }

    if (!strempty(config->output_dir)) {
        for (int q = 0; q < model->n_curriculas; q++) {
            verbose("Rendering timetable of curriculum '%s'", model->curriculas[q].id);
            if (!renderer_render_curriculum_timetable(renderer, config, model,
                                                      solution, &model->curriculas[q]))
                return false;
        }
        for (int c = 0; c < model->n_courses; c++) {
            verbose("Rendering timetable of course '%s'", model->courses[c].id);
            if (!renderer_render_course_timetable(renderer, config, model,
                                                  solution, &model->courses[c]))
                return false;
        }
        for (int r = 0; r < model->n_rooms; r++) {
            verbose("Rendering timetable of room '%s'", model->rooms[r].id);
            if (!renderer_render_room_timetable(renderer, config, model,
                                                solution, &model->rooms[r]))
                return false;
        }
        for (int t = 0; t < model->n_teachers; t++) {
            verbose("Rendering timetable of teacher '%s'", model->teachers[t].id);
            if (!renderer_render_teacher_timetable(renderer, config, model,
                                                   solution, &model->teachers[t]))
                return false;
        }
    }

    verbose("Rendering overview timetable");
    renderer_render_overview_timetable(renderer, config, model, solution);

    return strempty(renderer->error);
}

const char *renderer_get_error(renderer *renderer) {
    return renderer->error;
}