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
#include <io_utils.h>
#include "args.h"
#include "verbose.h"
#include "parser.h"
#include "solution.h"
#include "array_utils.h"

int main (int argc, char **argv) {
    args args;
    args_init(&args);
    args_parse(&args, argc, argv);

    set_verbose(args.verbose);

    char buffer[256];
    args_to_string(&args, buffer, LENGTH(buffer));
    verbose("====== ARGUMENTS ======\n"
            "%s",
            buffer);

    model m;
    model_init(&m);

    parser p;
    parser_init(&p);
    if (!parser_parse(&p, args.input, &m)) {
        eprint("ERROR: failed to parse input file '%s' (%s)", args.input, parser_get_error(&p));
        exit(EXIT_FAILURE);
    }

    parser_destroy(&p);

    model_finalize(&m);

    solution sol;
    solution_init(&sol);

    bool solved = false;
    double cost = 0;

    if (args.method == ITC2007_METHOD_EXACT) {
        exact_solver solver;
        exact_solver_init(&solver);

        exact_solver_config conf;
        exact_solver_config_init(&conf);
        conf.grb_write_lp = args.write_lp;
        conf.grb_verbose = args.verbose;
        conf.grb_time_limit = args.time_limit;

        solved = exact_solver_solve(&solver, &conf, &m, &sol);
        if (solved) {
            verbose("Model solved: cost = %g", solver.objective);
            cost = solver.objective;
        }
        else
            eprint("ERROR: failed to solve model (%s)", exact_solver_get_error(&solver));

        exact_solver_config_destroy(&conf);
        exact_solver_destroy(&solver);
    } else if (args.method == ITC2007_METHOD_TABU) {
        eprint("ERROR: not implemented yet");
        exit(EXIT_FAILURE);
    }

    if (solved) {
        char * sol_str = solution_to_string(&sol);
        // TODO
        // cost = solution_cost(&sol);
        print(
            "====== SOLUTION ======\n"
            "Cost: %g\n"
            "----------------------\n"
            "%s",
            cost,
            sol_str
        );

        if (args.output) {
            if (!filewrite(args.output, sol_str)) {
                eprint("ERROR: failed to write output solution to '%s' (%s)",
                       args.output, strerror(errno));
            }
        }

        free(sol_str);
    }

    solution_destroy(&sol);
    model_destroy(&m);
    args_destroy(&args);

    return 0;
}