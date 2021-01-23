#include <stdio.h>
#include <utils/io_utils.h>
#include "log/verbose.h"
#include "log/debug.h"
#include "exact_solver.h"
#include "gurobi/gurobi_c.h"
#include "utils/mem_utils.h"
#include "utils/array_utils.h"
#include "config.h"
#include "utils/def_utils.h"
#include "utils/str_utils.h"

static const char * status_to_string(int status) {
    switch (status) {
    case GRB_LOADED:
         return "model is loaded, but no solution information is available";
    case GRB_OPTIMAL:
        return "model was solved to optimality (subject to tolerances), and an optimal solution is available";
    case GRB_INFEASIBLE:
        return "model was proven to be infeasible";
    case GRB_INF_OR_UNBD:
        return "model was proven to be either infeasible or unbounded";
    case GRB_UNBOUNDED:
        return "model was proven unbounded";
    case GRB_CUTOFF:
        return "optimal objective for model was proven to be worse than the value specified in the Cutoff parameter. No solution information is available";
    case GRB_ITERATION_LIMIT:
        return "optimization terminated because the total number of simplex iterations performed exceeded the value specified in the IterationLimit parameter, or because the total number of barrier iterations exceeded the value specified in the BarIterLimit parameter";
    case GRB_NODE_LIMIT:
        return "optimization terminated because the total number of branch-and-cut nodes explored exceeded the value specified in the NodeLimit parameter";
    case GRB_TIME_LIMIT:
        return "optimization terminated because the time expended exceeded the value specified in the TimeLimit parameter";
    case GRB_SOLUTION_LIMIT:
        return "optimization terminated because the number of solutions found reached the value specified in the SolutionLimit parameter";
    case GRB_INTERRUPTED:
        return "optimization was terminated by the user";
    case GRB_NUMERIC:
        return "optimization was terminated due to unrecoverable numerical difficulties";
    case GRB_SUBOPTIMAL:
        return "unable to satisfy optimality tolerances; a sub-optimal solution is available";
    case GRB_INPROGRESS:
        return "an asynchronous optimization call was made, but the associated optimization run is not yet complete";
    case GRB_USER_OBJ_LIMIT:
        return "user specified an objective limit (a bound on either the best objective or the best bound), and that limit has been reached";
    default:
        return "unknown error";
    }
}

void exact_solver_config_init(exact_solver_config *config) {
    config->grb_verbose = true;
    config->grb_time_limit = 0;
    config->grb_write_lp = NULL;

    config->h_lectures = true;
    config->h_room_occupancy = true;
    config->h_conflicts = true;
    config->h_availability = true;

    config->s_room_capacity = ROOM_CAPACITY_COST;
    config->s_minimum_working_days = MIN_WORKING_DAYS_COST;
    config->s_curriculum_compactness = CURRICULUM_COMPACTNESS_COST;
    config->s_room_stability = ROOM_STABILITY_COST;
}

void exact_solver_config_destroy(exact_solver_config *config) {

}

void exact_solver_init(exact_solver *solver) {
    solver->objective = 0;
    solver->error = NULL;
}

void exact_solver_destroy(exact_solver *solver) {
    // DO NOT free solver->error since is a const char * from status_to_string()
}

static void exact_solver_reinit(exact_solver *solver) {
    exact_solver_init(solver);
    exact_solver_destroy(solver);
}

bool exact_solver_solve(exact_solver *solver, exact_solver_config *config,
                        solution *solution) {

#define GRB_CHECK(op) do { \
    error = op; \
    if (error) goto QUIT; \
} while(0)

#define GRB_ADD_VAR(term, lb, ub, type, fmt, ...) do { \
    snprintf(name, LENGTH(name), fmt, ##__VA_ARGS__); \
    debug("Adding var '%s'", name); \
    GRB_CHECK(GRBaddvar(model, 0, NULL, NULL, term, lb, ub, type, name)); \
    var_index++; \
} while(0)

#define GRB_ADD_BINARY_VAR(term, fmt, ...) do { \
    snprintf(name, LENGTH(name), fmt, ##__VA_ARGS__); \
    debug("Adding binary var '%s'", name); \
    GRB_CHECK(GRBaddvar(model, 0, NULL, NULL, term, 0.0, 1.0, GRB_BINARY, name)); \
    var_index++; \
} while(0)

