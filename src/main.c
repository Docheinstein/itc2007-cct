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
#include <exact_solver.h>
#include <utils/io_utils.h>
#include <utils/random_utils.h>
#include <time.h>
#include <utils/str_utils.h>
#include "args.h"
#include "log/verbose.h"
#include "parser.h"
#include "solution.h"
#include "utils/array_utils.h"
#include "renderer.h"
#include "feasible_solution_finder.h"
#include "log/debug.h"
#include "neighbourhoods/neighbourhood_swap.h"
#include "local_search_solver.h"
#include "tabu_search_solver.h"
#include <sys/time.h>   /* for setitimer */
#include <unistd.h>     /* for pause */
#include <signal.h>     /* for signal */
#include <math.h>

int main (int argc, char **argv) {
//    for (int i = 0; i < 10000; i++) {
//        double r = rand_triangular(0, 50, 80);
//        print("%g,", r);
//    }
//    exit(0);

    args args;
    args_init(&args);
    args_parse(&args, argc, argv);

    set_verbose(args.verbose);

    if (args.seed == ARG_INT_NONE) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
        args.seed = ts.tv_nsec ^ ts.tv_sec;
    }

    if (args.assignments_difficulty_ranking_randomness == ARG_INT_NONE)
        args.assignments_difficulty_ranking_randomness = 0.66;



    char buffer[1024];
    args_to_string(&args, buffer, LENGTH(buffer));
    verbose("====== ARGUMENTS ======\n"
            "%s\n"
            "-----------------------",
            buffer);

    verbose("Using RNG seed: %u", args.seed);
    rand_set_seed(args.seed);

    model model;
    model_init(&model);

    parser parser;
    parser_init(&parser);
    if (!parser_parse(&parser, args.input, &model)) {
        eprint("ERROR: failed to parse model file '%s' (%s)", args.input, parser_get_error(&parser));
        exit(EXIT_FAILURE);
    }

    parser_destroy(&parser);

    solution sol;
    solution_init(&sol, &model);

    bool solution_loaded = false;

    if (args.solution_input_file) {
        // Load the solution instead of computing it
        solution_loaded = read_solution(&sol, args.solution_input_file);
    }

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
    }


    if (solution_loaded || args.force_draw) {
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

        if (args.draw_overview_file || args.draw_directory)
            render_solution(&sol, args.draw_overview_file, args.draw_directory);
    }

    solution_destroy(&sol);
    model_destroy(&model);
    args_destroy(&args);

    debug("(seed was %d)", rand_get_seed());
    return 0;
}