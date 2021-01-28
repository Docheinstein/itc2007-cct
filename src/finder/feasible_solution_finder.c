#include "feasible_solution_finder.h"
#include "utils/str_utils.h"
#include "log/debug.h"
#include "utils/array_utils.h"
#include "utils/mem_utils.h"
#include "log/verbose.h"
#include "timeout/timeout.h"
#include <stdlib.h>
#include <utils/random_utils.h>

void feasible_solution_finder_config_init(feasible_solution_finder_config *config) {
    config->ranking_randomness = 0.66;
}

void feasible_solution_finder_config_destroy(feasible_solution_finder_config *config) {}

void feasible_solution_finder_init(feasible_solution_finder *finder) {
    finder->error = NULL;
}

void feasible_solution_finder_destroy(feasible_solution_finder *finder) {
    free(finder->error);
}

typedef struct lecture_assignment {
    const lecture *lecture;
    double difficulty;
} lecture_assignment;

static int lecture_assignment_compare(const void *_1, const void *_2) {
    const lecture_assignment * ca1 = (const lecture_assignment *) _1;
    const lecture_assignment * ca2 = (const lecture_assignment *) _2;
    return (int) (ca2->difficulty - ca1->difficulty);
}

static bool feasible_solution_finder_try_find(
        feasible_solution_finder *finder,
        const feasible_solution_finder_config *config,
        solution *sol) {

    MODEL(sol->model);

    debug("feasible_solution_finder_find: %d assignments to do", required_assignments);

    lecture_assignment *assignments = mallocx(C, sizeof(lecture_assignment));

    // Sort the courses by the difficulty, in order to assign the most
    // difficult courses before the easy ones
    // (i.e. a course is difficult to place if it has many associated constraints).
    // Specifically:
    // 1. H3a: How many curriculum does the course belong to?
    // 2. H3b: How many courses the teacher of the course teaches?
    // 3. H4: How many unavailability constraint the course has?

    // Compute the difficulty score of courses assignments
    static const int CURRICULAS_CONFLICTS_DIFFICULTY_FACTOR = 1;
    static const int TEACHER_CONFLICTS_DIFFICULTY_FACTOR = 1;
    static const int UNAVAILABILITY_CONFLICTS_DIFFICULTY_FACTOR = 1;
    static const int COURSE_LECTURES_DIFFICULTY_FACTOR = 1;

    FOR_L {
        const lecture *lecture = &model->lectures[l];
        const int c = lecture->course->index;
        const int t = lecture->course->teacher->index;

        // 1. H3a: How many curriculum does the course belong to?
        int n_curriculas;
        model_curriculas_of_course(model, c, &n_curriculas);
        debug2("Course %s belongs to %d curriculas",  course->id, n_curriculas);


        // 2. H3b: How many courses the teacher of the course teaches?
        int n_teacher_courses;
        model_courses_of_teacher(model, t, &n_teacher_courses);
        debug2("Course %s has teacher %s which teaches %d courses",
               course->id, model->courses[c].teacher_id, n_teacher_courses);

        // 3. H4: How many unavailability constraint the course has?
        int n_unavailabilities = 0;
        FOR_D {
            FOR_S {
                n_unavailabilities += !model_course_is_available_on_period(model, c, d, s);
            }
        }
        debug2("Course %s has %d unavailabilities constraints",
               model->courses[c].id, n_unavailabilities);

        lecture_assignment *la = &assignments[l];
        la->lecture = lecture;
        la->difficulty =
                (n_curriculas * CURRICULAS_CONFLICTS_DIFFICULTY_FACTOR +
                 n_teacher_courses * UNAVAILABILITY_CONFLICTS_DIFFICULTY_FACTOR +
                 n_unavailabilities * UNAVAILABILITY_CONFLICTS_DIFFICULTY_FACTOR)
                * MAX(1, lecture->course->n_lectures * COURSE_LECTURES_DIFFICULTY_FACTOR)
                * rand_normal(1, config->ranking_randomness);

        debug2("Course %s has assignment difficulty %d (random ranking factor = %g)",
               ca->course->id, ca->difficulty, ca->difficulty_ranking_factor);
    }

    qsort(assignments, C, sizeof(lecture_assignment), lecture_assignment_compare);

    int n_assignments = 0;
    int n_attempts = 0;

    bool *room_is_used = callocx(R * D * S, sizeof(bool));
    bool *teacher_is_busy = callocx(T * D * S, sizeof(bool));
    bool *curriculum_is_assigned = callocx(Q * D * S, sizeof(bool));

    FOR_L {
        const lecture_assignment *la = &assignments[l];
        const course *course = la->lecture->course;
        const int c = course->index;
        const int t = course->teacher->index;

        int course_n_curriculas;
        int *course_curriculas = model_curriculas_of_course(model, c, &course_n_curriculas);

        bool assigned = false;
        for (int r = 0; r < R && !assigned; r++) {
            for (int d = 0; d < D && !assigned; d++) {
                for (int s = 0; s < S; s++) {
                    debug2("\t%s in room %s on (day=%d, slot=%d)?", course->id, model->rooms[r].id, d, s);
                    n_attempts++;

                    // Check hard constraints

                    // H2: RoomOccupancy
                    if (room_is_used[INDEX3(r, sol->R, d, D, s, S)]) {
                        debug2("\t\tfailed (RoomOccupancy: %s)", model->rooms[r].id);
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
                        debug2("\t\tfailed (CurriculumConflict: %s)",
                               model->curriculas[course_curriculas[curriculum_conflict]].id);
                        continue;
                    }

                    // H3b: Conflicts (Teacher)
                    if (teacher_is_busy[INDEX3(t, T, d, D, s, S)]) {
                        debug2("\t\tfailed (TeacherConflict %s)", model->teachers[t].id);
                        continue;
                    }

                    // H4: availabilities
                    if (!model_course_is_available_on_period(model, c, d, s)) {
                        debug2("\t\tfailed (Availabilities (%s, %d, %d))", course->id, d, s);
                        continue;
                    }

                    debug2(" -> OK!");
                    room_is_used[INDEX3(r, R, d, D, s, S)] = true;
                    teacher_is_busy[INDEX3(t, T, d, D, s, S)] = true;
                    for (int cq = 0 ; cq < course_n_curriculas; cq++) {
                        curriculum_is_assigned[INDEX3(
                                course_curriculas[cq], Q, d, D, s, S)] =  true;
                    }

                    solution_set_lecture_assignment(sol, l, r, d, s);
                    assigned = true;
                    n_assignments++;
                    break;
                }
            }
        };

        if (!assigned) {
            debug("Can't find a feasible assignment for lecture of course %s (%d attempts, %d/%d assignments done)",
                  course->id, n_attempts, n_assignments, required_assignments);
            finder->error = strmake("can't find a feasible solution: %d attempts, %d/%d assignments",
                                    n_attempts, n_assignments, model->n_lectures);
            break;
        }
    }

    free(assignments);
    free(room_is_used);
    free(teacher_is_busy);
    free(curriculum_is_assigned);

    bool success = strempty(finder->error);
    if (success) {
        debug("Found feasible solution: %d attempts for %d/%d assignments",
              n_attempts, n_assignments, model->n_lectures);
    }

    return success;
}

bool feasible_solution_finder_find(feasible_solution_finder *finder,
                                   const feasible_solution_finder_config *config,
                                   solution *sol) {
    while (!timeout) {
        if (feasible_solution_finder_try_find(finder, config, sol))
            return true;
        solution_clear(sol);
    }
    debug("feasible_solution_finder_find: timed out");
    return false;
}

const char *feasible_solution_finder_find_get_error(feasible_solution_finder *finder) {
    return finder->error;
}