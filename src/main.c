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
#include <utils/io_utils.h>
#include <utils/rand_utils.h>
#include <time.h>
#include "args/args_parser.h"
#include "log/verbose.h"
#include "model/model_parser.h"
#include "solution/solution.h"
#include "renderer/renderer.h"
#include "log/debug.h"
#include <sys/time.h>
#include "heuristics/heuristic_solver.h"

#include <heuristics/methods/hill_climbing.h>
#include <config/config_parser.h>
#include <heuristics/methods/tabu_search.h>
#include <heuristics/methods/simulated_annealing.h>
#include <config/config.h>
#include <solution/solution_parser.h>
#include <heuristics/methods/local_search.h>
#include <utils/time_utils.h>
#include <utils/str_utils.h>

static void handle_new_best_solution(const solution *sol,
                                     const heuristic_solver_stats *stats,
                                     void *arg) {
    debug("Handling new best solution...");

    args *arguments = (args *) arg;

    unsigned long long fingerprint = solution_fingerprint(sol);

    bool do_print_quality = arguments->print_violations > 0;
    bool do_print_quality_more = arguments->print_violations > 1;
    bool do_print_sol = !arguments->quiet;
    bool do_write_sol = arguments->output_file;
    bool do_print_stats = arguments->benchmark_mode;
    bool do_write_stats = arguments->benchmark_file;
    bool do_draw_all = arguments->draw_all_directory;
    bool do_draw_overview = !arguments->draw_all_directory && arguments->draw_overview_file;

    debug("do_print_quality = %s", booltostr(do_print_quality));
    debug("do_print_quality_more = %s", booltostr(do_print_quality_more));
    debug("do_print_sol = %s", booltostr(do_print_sol));
    debug("do_write_sol = %s", booltostr(do_write_sol));
    debug("do_print_stats = %s", booltostr(do_print_stats));
    debug("do_write_stats = %s", booltostr(do_write_stats));


    // Print the solution
    if (do_print_sol) {
        char *sol_str = solution_to_string(sol);
        verbose("====== SOLUTION ======");
        puts(sol_str);
        verbose("----------------------");
        free(sol_str);
    }

    // Print the solution costs
    if (do_print_quality) {
        char *sol_quality_str = solution_quality_to_string(sol, do_print_quality_more);
        print("%s%s", do_print_sol ? "\n" : "", sol_quality_str);
        free(sol_quality_str);
    }

    // Write the solution
    if (do_write_sol) {
        write_solution(sol, arguments->output_file);
    }

    // Print stats
    // <seed> <feasible> <cycles> <moves> <rc> <mwd> <cc> <rs> <cost>
    if (do_print_stats || do_write_stats) {
        char stats_output[128];
        int rc = solution_room_capacity_cost(sol);
        int mwd = solution_min_working_days_cost(sol);
        int cc = solution_curriculum_compactness_cost(sol);
        int rs = solution_room_stability_cost(sol);
        int cost = rc + mwd + cc + rs;
        bool feasible = solution_satisfy_hard_constraints(sol);
        long ending_time = stats->ending_time != LONG_MAX ? stats->ending_time : ms();

        snprintf(stats_output, 128,
             "%u %llu %.1f %ld %ld "
             "%d %d %d %d %d %d\n",
             rand_get_seed(), fingerprint,
             (double) (ending_time - stats->starting_time) / 1000,
             stats->cycle_count, stats->move_count,
             feasible, rc, mwd, cc, rs, cost);

        if (do_write_stats)
            fileappend(arguments->benchmark_file, stats_output);

        if (do_print_stats)
            printf("%s", stats_output);
    }


    verbose("Solution fingerprint: %llu", fingerprint);

    // Render the solution (eventually)
    if (do_draw_all)
        render_solution_full(sol, arguments->draw_all_directory);
    if (do_draw_overview)
        render_solution_overview(sol, arguments->draw_overview_file);
}

static void set_default_resolution_strategy(config *cfg) {
    // Default resolution strategy: SA (+ LS only near best)
    heuristic_method sa = HEURISTIC_METHOD_SIMULATED_ANNEALING;
    heuristic_method ls = HEURISTIC_METHOD_LOCAL_SEARCH;

    g_array_append_val(cfg->solver.methods, sa);
    g_array_append_val(cfg->solver.methods, ls);

    cfg->ls.max_distance_from_best_ratio = 1.02;
}