#define GRB_ADD_CONSTR(numnz, sense, rhs, fmt, ...) do { \
    snprintf(name, LENGTH(name), fmt, ##__VA_ARGS__); \
    debug("Adding constraint '%s'", name); \
    GRB_CHECK(GRBaddconstr(model, numnz, indexes, values, sense, rhs, name)); \
} while(0)

#define GRB_ADD_INDICATOR(value, begin, nvars, sense, rhs, fmt, ...) do { \
    snprintf(name, LENGTH(name), fmt, ##__VA_ARGS__); \
    debug("Adding indicator constraint '%s'", name); \
    GRB_CHECK(GRBaddgenconstrIndicator(model, name, begin, value, nvars, indexes, values, sense, rhs)); \
} while(0)

#define GRB_ADD_AND(begin, nvars, fmt, ...) do { \
    snprintf(name, LENGTH(name), fmt, ##__VA_ARGS__); \
    debug("Adding indicator constraint '%s'", name); \
    GRB_CHECK(GRBaddgenconstrAnd(model, name, begin, nvars, indexes)); \
} while(0)

#define GRB_ADD_OR(begin, nvars, fmt, ...) do { \
    snprintf(name, LENGTH(name), fmt, ##__VA_ARGS__); \
    debug("Adding indicator constraint '%s'", name); \
    GRB_CHECK(GRBaddgenconstrOr(model, name, begin, nvars, indexes)); \
} while(0)

#define GRB_SET_OBJECTIVE_TERM(index, term) \
    GRB_CHECK(GRBsetdblattrelement(model, "Obj", (index), (term)))

