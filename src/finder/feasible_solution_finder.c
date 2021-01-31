#include "feasible_solution_finder.h"
#include <stdlib.h>
#include "log/debug.h"
#include "log/verbose.h"
#include "utils/str_utils.h"
#include "utils/array_utils.h"
#include "utils/mem_utils.h"
#include "utils/random_utils.h"
#include "timeout/timeout.h"

static GHashTable *models_courses_difficulty_cache;

void feasible_solution_finder_config_default(feasible_solution_finder_config *config) {
    config->ranking_randomness = 0.33;
}

void feasible_solution_finder_init(feasible_solution_finder *finder) {
    finder->error = NULL;
}

void feasible_solution_finder_destroy(feasible_solution_finder *finder) {
    free(finder->error);
}

static void feasible_solution_finder_reset(feasible_solution_finder *finder) {
    free(finder->error);
    finder->error = NULL;
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

static int *get_courses_difficulty(const model *m) {
    MODEL(m);

    // Sort the courses by the difficulty, in order to assign the most
    // difficult courses before the easy ones.
    // (i.e. a course is difficult to place if it has many associated constraints).
    // Specifically:
    // 1. H3a: How many curriculum does the course belong to?
    // 2. H3b: How many courses the teacher of the course teaches?
    // 3. H4: How many unavailability constraint the course has?

    // Initialize the cache the first time
    if (!models_courses_difficulty_cache)
        models_courses_difficulty_cache = g_hash_table_new(g_direct_hash, g_direct_equal);

    // Check if the courses difficulty of this model has already been computed
    void *v = g_hash_table_lookup(models_courses_difficulty_cache, GINT_TO_POINTER(GPOINTER_TO_INT(m)));
    if (v)
        return (int *) v;

    // Compute the difficulty score of courses assignments
    static const int CURRICULAS_CONFLICTS_DIFFICULTY_FACTOR = 1;
    static const int TEACHER_CONFLICTS_DIFFICULTY_FACTOR = 1;
    static const int UNAVAILABILITY_CONFLICTS_DIFFICULTY_FACTOR = 1;
    static const int COURSE_LECTURES_DIFFICULTY_FACTOR = 1;

    int *courses_difficulty = mallocx(model->n_courses, sizeof(int));
    FOR_C {
        const course *course = &model->courses[c];
        const int t = course->teacher->index;

        // 1. H3a: How many curriculum does the course belong to?
        int n_curriculas;
        model_curriculas_of_course(model, c, &n_curriculas);

        // 2. H3b: How many courses the teacher of the course teaches?
        int n_teacher_courses;
        model_courses_of_teacher(model, t, &n_teacher_courses);

        // 3. H4: How many unavailability constraint the course has?
        int n_unavailabilities = 0;
        FOR_D {
            FOR_S {
                n_unavailabilities += !model_course_is_available_on_period(model, c, d, s);
            }
        }

        // Compute the difficulty, based on the number of the course's associated constraints
        courses_difficulty[c] = (n_curriculas * CURRICULAS_CONFLICTS_DIFFICULTY_FACTOR +
                                 n_teacher_courses * UNAVAILABILITY_CONFLICTS_DIFFICULTY_FACTOR +
                                 n_unavailabilities * UNAVAILABILITY_CONFLICTS_DIFFICULTY_FACTOR)
                                * MAX(1, course->n_lectures * COURSE_LECTURES_DIFFICULTY_FACTOR);

        debug2("Course %s has assignment difficulty = %d",
               course->id, courses_difficulty[c]);
    }

    // Cache it for next times (useful for multistart)
    g_hash_table_insert(models_courses_difficulty_cache,
                        GINT_TO_POINTER(GPOINTER_TO_INT(m)),
                        courses_difficulty);

    return courses_difficulty;
}

bool feasible_solution_finder_try_find(feasible_solution_finder *finder,
                                       const feasible_solution_finder_config *config,
                                       solution *sol) {
    MODEL(sol->model);
    feasible_solution_finder_reset(finder);

    // Get the courses difficulty
    int *courses_difficulty = get_courses_difficulty(model);

    // Assign a score to each lecture, mostly based on `courses_difficulty`
    // but modified by a random factor `ranking_randomness`.
    lecture_assignment *assignments = mallocx(L, sizeof(lecture_assignment));
    FOR_L {
        lecture_assignment *la = &assignments[l];
        const lecture *lecture = &model->lectures[l];
        la->lecture = lecture;
        double r = rand_normal(1, config->ranking_randomness);
        la->difficulty = courses_difficulty[lecture->course->index] * r;
        debug2("Assignment difficulty of lecture %d (%s) = %.2f (base=%d, rand_factor=%.2f)",
               l, lecture->course->id, la->difficulty,
               courses_difficulty[lecture->course->index], r);
    }

    qsort(assignments, L, sizeof(lecture_assignment), lecture_assignment_compare);

    int n_assignments = 0;
    int n_attempts = 0;

    bool *room_is_used = callocx(R * D * S, sizeof(bool));
    bool *teacher_is_busy = callocx(T * D * S, sizeof(bool));
    bool *curriculum_is_assigned = callocx(Q * D * S, sizeof(bool));

    for (int i = 0; i < L; i++) {
        const lecture_assignment *la = &assignments[i];
        const lecture *lecture = la->lecture;
        const course *course = lecture->course;
        const int l = lecture->index;
        const int c = course->index;
        const int t = course->teacher->index;

        int course_n_curriculas;
        int *course_curriculas = model_curriculas_of_course(model, c, &course_n_curriculas);

        bool assigned = false;
        for (int r = 0; r < R && !assigned; r++) {
            for (int d = 0; d < D && !assigned; d++) {
                for (int s = 0; s < S; s++) {
                    debug2("Assignment attempt: lecture %d (%s) in room %s on (day=%d, slot=%d)?",
                           l, course->id, model->rooms[r].id, d, s);
                    n_attempts++;

                    // Check hard constraints

                    // H2: RoomOccupancy
                    if (room_is_used[INDEX3(r, sol->R, d, D, s, S)]) {
                        debug2("\tfailed (RoomOccupancy: %s)", model->rooms[r].id);
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
                        debug2("\tfailed (CurriculumConflict: %s)",
                               model->curriculas[course_curriculas[curriculum_conflict]].id);
                        continue;
                    }

                    // H3b: Conflicts (Teacher)
                    if (teacher_is_busy[INDEX3(t, T, d, D, s, S)]) {
                        debug2("\tfailed (TeacherConflict %s)", model->teachers[t].id);
                        continue;
                    }

                    // H4: availabilities
                    if (!model_course_is_available_on_period(model, c, d, s)) {
                        debug2("\tfailed (Availabilities (%s, %d, %d))", course->id, d, s);
                        continue;
                    }

                    // Does not break any hard constraint: lecture assigned!
                    debug2("\t-> ASSIGNED c=%d:%s to (r=%d:%s, d=%d, s=%d)",
                           c, model->courses[c].id, r, model->rooms[r].id, d, s);

                    room_is_used[INDEX3(r, R, d, D, s, S)] = true;
                    teacher_is_busy[INDEX3(t, T, d, D, s, S)] = true;
                    for (int cq = 0 ; cq < course_n_curriculas; cq++) {
                        curriculum_is_assigned[INDEX3(
                                course_curriculas[cq], Q, d, D, s, S)] =  true;
                    }

                    solution_assign_lecture(sol, l, r, d, s);
                    assigned = true;
                    n_assignments++;
                    break;
                }
            }
        };

        // The finder failed to provide a feasible solution
        if (!assigned) {
            verbose2("Failed to found a feasible solution: %d/%d assignments in %d attempts",
                    n_assignments, model->n_lectures, n_attempts);
            finder->error = strmake("can't find a feasible solution: %d/%d assignments in %d attempts",
                                    n_assignments, model->n_lectures, n_attempts);
            break;
        }
    }

    free(assignments);
    free(room_is_used);
    free(teacher_is_busy);
    free(curriculum_is_assigned);

    bool success = strempty(finder->error);
    if (success) {
        verbose2("Found feasible solution: %d/%d assignments in %d attempts",
                n_assignments, model->n_lectures, n_attempts);
        solution_assert(sol, true, -1);
    }

    return success;
}

bool feasible_solution_finder_find(feasible_solution_finder *finder,
                                   const feasible_solution_finder_config *config,
                                   solution *sol) {
    int n_trials = 0;
    bool found = false;

    // Try to generate a feasible solution until it is actually feasible
    while (!timeout && !found) {
        n_trials++;
        found = feasible_solution_finder_try_find(finder, config, sol);
        if (!found)
            solution_clear(sol);
    }

    if (found) {
        verbose2("Solution have been found after %d trials", n_trials);
        solution_assert_consistency(sol);
    }
    else
        verbose("Finder timed out");

    return found;
}

const char *feasible_solution_finder_find_get_error(feasible_solution_finder *finder) {
    return finder->error;
}