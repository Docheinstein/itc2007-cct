#include <stdio.h>
#include "log/verbose.h"
#include "utils.h"
#include "log/debug.h"
#include "solver.h"
#include "gurobi/gurobi_c.h"

#define GRB_CHECK(op) do { \
    error = op; \
    if (error) goto CLEAN; \
} while(0)

#define GRB_CHECK0(op) do { \
    error = op; \
    if (error) goto CLEAN_GUROBI; \
} while(0)


#define col2(i2, n2, i1, n1) \
    ((i2) * (n1) + (i1))

#define col3(i3, n3, i2, n2, i1, n1) \
    ((i3) * (n2) * (n1) + (i2) * (n1) + (i1))

#define col4(i4, n4, i3, n3, i2, n2, i1, n1) \
    ((i4) * (n3) * (n2) * (n1) + (i3) * (n2) * (n1) + (i2) * (n1) + (i1))

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

    const int C = itc->n_courses;
    const int R = itc->n_rooms;
    const int D = itc->n_days;
    const int S = itc->n_periods_per_day;

    GRB_CHECK0(GRBemptyenv(&env));
    GRB_CHECK0(GRBstartenv(env));
    GRB_CHECK0(GRBnewmodel(env, &model, "itc2007", 0, NULL, NULL, NULL, NULL, NULL));
//    GRB_CHECK(GRBreadmodel(env, "/home/stefano/Temp/itc/toy.lp", &model));

    // ===================
    // --- VARS ---
    static const int VAR_NAME_LENGTH = 16;
    // X_crds
    const int x_count = C * R * D *S;
    double *X = malloc(sizeof(double) * x_count);
    char * x_vtypes = malloc(sizeof(char) * x_count);
    char ** x_vnames = malloc(sizeof(char *) * x_count);

    verbose("|X|=%d", x_count);

    init_vars(x_vtypes, x_count, GRB_BINARY);
    for (int c = 0; c < C; c++)
        for (int r = 0; r < R; r ++)
            for (int d = 0; d < D; d++)
                for (int s = 0; s < S; s++) {
                    int i = col4(c, C, r, R, d, D, s, S);
                    x_vnames[i] = malloc(sizeof(char) * VAR_NAME_LENGTH);
                    snprintf(x_vnames[i], VAR_NAME_LENGTH, "X_%d%d%d%d", c, r, d, s);
                }

    GRBaddvars(model, x_count, 0, NULL, NULL, NULL, NULL, NULL, NULL, x_vtypes, x_vnames);

    // --- CONSTRAINTS ---
    char constr_name[128];
    int index;

    // H1: sum r€R, d€D, s€S X_crds = l_c     for c€C
    int * indexes = malloc(sizeof(int) * R * D * S);
    double * values = malloc(sizeof(double) * R * D * S);

    for (int c = 0; c < itc->n_courses; c++) {
        index = 0;
        for (int r = 0; r < R; r ++)
            for (int d = 0; d < D; d++)
                for (int s = 0; s < S; s++) {
                    int i = col4(c, C, r, R, d, D, s, S);
                    indexes[index] = i;
                    values[index++] = 1;
                }

        snprintf(constr_name, LENGTH(constr_name), "h1_%s", itc->courses[c].id);
        GRB_CHECK(GRBaddconstr(model, R * D * S, indexes, values,
                               GRB_EQUAL,
                               (double) itc->courses[c].n_lectures,
                               constr_name));
    }

    // ===================

    // --- OBJECTIVE ---
    GRB_CHECK(GRBsetintattr(model, GRB_INT_ATTR_MODELSENSE, GRB_MINIMIZE));

    GRB_CHECK(GRBwrite(model, "/home/stefano/Temp/itc_c.lp"));

    GRB_CHECK(GRBoptimize(model));
    GRB_CHECK(GRBgetintattr(model, GRB_INT_ATTR_STATUS, &optimal_status));

    if (optimal_status == GRB_OPTIMAL) {
        // ...
        GRB_CHECK(GRBgetdblattr(model, GRB_DBL_ATTR_OBJVAL, &obj_value););
        printf("Model solved - objective value = %g\n", obj_value);

        GRB_CHECK(GRBgetdblattrarray(model, GRB_DBL_ATTR_X, 0, x_count, X));

        for (int c = 0; c < C; c++)
            for (int r = 0; r < R; r ++)
                for (int d = 0; d < D; d++)
                    for (int s = 0; s < S; s++) {
                        double Xval = X[col4(c, C, r, R, d, D, s, S)];
                        if (Xval > 0) {
                            debug("X[%s,%s,%d,%d]=%g",
                                  itc->courses[c].id, itc->rooms[r].id, d, s,
                                  Xval);
                            solution_add_assignment(sol, assignment_new(&itc->courses[c], &itc->rooms[r], d, s));
                        }
                    }
    } else {
        printf("Model NOT solved - reason: %s\n", status_to_string(optimal_status));
    }

CLEAN:
    if (error) {
        eprint("ERROR: %s", GRBgeterrormsg(env));
    }

    // Free data
    free(X);
    free(x_vtypes);
    for (int i = 0; i < x_count; i++)
        free(x_vnames[i]);
    free(x_vnames);

    free(indexes);
    free(values);

CLEAN_GUROBI:
    // Free gurobi model/env
    GRBfreemodel(model);
    GRBfreeenv(env);

    return !error;
}
