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
    // TODO free assignments
    g_list_free(sol->assignments);
}

static char * solution_dump(const solution *sol, const char *fmt) {
    char *buffer = NULL;
    size_t buflen;

    for (GList *p = sol->assignments; p != NULL; p = p->next) {
        const assignment *a = p->data;
        strappend_realloc(&buffer, &buflen, fmt,
                          a->course->id, a->room->id, a->day, a->day_period);
    }

    return buffer;  // must be freed outside
}


char * solution_print(const solution *sol) {
    return solution_dump(sol, "%s %s %d %d\n");
}

char * solution_to_string(const solution *sol) {
    return solution_dump(sol, "(course=%s, room=%s, day=%d, day_period=%d)\n");
}

void solution_add_assignment(solution *sol, assignment *a) {
    sol->assignments = g_list_append(sol->assignments, a);
}
