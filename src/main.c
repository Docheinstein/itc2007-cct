/*
INTERNATION TIMETABLING COMPETITION 2007
http://www.cs.qub.ac.uk/itc2007/

===========================================

3     Problem formulation

The Curriculum-based timetabling problem consists of the weekly scheduling of the lectures
    for several university courses within a given number of rooms and time periods, where conflicts
    between courses are set according to the curricula published by the University and not on the
    basis of enrolment data.

This formulation applies to University of Udine (Italy) and to many Italian and indeed
    International Universities, although it is slightly simplified with respect to the real problem
    to maintain a certain level of generality.
    The problem consists of the following entities:

Days, Timeslots, and Periods. We are given a number of teaching days in the week (typ-
    ically 5 or 6). Each day is split in a fixed number of timeslots, which is equal for all days.
    A period is a pair composed of a day and a timeslot. The total number of scheduling
    periods is the product of the days times the day timeslots.

Courses and Teachers. Each course consists of a fixed number of lectures to be scheduled
    in distinct periods, it is attended by given number of students, and is taught by a
    teacher. For each course there is a minimum number of days that the lectures of the
    course should be spread in, moreover there are some periods in which the course cannot
    be scheduled.

Rooms. Each room has a capacity, expressed in terms of number of available seats. All
    rooms are equally suitable for all courses (if large enough).

Curricula. A curriculum is a group of courses such that any pair of courses in the group
    have students in common. Based on curricula, we have the conflicts between courses
    and other soft constraints.

The solution of the problem is an assignment of a period (day and timeslot) and a room
    to all lectures of each course.

3.1     Hard constraints

Lectures: All lectures of a course must be scheduled, and they must be assigned to distinct
       periods. A violation occurs if a lecture is not scheduled.

RoomOccupancy: Two lectures cannot take place in the same room in the same period.
       Two lectures in the same room at the same period represent one violation . Any extra
       lecture in the same period and room counts as one more violation.

Conflicts: Lectures of courses in the same curriculum or taught by the same teacher must be
       all scheduled in different periods. Two conflicting lectures in the same period represent
       one violation. Three conflicting lectures count as 3 violations: one for each pair.

Availabilities: If the teacher of the course is not available to teach that course at a given
       period, then no lectures of the course can be scheduled at that period. Each lecture in
       a period unavailable for that course is one violation.

3.2     Soft constraints

RoomCapacity: For each lecture, the number of students that attend the course must be
       less or equal than the number of seats of all the rooms that host its lectures.Each student
       above the capacity counts as 1 point of penalty.

MinimumWorkingDays: The lectures of each course must be spread into the given mini-
       mum number of days. Each day below the minimum counts as 5 points of penalty.

CurriculumCompactness: Lectures belonging to a curriculum should be adjacent to each
       other (i.e., in consecutive periods). For a given curriculum we account for a violation
       every time there is one lecture not adjacent to any other lecture within the same day.
       Each isolated lecture in a curriculum counts as 2 points of penalty.

RoomStability: All lectures of a course should be given in the same room. Each distinct
        room used for the lectures of a course, but the first, counts as 1 point of penalty.
 */

#include <stdlib.h>
#include <stdbool.h>
#include <solvers/exact_solver.h>
#include <utils/io_utils.h>
#include <utils/random_utils.h>
#include <time.h>
#include <utils/str_utils.h>
#include "args/args_parser.h"
#include "log/verbose.h"
#include "model_parser.h"
#include "solution.h"
#include "utils/array_utils.h"
#include "renderer.h"
#include "feasible_solution_finder.h"
#include "log/debug.h"
#include "heuristics/neighbourhoods/neighbourhood_swap.h"
#include "solvers/local_search_solver.h"
#include "solvers/tabu_search_solver.h"
#include <sys/time.h>   /* for setitimer */
#include <unistd.h>     /* for pause */
#include <signal.h>     /* for signal */
#include <math.h>
#include <solvers/hill_climbing_solver.h>
#include <solvers/iterated_local_search_solver.h>
#include <solvers/simulated_annealing_solver.h>
#include <solvers/hybrid_solver.h>
#include "heuristics/heuristic_solver.h"

