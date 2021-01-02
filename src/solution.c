#include "solution.h"
#include "utils.h"


assignment *assignment_new(course *course, room *room, int day, int day_period) {
    assignment *a = mallocx(sizeof(assignment));
    assignment_set(a, course, room, day, day_period);
    return a;
}

void assignment_set(assignment *a, course *course, room *room, int day, int day_period) {
    a->course = course;
    a->room = room;
    a->day = day;
    a->day_period = day_period;
}

void solution_init(solution *sol) {
    sol->assignments = NULL;
}

void solution_destroy(solution *sol) {
    g_list_free_full(sol->assignments, g_free);
}

static char * solution_to_string_fmt(const solution *sol, const char *fmt) {
    char *buffer = NULL;
    size_t buflen;

    for (GList *p = sol->assignments; p != NULL; p = p->next) {
        const assignment *a = p->data;
        strappend_realloc(&buffer, &buflen, fmt,
                          a->course->id, a->room->id, a->day, a->day_period);
    }

    return buffer;  // must be freed outside
}


char * solution_to_string(const solution *sol) {
    char *buffer = solution_to_string_fmt(sol, "%s %s %d %d\n");
    buffer[strlen(buffer) - 1] = '\0';
    return buffer;
}

char * solution_to_string_debug(const solution *sol) {
    char *buffer = solution_to_string_fmt(sol, "(course=%s, room=%s, day=%d, day_period=%d)\n");
    buffer[strlen(buffer) - 1] = '\0';
    return buffer;

}

void solution_add_assignment(solution *sol, assignment *a) {
    sol->assignments = g_list_append(sol->assignments, a);
}

bool solution_satisfy_hard_constraints(const solution *sol) {
    return
        solution_satisfy_hard_constraint_lectures(sol) &&
        solution_satisfy_hard_constraint_room_occupancy(sol) &&
        solution_satisfy_hard_constraint_conflicts(sol) &&
        solution_satisfy_hard_constraint_availabilities(sol)
    ;
}

bool solution_satisfy_hard_constraint_lectures(const solution *sol) {
    return 0;
}

bool solution_satisfy_hard_constraint_room_occupancy(const solution *sol) {
    return 0;
}

bool solution_satisfy_hard_constraint_conflicts(const solution *sol) {
    return 0;
}

bool solution_satisfy_hard_constraint_availabilities(const solution *sol) {
    return 0;
}