int main (int argc, char **argv) {
    // Parse command line arguments
    args args;
    args_init(&args);
    if (!parse_args(&args, argc, argv))
        exit(EXIT_FAILURE);

    set_verbosity(args.verbosity);

    unsigned int seed = args.seed;
    if (!seed) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        seed = ts.tv_nsec ^ ts.tv_sec;
    }

    char *args_str = args_to_string(&args);
    verbose("====== ARGUMENTS ======\n"
            "%s\n"
            "-----------------------",
            args_str);
    free(args_str);

    verbose("Seed: %d", seed);
    rand_set_seed(seed);

    // Parse config specified with -c CONFIG (eventually)
    config cfg;
    config_init(&cfg);

    if (args.config_file)
        if (!parse_config_file(&cfg, args.config_file))
            exit(EXIT_FAILURE);

    // Parse option given with -o KEY=VALUE (eventually)
    if (args.options->len)
        if (!parse_config_options(&cfg, (const char **) args.options->data, args.options->len))
            exit(EXIT_FAILURE);

    if (!cfg.solver.methods->len)
        // Set default solver strategy if -o solver.methods=... is not given
        set_default_resolution_strategy(&cfg);

    char *cfg_str = config_to_string(&cfg);
    verbose("====== CONFIG ======\n"
            "%s\n"
            "-----------------------",
            cfg_str);
    free(cfg_str);

    // Parse input dataset
    model model;
    model_init(&model);

    if (!parse_model(&model, args.input_file))
        exit(EXIT_FAILURE);

    solution sol;
    solution_init(&sol, &model);

    bool solution_loaded = false;

    if (args.solution_input_file) {
        // Load the solution from file (eventually)
        solution_loaded = parse_solution(&sol, args.solution_input_file);
        if (!solution_loaded)
            exit(EXIT_FAILURE);
    }

    // Configure solver
    heuristic_solver_config solver_conf;
    heuristic_solver_config_init(&solver_conf);

    solver_conf.starting_solution = solution_loaded ? &sol : NULL;
    solver_conf.multistart = cfg.solver.multistart;
    solver_conf.restore_best_after_cycles = cfg.solver.restore_best_after_cycles;
    solver_conf.dont_solve = args.dont_solve;
    solver_conf.max_cycles = cfg.solver.max_cycles;
    solver_conf.max_time = cfg.solver.max_time;
    if (args.max_time >= 0 || args.race_mode) {
        // -t TIME override the default time (60 seconds).
        // If -r is given and -t is not, the time limit is unlimited
        // (which is the default value of args.max_time).
        solver_conf.max_time = args.max_time;
    }

    if (args.race_mode) {
        // Race mode, dump a new best solution every time it is found
        solver_conf.new_best_callback.callback = handle_new_best_solution;
        solver_conf.new_best_callback.arg = &args;
    }

    heuristic_method *methods = (heuristic_method *) cfg.solver.methods->data;
    for (int i = 0; i < cfg.solver.methods->len; i++) {
        heuristic_method method = methods[i];
        const char *method_name = heuristic_method_to_string(method);
        const char *method_short_name = heuristic_method_to_string_short(method);

        if (method == HEURISTIC_METHOD_LOCAL_SEARCH) {
            heuristic_solver_config_add_method(&solver_conf, local_search,
                                               &cfg.ls, method_name, method_short_name);
        } else if (method == HEURISTIC_METHOD_TABU_SEARCH) {
            heuristic_solver_config_add_method(&solver_conf, tabu_search,
                                               &cfg.ts, method_name, method_short_name);
        } else if (method == HEURISTIC_METHOD_HILL_CLIMBING) {
            heuristic_solver_config_add_method(&solver_conf, hill_climbing,
                                               &cfg.hc, method_name, method_short_name);
        } else if (method == HEURISTIC_METHOD_SIMULATED_ANNEALING) {
            heuristic_solver_config_add_method(&solver_conf, simulated_annealing,
                                               &cfg.sa, method_name, method_short_name);
        } else if (method == HEURISTIC_METHOD_DEEP_LOCAL_SEARCH) {
            heuristic_solver_config_add_method(&solver_conf, deep_local_search,
                                               &cfg.dls, method_name, method_short_name);
        }
    }

    heuristic_solver solver;
    heuristic_solver_init(&solver);

    heuristic_solver_stats stats;
    heuristic_solver_stats_init(&stats);
    solution_loaded = heuristic_solver_solve(&solver, &solver_conf, &cfg.finder,
                                             &sol, &stats);

    if (solution_loaded) {
        // In race mode the solution has already been handled;
        // handle the solution now only if not in race mode
        if (!args.race_mode)
            handle_new_best_solution(&sol, &stats, &args);
    } else {
        eprint("ERROR: failed to solve model (%s)", heuristic_solver_get_error(&solver));
    }

    heuristic_solver_destroy(&solver);
    heuristic_solver_stats_destroy(&stats);
    heuristic_solver_config_destroy(&solver_conf);

    solution_destroy(&sol);
    model_destroy(&model);
    config_destroy(&cfg);
    args_destroy(&args);

    verbose("Seed was: %u", rand_get_seed());
    return 0;
}