#include "utils/assert_utils.h"
#include <omp.h>
#include <heuristics/methods/hill_climbing.h>
#include <config/config_parser.h>
#include <heuristics/methods/local_search.h>
#include <utils/mem_utils.h>
#include <heuristics/methods/tabu_search.h>
#include <heuristics/methods/simulated_annealing.h>
#include <utils/time_utils.h>
#include "config/config.h"

int main (int argc, char **argv) {
    // --- PARSE ARGS ---
    fileclear("/tmp/moves.txt");

    args args;
    args_init(&args);
    if (!parse_args(&args, argc, argv)) {
        exit(EXIT_FAILURE);
    }

    set_verbosity(args.verbosity);

    if (args.seed == ARG_INT_NONE) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
        args.seed = ts.tv_nsec ^ ts.tv_sec;
    }

    if (args.assignments_difficulty_ranking_randomness == ARG_INT_NONE)
        args.assignments_difficulty_ranking_randomness = 0.66;

    char *args_str = args_to_string(&args);
    verbose("====== ARGUMENTS ======\n"
            "%s\n"
            "-----------------------",
            args_str);
    free(args_str);

    verbose("Using RNG seed: %u", args.seed);
    rand_set_seed(args.seed);

    // --- PARSE CONFIG ---

    config cfg;
    config_default(&cfg);

    if (args.config) {
        if (!read_config_file(&cfg, args.config))
            exit(EXIT_FAILURE);
    }
    if (args.options->len) {
        if (!read_config_options(&cfg, (const char **) args.options->data, args.options->len))
            exit(EXIT_FAILURE);
    }

    char *cfg_str = config_to_string(&cfg);
    verbose("====== CONFIG ======\n"
            "%s\n"
            "-----------------------",
            cfg_str);
    free(cfg_str);

    // --- PARSE MODEL ---

    model model;
    model_init(&model);
    if (!read_model(&model, args.input)) {
        exit(EXIT_FAILURE);
    }

    solution sol;
    solution_init(&sol, &model);

    bool solution_loaded = false;

    if (args.solution_input_file) {
        // Load the solution instead of computing it
        solution_loaded = read_solution(&sol, args.solution_input_file);
    }

    heuristic_solver_config solver_conf;
    heuristic_solver_config_init(&solver_conf);

    solver_conf.cycles_limit = cfg.solver.cycles_limit;
    solver_conf.time_limit = cfg.solver.time_limit;
    solver_conf.starting_solution = solution_loaded ? &sol : NULL;
    solver_conf.multistart = cfg.solver.multistart;
    solver_conf.restore_best_after_cycles = cfg.solver.restore_best_after_cycles;

    if (args.time_limit > 0)
        solver_conf.time_limit = args.time_limit;

    hill_climbing_params *hc_params = mallocx(1, sizeof(hill_climbing_params));
    hc_params->max_idle = cfg.hc.idle;

    tabu_search_params *ts_params = mallocx(1, sizeof(tabu_search_params));
    ts_params->max_idle = cfg.ts.idle;
    ts_params->tabu_tenure = cfg.ts.tenure;
    ts_params->frequency_penalty_coeff = cfg.ts.frequency_penalty_coeff;
    ts_params->random_pick = cfg.ts.random_pick;
    ts_params->steepest = cfg.ts.steepest;
    ts_params->clear_on_best = cfg.ts.clear_on_best;

    simulated_annealing_params *sa_params = mallocx(1, sizeof(simulated_annealing_params));
    sa_params->max_idle = cfg.sa.idle;
    sa_params->temperature = cfg.sa.temperature;
    sa_params->min_temperature = cfg.sa.min_temperature;
    sa_params->temperature_length_coeff = cfg.sa.temperature_length_coeff;
    sa_params->cooling_rate = cfg.sa.cooling_rate;

    for (int i = 0; i < cfg.solver.n_methods; i++) {
        resolution_method method = cfg.solver.methods[i];
        const char *method_name = resolution_method_to_string(method);

        if (method == RESOLUTION_METHOD_LOCAL_SEARCH) {
            heuristic_solver_config_add_method(&solver_conf, local_search,
                                               NULL, method_name);
        }
        else if (method == RESOLUTION_METHOD_TABU_SEARCH) {
            heuristic_solver_config_add_method(&solver_conf, tabu_search,
                                                ts_params, method_name);
        }
        else if (method == RESOLUTION_METHOD_HILL_CLIMBING) {
            heuristic_solver_config_add_method(&solver_conf, hill_climbing,
                                               hc_params, method_name);
        }
        else if (method == RESOLUTION_METHOD_SIMULATED_ANNEALING) {
            heuristic_solver_config_add_method(&solver_conf, simulated_annealing,
                                               sa_params, method_name);
        }
    }

    heuristic_solver solver;
    heuristic_solver_init(&solver);

    solution_loaded = heuristic_solver_solve(&solver, &solver_conf, &sol);
    if (!solution_loaded)
        eprint("ERROR: failed to solve model (%s)", heuristic_solver_get_error(&solver));


    heuristic_solver_destroy(&solver);
    heuristic_solver_config_destroy(&solver_conf);

    free(hc_params);
    free(ts_params);
    free(sa_params);
