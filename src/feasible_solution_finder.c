#include "feasible_solution_finder.h"
#include "str_utils.h"
#include "debug.h"
#include "array_utils.h"
#include "mem_utils.h"
#include <stdlib.h>

void feasible_solution_finder_init(feasible_solution_finder *finder) {
    finder->error = NULL;
}

void feasible_solution_finder_destroy(feasible_solution_finder *finder) {
    free(finder->error);
}

typedef struct course_assignment {
    course *course;
    int difficulty;
} course_assignment;

int course_assignment_compare(const void *ca1, const void *ca2) {
    return
        ((const course_assignment *) ca1)->difficulty <
        ((const course_assignment *) ca2)->difficulty;
}

bool feasible_solution_finder_find(feasible_solution_finder *finder,
                                   solution *sol) {
    const int C = sol->model->n_courses;
    const int R = sol->model->n_rooms;
    const int D = sol->model->n_days;
    const int S = sol->model->n_slots;
    const int Q = sol->model->n_curriculas;
    const int T = sol->model->n_teachers;

    const model *m = sol->model;

    bool *room_is_used = callocx(R * D * S, sizeof(bool));
    bool *teacher_is_busy = callocx(T * D * S, sizeof(bool));
    bool *curriculum_is_assigned = callocx(Q * D * S, sizeof(bool));

    int required_assignments = 0;
    for (int c = 0; c < C; c++)
        required_assignments += m->courses[c].n_lectures;

    debug("feasible_solution_finder_find: %d assignments to do", required_assignments);

    int n_assignments = 0;
    int n_attempts = 0;

    // Sort the courses by the difficulty, in order to assign the most
    // difficult courses before the easy ones (i.e. a course is difficult
    // it place if it has many associated constraints).
    // Specifically:
    // 1. H3a: How many curriculum does the course belong to?
    // 2. H3b: How many courses the teacher of the course teaches?
    // 3. H4: How many unavailability constraint the course has?

    // 1. H3a: How many curriculum does the course belong to?
    int *courses_n_curriculas = callocx(C, sizeof(int));
    for (int c = 0; c < C; c++) {
        model_curriculas_of_course(m, c, &courses_n_curriculas[c]);
        debug("Course %s belongs to %d curriculas",
              m->courses[c].id, courses_n_curriculas[c]);
    }

    // 2. H3b: How many courses the teacher of the course teaches?
    int *courses_teacher_n_courses = callocx(C, sizeof(int));
    for (int c = 0; c < C; c++) {
        model_courses_of_teacher(m, model_teacher_by_id(m, m->courses[c].teacher_id)->index,
                                 &courses_teacher_n_courses[c]);
        debug("Course %s has teacher %s which teaches %d courses",
              m->courses[c].id, m->courses[c].teacher_id, courses_teacher_n_courses[c]);
    }

    // 3. H4: How many unavailability constraint the course has?
    int *courses_n_unavailabilities = callocx(C, sizeof(int));
    for (int c = 0; c < C; c++) {
        for (int d = 0; d < D; d++) {
            for (int s = 0; s < S; s++) {
                courses_n_unavailabilities[c] += !model_course_is_available_on_period(m, c, d, s);
            }
        }
        debug("Course %s has %d unavailabilities constraints",
              m->courses[c].id, courses_n_unavailabilities[c]);
    }

    // Compute the difficulty score of courses assignments
    static const int CURRICULAS_CONFLICTS_DIFFICULTY_FACTOR = 1;
    static const int TEACHER_CONFLICTS_DIFFICULTY_FACTOR = 1;
    static const int UNAVAILABILITY_CONFLICTS_DIFFICULTY_FACTOR = 1;
    static const int COURSE_LECTURES_DIFFICULTY_FACTOR = 1;

    course_assignment *courses_assignment_difficulty = mallocx(C, sizeof(course_assignment));

    for (int c = 0; c < C; c++) {
        courses_assignment_difficulty[c].course = &m->courses[c];
        courses_assignment_difficulty[c].difficulty =
            (courses_n_curriculas[c] * CURRICULAS_CONFLICTS_DIFFICULTY_FACTOR +
            courses_teacher_n_courses[c] * UNAVAILABILITY_CONFLICTS_DIFFICULTY_FACTOR +
            courses_n_unavailabilities[c] * UNAVAILABILITY_CONFLICTS_DIFFICULTY_FACTOR)
            * MAX(1, courses_assignment_difficulty[c].course->n_lectures * COURSE_LECTURES_DIFFICULTY_FACTOR);
        debug("Course %s has assignment difficulty %d",
              courses_assignment_difficulty[c].course->id,
              courses_assignment_difficulty[c].difficulty);
    }

    qsort(courses_assignment_difficulty, C, sizeof(course_assignment),
          course_assignment_compare);

    for (int c = 0; c < C; c++) {
        debug("POST sort: course %s has assignment difficulty %d",
              courses_assignment_difficulty[c].course->id,
              courses_assignment_difficulty[c].difficulty);
    }

    for (int i = 0; i < C; i++) {
        const course *course = courses_assignment_difficulty[i].course;
        const int c = course->index;

        int course_n_curriculas;
        int *course_curriculas = model_curriculas_of_course(m, c, &course_n_curriculas);

        int lectures_to_assign = course->n_lectures;

        while (lectures_to_assign) {
            debug("%s: %d lectures not assigned yet", course->id, lectures_to_assign);
            int iter_assignments = 0;

            for (int r = 0; r < R && lectures_to_assign; r++) {
                const room *room = &m->rooms[r];
                for (int d = 0; d < D && lectures_to_assign;  d++) {
                    for (int s = 0; s < S /* && lectures_to_assign */; s++) {
                        debug("\t%s in room %s on (day=%d, slot=%d)?", course->id, room->id, d, s);
                        n_attempts++;
                        // Check hard constraints

                        // H2: RoomOccupancy
                        if (room_is_used[INDEX3(r, sol->R, d, D, s, S)]) {
                            debug("\t\tfailed (RoomOccupancy: %s)", room->id);
                            continue;
                        }

                        // H3a: Conflicts (Curriculum)
                        int curriculum_conflict = -1;
                        for (int cq = 0 ; cq < course_n_curriculas; cq++) {
                            if (curriculum_is_assigned[INDEX3(course_curriculas[cq], Q, d, D, s, S)]) {
                                curriculum_conflict = cq;
                                break;
                            }
                        }
                        if (curriculum_conflict >= 0) {
                            debug("\t\tfailed (CurriculumConflict: %s)",
                                  m->curriculas[course_curriculas[curriculum_conflict]].id);
                            continue;
                        }

                        // H3b: Conflicts (Teacher)
                        int t = model_teacher_by_id(m, course->teacher_id)->index;

                        if (teacher_is_busy[INDEX3(t, T, d, D, s, S)]) {
                            debug("\t\tfailed (TeacherConflict %s)", m->teachers[t].id);
                            continue;
                        }

                        // H4: availabilities
                        if (!model_course_is_available_on_period(m, c, d, s)) {
                            debug("\t\tfailed (Availabilities (%s, %d, %d))", course->id, d, s);
                            continue;
                        }

                        debug(" -> OK!");
                        room_is_used[INDEX3(r, R, d, D, s, S)] = true;
                        teacher_is_busy[INDEX3(t, T, d, D, s, S)] = true;
                        for (int cq = 0 ; cq < course_n_curriculas; cq++) {
                            curriculum_is_assigned[INDEX3(
                                    course_curriculas[cq], Q, d, D, s, S)] =  true;
                        }

                        solution_set_at(sol, c, r, d, s, true);
                        lectures_to_assign--;
                        iter_assignments++;
                        n_assignments++;
                        break;
                    }
                }
            }

            if (!iter_assignments) {
                debug("Can't find a feasible assignment for course %s (%d attempts, %d/%d assignments done)",
                      course->id, n_attempts, n_assignments, required_assignments);
                finder->error = strmake("can't find a feasible solution, done %d/%d assignments",
                                        n_assignments, required_assignments);
                return false;
            }
        }
    }

    debug("All assignments done; %d attempts for %d assignments",
          n_attempts, n_assignments);

    return strempty(finder->error);
}

const char *feasible_solution_finder_find_get_error(feasible_solution_finder *finder) {
    return finder->error;
}
