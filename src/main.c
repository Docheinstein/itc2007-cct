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
#include <argp.h>
#include <stdbool.h>
#include <exact_solver.h>
#include <io_utils.h>
#include "args.h"
#include "verbose.h"
#include "parser.h"
#include "solution.h"
#include "array_utils.h"

const char *argp_program_version = "0.1";
static char doc[] = "Solver of the Curriculum-Based Course Timetabling Problem of ITC 2007";
static char args_doc[] = "INPUT OUTPUT";

static struct argp_option options[] = {
  { "verbose", 'v', 0, 0, "Print debugging information" },
  { 0 }
};

static error_t parse_option(int key, char *arg, struct argp_state *state) {
    struct args *args = state->input;

    switch (key) {
    case 'v':
        args->verbose = true;
        break;
    case ARGP_KEY_ARG:
        switch (state->arg_num) {
        case 0:
            args->input = arg;
            break;
        case 1:
            args->output = arg;
            break;
        default:
            argp_usage(state);
        }

      break;
    case ARGP_KEY_END:
        if (state->arg_num < 1)
            argp_usage(state);
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

int main (int argc, char **argv) {
    struct argp argp = { options, parse_option, args_doc, doc };

    struct args args;
    args_init(&args);
    argp_parse(&argp, argc, argv, 0, 0, &args);
    set_verbose(args.verbose);

    char buffer[256];
    args_to_string(&args, buffer, LENGTH(buffer));
    verbose("=== ARGUMENTS ===\n"
            "%s\n"
            "=================", buffer);

    model m;
    model_init(&m);
    if (!parse_model(args.input, &m)) {
        model_destroy(&m);
        exit(EXIT_FAILURE);
    }
    model_finalize(&m);

    solution sol;
    solution_init(&sol);

    if (solver_solve(&m, &sol)) {
        char * sol_str = solution_to_string(&sol);
        print(
            "=== SOLUTION ===\n"
            "%s",
            sol_str
        );

        if (!filewrite(args.output, sol_str)) {
            eprint("ERROR: failed to write output solution to '%s' (%s)",
                   args.output, strerror(errno));
        }

        free(sol_str);
    }

    solution_destroy(&sol);
    model_destroy(&m);

    return 0;
}