#if 0

    // Standard case, compute the solution
    if (args.method == RESOLUTION_METHOD_EXACT) {
        exact_solver_config conf;
        exact_solver_config_init(&conf);

        exact_solver solver;
        exact_solver_init(&solver);

        conf.grb_write_lp = args.write_lp_file;
        conf.grb_verbose = args.verbose;
        conf.grb_time_limit = args.time_limit;

        verbose("Solving model exactly, it might take a long time...");
        solution_loaded = exact_solver_solve(&solver, &conf, &sol);
        if (!solution_loaded)
            eprint("ERROR: failed to solve model (%s)", exact_solver_get_error(&solver));

        exact_solver_config_destroy(&conf);
        exact_solver_destroy(&solver);
    } else if (args.method == RESOLUTION_METHOD_LOCAL_SEARCH) {
        local_search_solver_config config;
        local_search_solver_config_init(&config);
        config.multistart = args.multistart;
        config.time_limit = args.time_limit;
        config.difficulty_ranking_randomness = args.assignments_difficulty_ranking_randomness;
        config.starting_solution = solution_loaded ? &sol : NULL;

        local_search_solver solver;
        local_search_solver_init(&solver);

        solution_loaded = local_search_solver_solve(&solver, &config, &sol);
        if (!solution_loaded)
            eprint("ERROR: failed to solve model (%s)", local_search_solver_get_error(&solver));

        local_search_solver_config_destroy(&config);
        local_search_solver_destroy(&solver);
    } else if (args.method == RESOLUTION_METHOD_HILL_CLIMBING) {
        #pragma omp parallel default(none) shared(model, args, stderr, sol, best_cost, solution_loaded)
        {
            solution s;
            solution_init(&s, &model);

            hill_climbing_solver_config config;
            hill_climbing_solver_config_init(&config);
//            config.idle_limit = args.multistart;
            config.time_limit = args.time_limit;
            config.difficulty_ranking_randomness = args.assignments_difficulty_ranking_randomness;
//            config.starting_solution = solution_loaded ? &s : NULL;

            hill_climbing_solver solver;
            hill_climbing_solver_init(&solver);

            bool ok = hill_climbing_solver_solve(&solver, &config, &s);
            if (!ok)
                eprint("ERROR: failed to solve model (%s)", hill_climbing_solver_get_error(&solver));

            hill_climbing_solver_config_destroy(&config);
            hill_climbing_solver_destroy(&solver);

            verbose("Thread %d reached cost %d", omp_get_thread_num(), solution_cost(&s));
            #pragma omp critical
            {
                solution_loaded = solution_loaded || ok;
                int cost = solution_cost(&s);
                if (cost < best_cost) {
                    solution_copy(&sol, &s);
                    best_cost = cost;
                }
            }

    } else if (args.method == RESOLUTION_METHOD_ITERATED_LOCAL_SEARCH) {
        iterated_local_search_solver_config config;
        iterated_local_search_solver_config_init(&config);
        config.idle_limit = args.multistart;
        config.time_limit = args.time_limit;
        config.difficulty_ranking_randomness = args.assignments_difficulty_ranking_randomness;
        config.starting_solution = solution_loaded ? &sol : NULL;

        iterated_local_search_solver solver;
        iterated_local_search_solver_init(&solver);

        solution_loaded = iterated_local_search_solver_solve(&solver, &config, &sol);
        if (!solution_loaded)
            eprint("ERROR: failed to solve model (%s)", iterated_local_search_solver_get_error(&solver));

        iterated_local_search_solver_config_destroy(&config);
        iterated_local_search_solver_destroy(&solver);
    } else if (args.method == RESOLUTION_METHOD_SIMULATED_ANNEALING) {
        simulated_annealing_solver_config config;
        simulated_annealing_solver_config_init(&config);
        config.idle_limit = args.multistart;
        config.time_limit = args.time_limit;
        config.difficulty_ranking_randomness = args.assignments_difficulty_ranking_randomness;
        config.starting_solution = solution_loaded ? &sol : NULL;

        simulated_annealing_solver solver;
        simulated_annealing_solver_init(&solver);

        solution_loaded = simulated_annealing_solver_solve(&solver, &config, &sol);
        if (!solution_loaded)
            eprint("ERROR: failed to solve model (%s)", simulated_annealing_solver_get_error(&solver));

        simulated_annealing_solver_config_destroy(&config);
        simulated_annealing_solver_destroy(&solver);
    } else if (args.method == RESOLUTION_METHOD_TABU_SEARCH) {
        tabu_search_solver_config config;
        tabu_search_solver_config_init(&config);
        config.iter_limit = args.multistart;
        config.time_limit = args.time_limit;
        config.difficulty_ranking_randomness = args.assignments_difficulty_ranking_randomness;
        config.starting_solution = solution_loaded ? &sol : NULL;

        tabu_search_solver solver;
        tabu_search_solver_init(&solver);

        solution_loaded = tabu_search_solver_solve(&solver, &config, &sol);
        if (!solution_loaded)
            eprint("ERROR: failed to solve model (%s)", tabu_search_solver_get_error(&solver));

        tabu_search_solver_config_destroy(&config);
        tabu_search_solver_destroy(&solver);
    } else if (args.method == RESOLUTION_METHOD_HYBRYD) {
        hybrid_solver_config config;
        hybrid_solver_config_init(&config);
        config.time_limit = args.time_limit;
        config.difficulty_ranking_randomness = args.assignments_difficulty_ranking_randomness;
        config.starting_solution = solution_loaded ? &sol : NULL;

        hybrid_solver solver;
        hybrid_solver_init(&solver);

        solution_loaded = hybrid_solver_solve(&solver, &config, &sol);
        if (!solution_loaded)
            eprint("ERROR: failed to solve model (%s)", hybrid_solver_get_error(&solver));

        hybrid_solver_config_destroy(&config);
        hybrid_solver_destroy(&solver);
    }
#endif

    if (solution_loaded || args.force_draw) {
        if (args.benchmark_mode) {
            char benchmark_output[64];
            int rc = solution_room_capacity_cost(&sol);
            int mwd = solution_min_working_days_cost(&sol);
            int cc = solution_curriculum_compactness_cost(&sol);
            int rs = solution_room_stability_cost(&sol);
            int cost = rc + mwd + cc + rs;
            bool feasible = solution_satisfy_hard_constraints(&sol);
            snprintf(benchmark_output, 64, "%u %d %d %d %d %d %d\n",
                     rand_get_seed(), feasible, rc, mwd, cc, rs, cost);
            printf("%s", benchmark_output);

            if (args.output)
                fileappend(args.output, benchmark_output);
        }
        else {
            char *sol_str = solution_to_string(&sol);
            char *sol_quality_str = solution_quality_to_string(&sol, true);

            print("====== SOLUTION ======\n"
                  "%s\n"
                  "----------------------\n"
                  "%s",
                  sol_str,
                  sol_quality_str);

            free(sol_str);
            free(sol_quality_str);

            if (args.output)
                write_solution(&sol, args.output);
        }


        if (args.draw_overview_file || args.draw_directory)
            render_solution_full(&sol, args.draw_overview_file, args.draw_directory);
    }

    solution_destroy(&sol);
    model_destroy(&model);
    config_destroy(&cfg);
    args_destroy(&args);

    debug("(seed was %u)", rand_get_seed());
    return 0;
}