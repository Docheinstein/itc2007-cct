#ifndef RENDERER_H
#define RENDERER_H

#include "solution/solution.h"

/*
 * Graphical renderer of a solution.
 * Creates one or multiple timetables (.png).
 */
typedef struct renderer_config {
    char *output_dir;
    char *output_file;
    int width;
    int height;
    int font_size_medium;
    int font_size_small;
} renderer_config;

typedef struct renderer {
    char *error;
} renderer;

/*
 * Generate only an overview timetable.
 * All the courses, rooms, days, slots are fitted in one table.
 **/
bool render_solution_overview(const solution *sol, char *overview_file);

/*
 * Generate multiple timetables (by room, by course, by curricula, by teacher).
 */
bool render_solution_full(const solution *sol, char *output_dir);

void renderer_config_default(renderer_config *config);

void renderer_init(renderer *renderer);
void renderer_destroy(renderer *renderer);

bool renderer_render(
        renderer *renderer, const renderer_config *config,
        const solution *solution);
bool renderer_render_overview_timetable(
        renderer *renderer, const renderer_config *config,
        const solution *solution);
bool renderer_render_curriculum_timetable(
        renderer *renderer, const renderer_config *config,
        const solution *solution, const curricula *q);
bool renderer_render_course_timetable(
        renderer *renderer, const renderer_config *config,
        const solution *solution, const course *c);
bool renderer_render_room_timetable(
        renderer *renderer, const renderer_config *config,
        const solution *solution, const room *r);
bool renderer_render_teacher_timetable(
        renderer *renderer, const renderer_config *config,
        const solution *solution, const teacher *t);

const char *renderer_get_error(renderer *render);

#endif // RENDERER_H