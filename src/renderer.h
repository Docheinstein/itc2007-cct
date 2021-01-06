#ifndef RENDERER_H
#define RENDERER_H

#include "model.h"
#include "solution.h"



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

void renderer_config_init(renderer_config *config);
void renderer_config_destroy(renderer_config *config);

void renderer_init(renderer *renderer);
void renderer_destroy(renderer *renderer);

bool renderer_render(renderer *renderer, const renderer_config *config,
                     const model *model, const solution *solution);
bool renderer_render_overview_timetable(
        renderer *renderer, const renderer_config *config,
        const model *model, const solution *solution);
bool renderer_render_curriculum_timetable(
        renderer *renderer, const renderer_config *config,
        const model *model, const solution *solution,
        const curricula *q);
bool renderer_render_course_timetable(
        renderer *renderer, const renderer_config *config,
        const model *model, const solution *solution,
        const course *c);
bool renderer_render_room_timetable(
        renderer *renderer, const renderer_config *config,
        const model *model, const solution *solution,
        const room *r);
bool renderer_render_teacher_timetable(
        renderer *renderer, const renderer_config *config,
        const model *model, const solution *solution,
        const teacher *t);

const char *renderer_get_error(renderer *render);

#endif // RENDERER_H