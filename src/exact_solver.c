#include <stdio.h>
#include <io_utils.h>
#include "verbose.h"
#include "debug.h"
#include "exact_solver.h"
#include "gurobi/gurobi_c.h"
#include "mem_utils.h"
#include "str_utils.h"
#include "array_utils.h"

#define GRB_CHECK(op) do { \
    error = op; \
    if (error) goto CLEAN; \
} while(0)

#define GRB_CHECK0(op) do { \
    error = op; \
    if (error) goto CLEAN_GUROBI; \
} while(0)

static void init_vars(char *vtypes, int count, char vtype) {
    for (int i = 0; i < (count); i++)
        vtypes[i] = vtype;
}

static const char * status_to_string(int status) {
    static const char * STATUSES[] = {
        "",
        "Model is loaded, but no solution information is available",
        "Model was solved to optimality (subject to tolerances), and an optimal solution is available",
        "Model was proven to be infeasible.",
        "Model was proven to be either infeasible or unbounded. To obtain a more definitive conclusion, set the DualReductions parameter to 0 and reoptimize.",
    };

    if (status < LENGTH(STATUSES))
        return STATUSES[status];

    return "unknown error";
}

static void set_indexes(int *indexes, size_t size) {
    for (int i = 0; i < size; i++)
        indexes[i] = i;
}


static void set_values(double *values, size_t size) {
    for (int i = 0; i < size; i++)
        values[i] = 1;
}


