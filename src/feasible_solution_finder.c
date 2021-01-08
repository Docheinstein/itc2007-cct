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

    for (int c = 0; c < C; c++) {
        const course *course = &m->courses[c];
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