#define GRB_GET_VARS(begin, count, dest) \
    GRB_CHECK(GRBgetdblattrarray(model, GRB_DBL_ATTR_X, begin, count, dest));

    exact_solver_reinit(solver);

    verbose(
        "====== EXACT SOLVER ======\n"
        "H1: Lectures = %s\n"
        "H2: RoomOccupancy = %s\n"
        "H3: Conflicts = %s\n"
        "H4: Availabilities = %s\n"
        "S1: RoomCapacity = %g\n"
        "S2: MinWorkingDays = %g\n"
        "S3: CurriculumCompactnes = %g\n"
        "S4: RoomStability = %g\n",
        BOOL_TO_STR(config->h_lectures),
        BOOL_TO_STR(config->h_room_occupancy),
        BOOL_TO_STR(config->h_conflicts),
        BOOL_TO_STR(config->h_availability),
        config->s_room_capacity,
        config->s_minimum_working_days,
        config->s_curriculum_compactness,
        config->s_room_stability
    );

    const model *itc = solution->model;

    GRBenv *env = NULL;
    GRBmodel *model = NULL;

    bool error;
    const int C = itc->n_courses;
    const int R = itc->n_rooms;
    const int D = itc->n_days;
    const int S = itc->n_slots;
    const int Q = itc->n_curriculas;
    const int T = itc->n_teachers;

    char name[128];
    int var_index = 0;
    int * indexes = NULL;
    double * values = NULL;

    double time_limit = config->grb_time_limit > 1 ? config->grb_time_limit : GRB_INFINITY;
    GRB_CHECK(GRBemptyenv(&env));
    GRB_CHECK(GRBstartenv(env));
    GRB_CHECK(GRBsetintparam(env, GRB_INT_PAR_OUTPUTFLAG, config->grb_verbose));
    GRB_CHECK(GRBsetdblparam(env, GRB_DBL_PAR_TIMELIMIT, time_limit));
    GRB_CHECK(GRBnewmodel(env, &model, "itc2007_cct", 0, NULL, NULL, NULL, NULL, NULL));

    verbose("Creating model...");

    // ===================

    // ------- VARS -------

    // Constants
    const int One_begin = var_index;
    GRB_ADD_VAR(0.0, 1.0, 1.0, GRB_INTEGER, "One");

    // X_crds
    const int X_begin = var_index;
    for (int c = 0; c < C; c++) {
        for (int r = 0; r < R; r++) {
            for (int d = 0; d < D; d++) {
                for (int s = 0; s < S; s++) {
                    GRB_ADD_BINARY_VAR(0.0, "X_%s_%s_%d_%d",
                                       itc->courses[c].id, itc->rooms[r].id, d, s);
                }
            }
        }
    }
    const int X_count = var_index - X_begin;

    debug("|X|=%d", X_count);

    indexes = mallocx(X_count, sizeof(int));
    values = mallocx(X_count, sizeof(double));

    // Y_cd
    const int Y_begin = var_index;
    for (int c = 0; c < C; c++) {
        for (int d = 0; d < D; d++) {
            GRB_ADD_BINARY_VAR(0.0, "Y_%s_%d", itc->courses[c].id, d);
        }
    }

    // I2_c
    const int I2_begin = var_index;
    for (int c = 0; c < C; c++) {
        GRB_ADD_BINARY_VAR(0.0, "I2_%s", itc->courses[c].id);
    }

    // Z_qds
    const int Z_begin = var_index;
    for (int q = 0; q < Q; q++) {
        for (int d = 0; d < D; d++) {
            for (int s = 0; s < S; s++) {
                GRB_ADD_BINARY_VAR(0.0, "Z_%s_%d_%d", itc->curriculas[q].id, d, s);
            }
        }
    }

    // Z_NOT_qds
    const int Z_NOT_begin = var_index;
    for (int q = 0; q < Q; q++) {
        for (int d = 0; d < D; d++) {
            for (int s = 0; s < S; s++) {
                GRB_ADD_BINARY_VAR(0.0, "Z_NOT_%s_%d_%d", itc->curriculas[q].id, d, s);
            }
        }
    }

    // I3_qds
    const int I3_begin = var_index;
    for (int q = 0; q < Q; q++) {
        for (int d = 0; d < D; d++) {
            for (int s = 0; s < S; s++) {
                GRB_ADD_BINARY_VAR(0.0, "I3_%s_%d_%d", itc->curriculas[q].id, d, s);
            }
        }
    }

    // W_cr
    const int W_begin = var_index;
    for (int c = 0; c < C; c++) {
        for (int r = 0; r < R; r++) {
            GRB_ADD_BINARY_VAR(0.0, "W_%s_%s",
                               itc->courses[c].id, itc->rooms[r].id);
        }
    }

    // ------- INDICATOR CONSTRAINTS -------

    // Z_NOT_qds_Indicator
    for (int q = 0; q < Q; q++) {
        for (int d = 0; d < D; d++) {
            for (int s = 0; s < S; s++) {
                indexes[0] = Z_NOT_begin + INDEX3(q, Q, d, D, s, S);
                values[0] = 1.0;
                indexes[1] = Z_begin + INDEX3(q, Q, d, D, s, S);
                values[1] = 1.0;

                GRB_ADD_CONSTR(2, GRB_EQUAL, 1.0,
                               "Z_NOT_%s_%d_%d_Indicator", itc->curriculas[q].id, d, s);
            }
        }
    }

    // Y_cd_Indicator
    for (int c = 0; c < C; c++) {
        for (int d = 0; d < D; d++) {
            int i = 0;

            for (int r = 0; r < R; r++) {
                for (int s = 0; s < S; s++) {
                    indexes[i] = X_begin + INDEX4(c, C, r, R, d, D, s, S);
                    values[i++] = 1.0;
                }
            }

            GRB_ADD_OR(Y_begin + INDEX2(c, C, d, D), i,
                       "Y_%s_%d_Indicator", itc->courses[c].id, d);
        }
    }

    // I2_c_Indicator
    for (int c = 0; c < C; c++) {
        int i = 0;

        for (int d = 0; d < D; d++) {
            indexes[i] = Y_begin + INDEX2(c, C, d, D);
            values[i++] = 1.0;
        }

        GRB_ADD_INDICATOR(0,
                  I2_begin + INDEX1(c, C), i,
                  GRB_GREATER_EQUAL, (double) itc->courses[c].min_working_days + 1,
                  "I2_%s_Indicator0", itc->courses[c].id);
        GRB_ADD_INDICATOR(1,
                          I2_begin + INDEX1(c, C), i,
                          GRB_LESS_EQUAL, (double) itc->courses[c].min_working_days,
                          "I2_%s_Indicator1", itc->courses[c].id);
    }

    // Z_qds_Indicator
    for (int q = 0; q < Q; q++) {
        for (int d = 0; d < D; d++) {
            for (int s = 0; s < S; s++) {

                int i = 0;
                for (int c = 0; c < C; c++) {
                    if (!model_course_belongs_to_curricula(itc, c, q))
                        continue;

                    for (int r = 0; r < R; r++) {
                        indexes[i] = X_begin + INDEX4(c, C, r, R, d, D, s, S);
                        values[i++] = 1.0;
                    }
                }

                GRB_ADD_OR(Z_begin + INDEX3(q, Q, d, D, s, S), i,
                           "Z_%s_%d_%d_Indicator", itc->curriculas[q].id, d, s);
            }
        }
    }

    // I3_qds_Indicator
    for (int q = 0; q < Q; q++) {
        for (int d = 0; d < D; d++) {
            for (int s = 0; s < S; s++) {
                int i = 0;

                if (s >= 1)
                    indexes[i++] = Z_NOT_begin + INDEX3(q, Q, d, D, s - 1, S);

                indexes[i++] = Z_begin + INDEX3(q, Q, d, D, s, S);
                // Check bounds of timeslots

                if (s < S - 1)
                    indexes[i++] = Z_NOT_begin + INDEX3(q, Q, d, D, s + 1, S);

                GRB_ADD_AND(I3_begin + INDEX3(q, Q, d, D, s, S), i,
                            "I3_%s_%d_%d_Indicator", itc->curriculas[q].id, d, s);
            }
        }
    }

    // W_cr_Indicator
    for (int c = 0; c < C; c++) {
        for (int r = 0; r < R; r++) {
            int i = 0;

            for (int d = 0; d < D; d++) {
                for (int s = 0; s < S; s++) {
                    indexes[i] = X_begin + INDEX4(c, C, r, R, d, D, s, S);
                    values[i++] = 1.0;
                }
            }

            GRB_ADD_OR(W_begin + INDEX2(c, C, r, R), i,
                       "W_%s_%s_Indicator", itc->courses[c].id, itc->rooms[r].id);
        }
    }

    // ------- SOFT CONSTRAINTS (objective function) -------

    debug("Setting soft constraints (objective function terms)");

    // S1: RoomCapacity
    // RC: sum c€C r€R, d€D, s€S max(0, (nC_c nR_r) * X_crds)
    if (config->s_room_capacity) {
        for (int c = 0; c < C; c++) {
            for (int r = 0; r < R; r++) {
                for (int d = 0; d < D; d++) {
                    for (int s = 0; s < S; s++) {
                        double capacity_delta = itc->courses[c].n_students - itc->rooms[r].capacity;
                        if (capacity_delta > 0) {
                            GRB_SET_OBJECTIVE_TERM(
                                    X_begin + INDEX4(c, C, r, R, d, D, s, S),
                                    config->s_room_capacity * capacity_delta
                            );
                        }
                    }
                }
            }
        }
    }

    // S2: MinimumWorkingDays
    // MWD: sum c€C (m_c - sum d€D Y_cd) * I2_c
    if (config->s_minimum_working_days) {
        for (int c = 0; c < C; c++) {
            int index1 = I2_begin + INDEX1(c, C);
            for (int d = 0; d < D; d++) {
                double coeff = - config->s_minimum_working_days;
                int index2 = Y_begin + INDEX2(c, C, d, D);
                GRB_CHECK(GRBaddqpterms(model, 1, &index1, &index2, &coeff));
            }
        }

        for (int c = 0; c < C; c++) {
            GRB_SET_OBJECTIVE_TERM(
                    I2_begin + INDEX1(c, C),
                    config->s_minimum_working_days * (double) itc->courses[c].min_working_days
            );
        }
    }

    // S3: CurriculumCompactness
    // CC: sum q€q d€D s€S I3_qds
    if (config->s_curriculum_compactness) {
        for (int q = 0; q < Q; q++) {
            for (int d = 0; d < D; d++) {
                for (int s = 0; s < S; s++) {
                    GRB_SET_OBJECTIVE_TERM(
                            I3_begin + INDEX3(q, Q, d, D, s, S),
                            config->s_curriculum_compactness
                    );
                }
            }
        }
    }

    // S4; RoomStability
    // RS: sum c€C (r€R w_cr - 1)
    if (config->s_room_stability) {
       for (int c = 0; c < C; c++) {
            for (int r = 0; r < R; r++) {
                GRB_SET_OBJECTIVE_TERM(
                        W_begin + INDEX2(c, C, r, R),
                        config->s_room_stability
                );
            }
       }

       GRB_SET_OBJECTIVE_TERM(One_begin, (double) - itc->n_courses); // -c
    }

    // ------- HARD CONSTRAINTS -------

    // H1: Lectures
    // L: for c€C  |  sum r€R, d€D, s€S X_crds = l_c
    if (config->h_lectures) {
        debug("Adding %d L1 (Lectures) constraints", C);

        for (int c = 0; c < C; c++) {
            int i = 0;
            for (int r = 0; r < R; r ++) {
                for (int d = 0; d < D; d++) {
                    for (int s = 0; s < S; s++) {
                        indexes[i] = X_begin + INDEX4(c, C, r, R, d, D, s, S);
                        values[i++] = 1.0;
                    }
                }
            }

            GRB_ADD_CONSTR(i,
                           GRB_EQUAL, (double) itc->courses[c].n_lectures,
                           "L1_%s", itc->courses[c].id);
        }

        // If Availability constraint is required, there is no need
        // to impose this L2 constraint
        if (!config->h_availability) {
            debug("Adding %d L2 (Lectures) constraints", C * D * S);

            for (int c = 0; c < C; c ++) {
                for (int d = 0; d < D; d++) {
                    for (int s = 0; s < S; s++) {
                        int i = 0;

                        for (int r = 0; r < R; r++) {
                            indexes[i] = X_begin + INDEX4(c, C, r, R, d, D, s, S);
                            values[i++] = 1.0;
                        }

                        GRB_ADD_CONSTR(i,
                                       GRB_LESS_EQUAL, 1.0,
                                       "L2_%s_%d_%d", itc->courses[c].id, d, s);
                    }
                }
            }
        }

    } else {
        verbose("WARN: not using hard constraint 'Lectures'");
    }


    // H2: RoomOccupancy
    // RO: for r€R, d€D, s€S  |  sum c€C X_crds <= 1
    if (config->h_room_occupancy) {
        debug("Adding %d RO (RoomOccupancy) constraints", R * D * S);

        for (int r = 0; r < R; r ++) {
            for (int d = 0; d < D; d++) {
                for (int s = 0; s < S; s++) {
                    int i = 0;

                    for (int c = 0; c < C; c++) {
                        indexes[i] = X_begin + INDEX4(c, C, r, R, d, D, s, S);
                        values[i++] = 1.0;
                    }

                    GRB_ADD_CONSTR(i,
                                   GRB_LESS_EQUAL, 1.0,
                                   "RO_%s_%d_%d", itc->rooms[r].id, d, s);
                }
            }
        }
    } else {
        verbose("WARN: not using hard constraint 'RoomOccupancy'");
    }


    // H3: Conflicts
    // C1: for q€Q, d€D, s€S |  sum c€C, r€R X_crds * b_cq <= 1
    if (config->h_conflicts) {
        debug("Adding %d C1 (Conflicts) constraints", Q * D * S);

        for (int q = 0; q < Q; q++) {
            for (int d = 0; d < D; d++) {
                for (int s = 0; s < S; s++) {
                    int i = 0;

                    for (int c = 0; c < C; c++) {
                        for (int r = 0; r < R; r++) {
                            if (model_course_belongs_to_curricula(itc, c, q)) {
                                indexes[i] = X_begin + INDEX4(c, C, r, R, d, D, s, S);
                                values[i++] = 1.0;
                            }
                        }
                    }

                    GRB_ADD_CONSTR(i,
                                   GRB_LESS_EQUAL, 1.0,
                                   "C1_%s_%d_%d", itc->curriculas[q].id, d, s);
                }
            }
        }

        // C2: for t€T, d€D, s€S |  sum c€C, r€R X_crds * e_ct <= 1
        debug("Adding %d C2 (Conflicts) constraints", T * D * S);

        for (int t = 0; t < T; t++) {
            for (int d = 0; d < D; d++) {
                for (int s = 0; s < S; s++) {
                    int i = 0;

                    for (int c = 0; c < C; c++) {
                        for (int r = 0; r < R; r++) {
                            if (model_course_is_taught_by_teacher(itc, c, t)) {
                                indexes[i] = X_begin + INDEX4(c, C, r, R, d, D, s, S);
                                values[i++] = 1.0;
                            }
                        }
                    }

                    GRB_ADD_CONSTR(i,
                                   GRB_LESS_EQUAL, 1.0,
                                   "C2_%s_%d_%d", itc->teachers[t].id, d, s);
                }
            }
        }
    } else {
        verbose("WARN: not using hard constraint 'Conflicts'");
    }


    // H4: Availabilities
    // A: for r€R  |  sum c€C d€D, s€S X_crds <= a_cds
    if (config->h_availability) {
        debug("Adding %d A (Availabilities) constraints", C * D * S);

        for (int c = 0; c < C; c ++) {
            for (int d = 0; d < D; d++) {
                for (int s = 0; s < S; s++) {
                    int i = 0;

                    for (int r = 0; r < R; r++) {
                        indexes[i] = X_begin + INDEX4(c, C, r, R, d, D, s, S);
                        values[i++] = 1.0;
                    }

                    GRB_ADD_CONSTR(i,
                                   GRB_LESS_EQUAL, (double) model_course_is_available_on_period(itc, c, d, s),
                                   "A_%s_%d_%d", itc->courses[c].id, d, s);
                }
            }
        }
    }


    // ===================

    GRB_CHECK(GRBsetintattr(model, GRB_INT_ATTR_MODELSENSE, GRB_MINIMIZE));

    if (config->grb_write_lp) {
        verbose("Writing model to '%s'", config->grb_write_lp);
        GRB_CHECK(GRBwrite(model, config->grb_write_lp));
    }

    verbose("Starting optimization...");
    GRB_CHECK(GRBoptimize(model));

    int optimal_status;
    GRB_CHECK(GRBgetintattr(model, GRB_INT_ATTR_STATUS, &optimal_status));

    if (optimal_status == GRB_OPTIMAL) {
        GRB_CHECK(GRBgetdblattr(model, GRB_DBL_ATTR_OBJVAL, &solver->objective));
        debug("Model solved: objective = %g", solver->objective);

        // Create solution from X
        GRB_GET_VARS(X_begin, X_count, values);
        int i = 0; // TODO: lecture is useless here
        for (int c = 0; c < C; c++) {
            for (int r = 0; r < R; r++) {
                for (int d = 0; d < D; d++) {
                    for (int s = 0; s < S; s++) {
                        double Xval = values[INDEX4(c, C, r, R, d, D, s, S)];
                        if (Xval > 0) {
                            debug("X[%s,%s,%d,%d]=%g",
                                  itc->courses[c].id, itc->rooms[r].id, d, s,
                                  Xval);
                            solution_set(solution, c, r, d, s, true, i++);
                        }
                    }
                }
            }
        }

        for (int d = 0; d < D; d++) {
            debug("DAY %d", d);
            for (int s = 0; s < S; s++) {
                debug("  SLOT %d", s);
                for (int c = 0; c < C; c++) {
                    for (int r = 0; r < R; r++) {
                        if (values[INDEX4(c, C, r, R, d, D, s, S)]) {
                            debug("    course %s in room %s",
                                  itc->courses[c].id, itc->rooms[r].id);
                        }
                    }
                }
            }
        }
    } else {
        debug("Model NOT solved: %s", status_to_string(optimal_status));
        solver->error = status_to_string(optimal_status);
    }

QUIT:
    if (error) {
        eprint("ERROR: %s", GRBgeterrormsg(env));
        solver->error = GRBgeterrormsg(env);
    }

    // Free data
    free(indexes);
    free(values);

    // Free gurobi model/env
    GRBfreemodel(model);
    GRBfreeenv(env);

#undef GRB_CHECK
#undef GRB_ADD_VAR
#undef GRB_ADD_BINARY_VAR
#undef GRB_ADD_CONSTR
#undef GRB_ADD_INDICATOR
#undef GRB_ADD_AND
#undef GRB_ADD_OR
#undef GRB_SET_OBJECTIVE_TERM
#undef GRB_GET_VARS

    return strempty(solver->error);
}

const char *exact_solver_get_error(exact_solver *solver) {
    return solver->error;
}