bool solver_solve(model *itc, solution *sol) {
    GRBenv *env = NULL;
    GRBmodel *model = NULL;

    int optimal_status;
    double obj_value;
    bool error;
//
    const int C = itc->n_courses;
    const int R = itc->n_rooms;
    const int D = itc->n_days;
    const int S = itc->n_slots;
    const int Q = itc->n_curriculas;
    const int T = itc->n_teachers;

    GRB_CHECK0(GRBemptyenv(&env));
    GRB_CHECK0(GRBstartenv(env));
    GRB_CHECK0(GRBnewmodel(env, &model, "itc2007", 0, NULL, NULL, NULL, NULL, NULL));

    // ===================
    // --- VARS ---
    static const int VAR_NAME_LENGTH = 16;
    // X_crds
    const int x_count = C * R * D *S;
    double *X = mallocx(sizeof(double) * x_count);
    char * x_vtypes = mallocx(sizeof(char) * x_count);
    char ** x_vnames = mallocx(sizeof(char *) * x_count);


    verbose("|X|=%d", x_count);

    init_vars(x_vtypes, x_count, GRB_BINARY);
    for (int c = 0; c < C; c++)
        for (int r = 0; r < R; r ++)
            for (int d = 0; d < D; d++)
                for (int s = 0; s < S; s++)
                    x_vnames[INDEX4(c, C, r, R, d, D, s, S)] =
                            strmake("X_%s_%s_%d_%d", itc->courses[c].id, itc->rooms[r].id, d, s);

    GRBaddvars(model, x_count, 0, NULL, NULL, NULL, NULL, NULL, NULL, x_vtypes, x_vnames);

    // --- CONSTRAINTS ---
    char constr_name[128];

    int * indexes = mallocx(sizeof(int) * C * R * D * S);
    double * values = mallocx(sizeof(double) * C * R * D * S);

    // H1: Lectures
    // L: for c€C  |  sum r€R, d€D, s€S X_crds = l_c
    debug("Adding %d L (Lectures) constraints", C);

    for (int c = 0; c < C; c++) {
        int var_count = 0;
        for (int r = 0; r < R; r ++) {
            for (int d = 0; d < D; d++) {
                for (int s = 0; s < S; s++) {
                    indexes[var_count] = INDEX4(c, C, r, R, d, D, s, S);
                    values[var_count++] = 1.0;
                }
            }
        }


        snprintf(constr_name, LENGTH(constr_name), "L_%s", itc->courses[c].id);
        debug("Adding constraint '%s'", constr_name);
        GRB_CHECK(GRBaddconstr(model, var_count, indexes, values,
                               GRB_EQUAL,
                               (double) itc->courses[c].n_lectures,
                               constr_name));
    }

    // H2: RoomOccupancy
    // RO: for r€R, d€D, s€S  |  sum c€C X_crds <= 1
    debug("Adding %d RO (RoomOccupancy) constraints", R * D * S);

    for (int r = 0; r < R; r ++) {
        for (int d = 0; d < D; d++) {
            for (int s = 0; s < S; s++) {
                int var_count = 0;

                for (int c = 0; c < C; c++) {
                    indexes[var_count] = INDEX4(c, C, r, R, d, D, s, S);
                    values[var_count++] = 1.0;
                }

                snprintf(constr_name, LENGTH(constr_name), "RO_%s_%d_%d",
                         itc->rooms[r].id, d, s);
                debug("Adding constraint '%s'", constr_name);
                GRB_CHECK(GRBaddconstr(model, var_count, indexes, values,
                                       GRB_LESS_EQUAL,
                                       1.0,
                                       constr_name));
            }
        }
    }

    // H3: Conflicts
    // C1: for q€Q, d€D, s€S |  sum c€C, r€R X_crds * b_cq <= 1
    debug("Adding %d C1 (Conflicts) constraints", Q * D * S);

    for (int q = 0; q < Q; q++) {
        for (int d = 0; d < D; d++) {
            for (int s = 0; s < S; s++) {
                int var_count = 0;

                for (int c = 0; c < C; c++) {
                    for (int r = 0; r < R; r++) {
                        if (model_course_belongs_to_curricula(itc, c, q)) {
                            indexes[var_count] = INDEX4(c, C, r, R, d, D, s, S);
                            values[var_count++] = 1.0;
                        }
                    }
                }

                snprintf(constr_name, LENGTH(constr_name), "C1_%s_%d_%d",
                         itc->curriculas[q].id, d, s);
                debug("Adding constraint '%s'", constr_name);
                GRB_CHECK(GRBaddconstr(model, var_count, indexes, values,
                                       GRB_LESS_EQUAL,
                                       1.0,
                                       constr_name));
            }
        }
    }

    // C2: for t€T, d€D, s€S |  sum c€C, r€R X_crds * e_ct <= 1
    debug("Adding %d C2 (Conflicts) constraints", T * D * S);

    for (int t = 0; t < T; t++) {
        for (int d = 0; d < D; d++) {
            for (int s = 0; s < S; s++) {
                int var_count = 0;

                for (int c = 0; c < C; c++) {
                    for (int r = 0; r < R; r++) {
                        if (model_course_taught_by_teacher(itc, c, t)) {
                            indexes[var_count] = INDEX4(c, C, r, R, d, D, s, S);
                            values[var_count++] = 1.0;
                        }
                    }
                }

                snprintf(constr_name, LENGTH(constr_name), "C2_%s_%d_%d",
                         itc->teachers[t], d, s);
                debug("Adding constraint '%s'", constr_name);
                GRB_CHECK(GRBaddconstr(model, var_count, indexes, values,
                                       GRB_LESS_EQUAL,
                                       1.0,
                                       constr_name));
            }
        }
    }


    // H4: Availabilities
    // RO: for r€R  |  sum c€C d€D, s€S X_crds <= a_cds
    debug("Adding %d A (Availabilities) constraints", C * D * S);

    for (int c = 0; c < C; c ++) {
        for (int d = 0; d < D; d++) {
            for (int s = 0; s < S; s++) {
                int var_count = 0;

                for (int r = 0; r < R; r++) {
                    indexes[var_count] = INDEX4(c, C, r, R, d, D, s, S);
                    values[var_count++] = 1.0;
                }

                snprintf(constr_name, LENGTH(constr_name), "A_%s_%d_%d",
                         itc->courses[c].id, d, s);
                debug("Adding constraint '%s'", constr_name);
                GRB_CHECK(GRBaddconstr(model, var_count, indexes, values,
                                       GRB_LESS_EQUAL,
                                       model_course_is_available_on_period(itc, c, d, s),
                                       constr_name));
            }
        }
    }


    // ===================
    debug("Setting objective function");

    // --- OBJECTIVE ---
    GRB_CHECK(GRBsetintattr(model, GRB_INT_ATTR_MODELSENSE, GRB_MINIMIZE));

    debug("Writing .lp");
    GRB_CHECK(GRBwrite(model, "/home/stefano/Temp/itc_c/itc_c.lp"));

    debug("Starting optimization...");
    GRB_CHECK(GRBoptimize(model));
    GRB_CHECK(GRBgetintattr(model, GRB_INT_ATTR_STATUS, &optimal_status));

    if (optimal_status == GRB_OPTIMAL) {
        // ...
        GRB_CHECK(GRBgetdblattr(model, GRB_DBL_ATTR_OBJVAL, &obj_value););
        print("Model solved - objective value = %g", obj_value);

        GRB_CHECK(GRBgetdblattrarray(model, GRB_DBL_ATTR_X, 0, x_count, X));

        for (int c = 0; c < C; c++)
            for (int r = 0; r < R; r ++)
                for (int d = 0; d < D; d++)
                    for (int s = 0; s < S; s++) {
                        double Xval = X[INDEX4(c, C, r, R, d, D, s, S)];
                        if (Xval > 0) {
                            debug("X[%s,%s,%d,%d]=%g",
                                  itc->courses[c].id, itc->rooms[r].id, d, s,
                                  Xval);
                            solution_add_assignment(sol, assignment_new(&itc->courses[c], &itc->rooms[r], d, s));
                        }
                    }
    } else {
        print("Model NOT solved - reason: %s", status_to_string(optimal_status));
    }

CLEAN:
    if (error) {
        eprint("ERROR: %s", GRBgeterrormsg(env));
    }

    // Free data
    free(X);
    free(x_vtypes);
    freearray(x_vnames, x_count);
    free(indexes);
    free(values);

CLEAN_GUROBI:
    // Free gurobi model/env
    GRBfreemodel(model);
    GRBfreeenv(env);

    return !error;
}
