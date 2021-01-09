#include "feasible_solution_finder.h"
#include "utils/str_utils.h"
#include "debug.h"
#include "utils/array_utils.h"
#include "utils/mem_utils.h"
#include "verbose.h"
#include <stdlib.h>
#include <utils/io_utils.h>
#include <utils/random_utils.h>

//#ifdef debug
//#undef debug
//#define debug(format, ...)
//#endif

void feasible_solution_finder_config_init(feasible_solution_finder_config *config) {
    config->difficulty_ranking_randomness = 0; // deterministic
}

void feasible_solution_finder_config_destroy(feasible_solution_finder_config *config) {

}

void feasible_solution_finder_init(feasible_solution_finder *finder) {
    finder->error = NULL;
}

void feasible_solution_finder_destroy(feasible_solution_finder *finder) {
    free(finder->error);
}

static void feasible_solution_finder_reinit(feasible_solution_finder *finder) {
    feasible_solution_finder_init(finder);
    feasible_solution_finder_destroy(finder);
}

typedef struct course_assignment {
    const course *course;
    int difficulty;
    double difficulty_ranking_factor;
} course_assignment;

int course_assignment_compare(const void *_1, const void *_2) {
    const course_assignment * ca1 = (const course_assignment *) _1;
    const course_assignment * ca2 = (const course_assignment *) _2;
    return
        (ca1->difficulty * ca1->difficulty_ranking_factor) <
        (ca2->difficulty * ca2->difficulty_ranking_factor);
}

bool feasible_solution_finder_find(feasible_solution_finder *finder,
                                   feasible_solution_finder_config *config,
                                   solution *sol) {

    feasible_solution_finder_reinit(finder);

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


    // Compute the difficulty score of courses assignments
    static const int CURRICULAS_CONFLICTS_DIFFICULTY_FACTOR = 1;
    static const int TEACHER_CONFLICTS_DIFFICULTY_FACTOR = 1;
    static const int UNAVAILABILITY_CONFLICTS_DIFFICULTY_FACTOR = 1;
    static const int COURSE_LECTURES_DIFFICULTY_FACTOR = 1;

    course_assignment *courses_assignment_difficulty = mallocx(C, sizeof(course_assignment));

    for (int c = 0; c < C; c++) {
        const course *course = &m->courses[c];
        // 1. H3a: How many curriculum does the course belong to?
        int n_curriculas;
        model_curriculas_of_course(m, c, &n_curriculas);
        debug("Course %s belongs to %d curriculas",  course->id, n_curriculas);


        // 2. H3b: How many courses the teacher of the course teaches?
        int n_teacher_courses;
        model_courses_of_teacher(m, model_teacher_by_id(m, course->teacher_id)->index,
                                 &n_teacher_courses);
        debug("Course %s has teacher %s which teaches %d courses",
              course->id, m->courses[c].teacher_id, n_teacher_courses);

        // 3. H4: How many unavailability constraint the course has?
        int n_unavailabilities = 0;
        for (int d = 0; d < D; d++) {
            for (int s = 0; s < S; s++) {
                n_unavailabilities += !model_course_is_available_on_period(m, c, d, s);
            }
        }
        debug("Course %s has %d unavailabilities constraints",
              m->courses[c].id, n_unavailabilities);

        course_assignment *ca = &courses_assignment_difficulty[c];
        ca->course = course;
        ca->difficulty =
            (n_curriculas * CURRICULAS_CONFLICTS_DIFFICULTY_FACTOR +
            n_teacher_courses * UNAVAILABILITY_CONFLICTS_DIFFICULTY_FACTOR +
            n_unavailabilities * UNAVAILABILITY_CONFLICTS_DIFFICULTY_FACTOR)
            * MAX(1, course->n_lectures * COURSE_LECTURES_DIFFICULTY_FACTOR);
        ca->difficulty_ranking_factor =
                rand_normal(1, config->difficulty_ranking_randomness);

        debug("Course %s has assignment difficulty %d (random ranking factor = %g)",
              ca->course->id, ca->difficulty, ca->difficulty_ranking_factor);
    }

    qsort(courses_assignment_difficulty, C, sizeof(course_assignment),
          course_assignment_compare);

    for (int c = 0; c < C; c++)
        debug("Course %s : %d * %g -> %g",
              courses_assignment_difficulty[c].course->id,
              courses_assignment_difficulty[c].difficulty,
              courses_assignment_difficulty[c].difficulty_ranking_factor,
              courses_assignment_difficulty[c].difficulty * courses_assignment_difficulty[c].difficulty_ranking_factor);

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
                finder->error = strmake("can't find a feasible solution, %d attempts: done %d/%d assignments",
                                        n_attempts, n_assignments, required_assignments);
                goto QUIT;
            }
        }
    }

QUIT:
    free(room_is_used);
    free(teacher_is_busy);
    free(curriculum_is_assigned);
    free(courses_assignment_difficulty);

    bool success = strempty(finder->error);
    if (success) {
        debug("All assignments done; %d attempts for %d assignments",
          n_attempts, n_assignments);
    }

    return success;
}

const char *feasible_solution_finder_find_get_error(feasible_solution_finder *finder) {
    return finder->error;
}