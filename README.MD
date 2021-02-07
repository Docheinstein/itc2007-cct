# ITC2007 CCT

University Project.

Solver of the "*Curriculum-Based Course Timetabling*" problem (3rd track of [*International Timetabling Competition 2007*](http://www.cs.qub.ac.uk/itc2007/)).

Implements different metaheuristics (*Local Search*, *Hill Climbing*, *Tabu Search*, *Simulated Annealing*).

Written in C.  
Tested only on Linux (Ubuntu and Arch Linux).

## Problem formulation

The Curriculum-based timetabling problem consists of the weekly scheduling of the lectures
    for several university courses_ids within a given number of rooms and time periods, where conflicts
    between courses_ids are set according to the curricula published by the University and not on the
    basis of enrolment data.

This formulation applies to University of Udine (Italy) and to many Italian and indeed
    International Universities, although it is slightly simplified with respect to the real problem
    to maintain a certain level of generality.
    The problem consists of the following entities:

**Days, Timeslots, and Periods**. We are given a number of teaching days in the week (typ-
    ically 5 or 6). Each day is split in a fixed number of timeslots, which is equal for all days.
    A period is a pair composed of a day and a timeslot. The total number of scheduling
    periods is the product of the days times the day timeslots.

**Courses and Teachers**. Each course_id consists of a fixed number of lectures to be scheduled
    in distinct periods, it is attended by given number of students, and is taught by a
    teacher_id. For each course_id there is a minimum number of days that the lectures of the
    course_id should be spread in, moreover there are some periods in which the course_id cannot
    be scheduled.

**Rooms**. Each room has a capacity, expressed in terms of number of available seats. All
    rooms are equally suitable for all courses_ids (if large enough).

**Curricula**. A curriculum is a group of courses_ids such that any pair of courses_ids in the group
    have students in common. Based on curricula, we have the conflicts between courses_ids
    and other soft constraints.

The solution of the problem is an assignment of a period (day and timeslot) and a room
    to all lectures of each course_id.

### Hard constraints

**Lectures**: All lectures of a course_id must be scheduled, and they must be assigned to distinct
   periods. A violation occurs if a lecture is not scheduled.

**RoomOccupancy**: Two lectures cannot take place in the same room in the same period.
Two lectures in the same room at the same period represent one violation . Any extra
lecture in the same period and room counts as one more violation.

**Conflicts**: Lectures of courses_ids in the same curriculum or taught by the same teacher_id must be
all scheduled in different periods. Two conflicting lectures in the same period represent
one violation. Three conflicting lectures count as 3 violations: one for each pair.

**Availabilities**: If the teacher_id of the course_id is not available to teach that course_id at a given
period, then no lectures of the course_id can be scheduled at that period. Each lecture in
a period unavailable for that course_id is one violation.

### Soft constraints

**RoomCapacity**: For each lecture, the number of students that attend the course_id must be
       less or equal than the number of seats of all the rooms that host its lectures.Each student
       above the capacity counts as 1 point of penalty.

**MinimumWorkingDays**: The lectures of each course_id must be spread into the given mini-
       mum number of days. Each day below the minimum counts as 5 points of penalty.

**CurriculumCompactness**: Lectures belonging to a curriculum should be adjacent to each
       other (i.e., in consecutive periods). For a given curriculum we account for a violation
       every time there is one lecture not adjacent to any other lecture within the same day.
       Each isolated lecture in a curriculum counts as 2 points of penalty.

**RoomStability**: All lectures of a course_id should be given in the same room. Each distinct
        room used for the lectures of a course_id, but the first, counts as 1 point of penalty.

## Dependencies
* glib2
* cairo

For Debian/Ubuntu
```
sudo apt install libglib2.0-dev libcairo2-dev
```

For Arch Linux
```
pacman -Sy glib2 cairo
```

## Building
To build in release mode:
```
mkdir build
cd build
cmake ..
make
```

To build with `debug` statements enabled (more verbose):
```
mkdir build-debug
cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS="-DDEBUG=1" ..
make
```

To build with `assert` statements enabled (very slow):
```
mkdir build-assert
cd build-assert
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS="-DDEBUG=1 -DASSERT" ..
make
```


## Usage examples

Solve the instance `datasets/comp01.ctt` with default settings 
(*Simulated Annealing* + *Local Search*, for 60 seconds), write 
the solution to `/tmp/sol` and print the cost:
```
build/itc2007-cct datasets/comp01.ctt /tmp/sol -V
```

Solve the instance `datasets/comp01.ctt` for 
120 seconds with verbose statements, seed = 2488295934 and write
the solution to `/tmp/sol`:
```
build/itc2007-cct datasets/comp01.ctt /tmp/sol -t 120 -s 2488295934 -v
```

Solve with different methods/parameters given in config file (e.g. HC only):
```
build/itc2007-cct datasets/comp01.ctt /tmp/sol -c configs/hc.conf
```

Solve with different methods/parameters given at command line (e.g. SA only):
```
build/itc2007-cct datasets/comp01.ctt /tmp/sol -osolver.methods=sa
```

Solve the instance `datasets/comp01.ctt` in race mode: write 
the solution to `/tmp/comp01.ctt.sol` as soon as a new best is found:
```
build/itc2007-cct datasets/comp01.ctt /tmp/comp01.ctt.sol -r
```

Solve the instance `datasets/comp01.ctt`, write the solution to
`/tmp/comp01.ctt.sol` and append a line of benchmark stats to `/tmp/comp01.ctt.bench`
```
build/itc2007-cct datasets/comp01.ctt /tmp/comp01.ctt.sol -b/tmp/comp01.ctt.bench
```

Solve the instance `datasets/comp01.ctt` and render the solution to `/tmp/sol.png`:
```
build/itc2007-cct datasets/comp01.ctt -d /tmp/sol.png
```

## Validation

The option `-I` can be used to load a solution and print its cost.

e.g.  
Load the solution `/tmp/sol` of the model `datasets/comp01.ctt` and print the costs/violations without solve it:
```
build/itc2007-cct datasets/comp01.ctt -I /tmp/comp01.ctt.sol
```
```
S1(1) [RoomCapacity] penalty: course 'c0032' has 31 students but it's scheduled in room 'F' with 30 seats at (day=3, slot=2)
S1(1) [RoomCapacity] penalty: course 'c0033' has 31 students but it's scheduled in room 'F' with 30 seats at (day=2, slot=0)
S1(1) [RoomCapacity] penalty: course 'c0033' has 31 students but it's scheduled in room 'F' with 30 seats at (day=2, slot=1)
S1(1) [RoomCapacity] penalty: course 'c0033' has 31 students but it's scheduled in room 'F' with 30 seats at (day=3, slot=1)
S4(1) [RoomStability] penalty: course 'c0033' uses 2 rooms

Violations (0)
  Lectures:                    0
  RoomOccupancy:               0
  Conflicts violations:        0
  Availabilities violations:   0
Cost (5)
  RoomCapacity cost:           4
  MinWorkingDays cost:         0
  CurriculumCompactness cost:  0
  RoomStability cost:          1

```

Alternatively, the ITC2007 validator can be used as well.

```
build/itc2007-cct-validator datasets/comp01.ctt /tmp/comp01.ctt.sol
```
```
[S(1)] Room F too small for course c0032 the period 20 (day 3, timeslot 2)
[S(1)] Room F too small for course c0033 the period 12 (day 2, timeslot 0)
[S(1)] Room F too small for course c0033 the period 13 (day 2, timeslot 1)
[S(1)] Room F too small for course c0033 the period 19 (day 3, timeslot 1)
[S(1)] Course c0033 uses 2 different rooms

Violations of Lectures (hard) : 0
Violations of Conflicts (hard) : 0
Violations of Availability (hard) : 0
Violations of RoomOccupation (hard) : 0
Cost of RoomCapacity (soft) : 4
Cost of MinWorkingDays (soft) : 0
Cost of CurriculumCompactness (soft) : 0
Cost of RoomStability (soft) : 1

Summary: Total Cost = 5
```

## Benchmarks

Benchmarks can be found in `results/benchmarks` folder and are produced with the `benchmark.py` script.  

For perform all the benchmarks:
```
python benchmark.py
```

For a specific benchmark:
```
python benchmark.py comp03-sa
```

For see the stats of the benchmarks in `results/benchmarks` folder:
```
python stats.py
```

The best solutions found (about an hour of computation for each instance) are in `results/races`.

| Instance    | Best     | ITC2007 Winner |
| ----------- |:--------:|:--------------:|
| `comp01`    | **5**    | **5**          |
| `comp02`    | **36**   | 43             |
| `comp03`    | **66**   | 72             |
| `comp04`    | 37       | **35**         |
| `comp05`    | 305      | **298**        |
| `comp06`    | **40**   | 41             |
| `comp07`    | **14**   | **14**         |


## Help

`build/itc2007-ctt --help`
```
Usage: itc2007-cct [OPTION...] INPUT [OUTPUT]
Solver of the Curriculum-Based Course Timetabling Problem of ITC 2007

  -0, --no-solve             Do not solve the model: can be used with -i to
                             load a solution without solving it
  -b, --benchmark[=FILE]     Benchmark mode: print a line of stats with the
                             format <seed> <fingerprint> <cycles> <moves>
                             <feasible> <rc> <mwd> <cc> <rs> <cost> and append
                             it to FILE if it is given.
                             When used with -r, append a line of stats each
                             time a new best solution is found.
  -c, --config=FILE          Load config from FILE
  -d, --draw-overview=FILE   Draw only the overview timetable of the solution
                             and save it as FILE
  -D, --draw-all=DIR         Draw all the timetables (room, teacher, course,
                             curricula) of the solution and place them in DIR
  -i, --solution=FILE        Load the initial solution from FILE instead of
                             find it. Can be useful to continue the computation
                             from a previously computed solution (eventually
                             using another method). For print only the costs
                             and the violations of a solution (as a validator)
                             without use the solver, use -0VVqi FILE.
  -I, --validate=FILE        Print only the costs and the violations of a
                             solution in FILE (as a validator).
                             Alias for -0VVqi FILE.
  -o, --option=KEY=VALUE     Set an option KEY to VALUE (as if it was specified
                             in a config file)
  -q, --quiet                Does not print the solution to stdout
  -r, --race                 Race mode: run forever (unless -t is explicitly
                             given).
                             When a new best solution is found, it is handled
                             immediately (and depending on other options it is
                             printed to stdout, written to OUTPUT, drawn,
                             etc.).
  -s, --seed=N               Seed to use for random number generator
  -t, --time=SECONDS         Time limit in seconds (override solver.max_time)
  -v, --verbose              Print more information (-vv for print even more)
  -V, --print-violations     Print the costs and the violations of the solution
                             (-VV for print the details of each violation)
  -?, --help                 Give this help list
      --usage                Give a short usage message
      --version              Print program version

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.

The solver supports the following heuristics methods:
Local Search, Hill Climbing, Tabu Search, Simulated Annealing and Deep Local
Search.

The solver itself and each heuristics can be customized via a config file (-c
FILE) or giving options at the command line (-o KEY=VALUE).

The following options are currently supported:

SOLVER

# Comma separated list of methods among 'ls', 'hc', 'ts', 'sa', 'dls'.
solver.methods=sa,ls

# Solve for no more than N seconds.
solver.max_time=60

# Solve for no more than N cycles.
solver.max_cycles=-1

# Whether generate a new initial solution each cycle.
solver.multistart=false

# Restore the best solution so far after N non improving cycles.
# (does nothing if multistart is true).
solver.restore_best_after_cycles=50

FINDER

# Randomness of the initial feasible solution.
# (0 produces always the same deterministic solution,
# an higher value produces "more random" solutions,
# but makes it harder to find feasible ones).
finder.ranking_randomness=0.33

SIMULATED ANNEALING

# Initial temperature.
sa.initial_temperature=1.4

# Temperature's cooling rate.
sa.cooling_rate=0.965

# Minimum temperature to reach before leave the method.
sa.min_temperature=0.12

# Decrease the minimum temperature to reach to 
# sa.min_temperature * sa.min_temperature_near_best_coeff if near
# the best; the meaning of "near" can be customized with sa.near_best_ratio.
sa.min_temperature_near_best_coeff=0.68

# Consider a solution "near best" if the current solution has cost
# equal or less sa.near_best_ratio times the best solution cost.
sa.near_best_ratio=1.05

# Coefficient for the number of iterations with the same temperature.
# temperature_length = sa.temperature_length_coeff * L * R* * D * S
sa.temperature_length_coeff=0.125

LOCAL SEARCH

# Do nothing if the current solution has cost greater than 
# ls.max_distance_from_best_ratio times the best solution cost.
ls.max_distance_from_best_ratio=-1

HILL CLIMBING

# Maximum non-improving iterations number.
hc.max_idle=120000

# Increase hc.max_idle to hc.max_idle * by hc.max_idle_near_best_coeff
# if near the best (intensification); the meaning of "near" can be
# customized with hc.near_best_ratio.
hc.max_idle_near_best_coeff=3

# Consider a solution "near best" if the current solution has cost
# equal or less hc.near_best_ratio times the best solution cost.
hc.near_best_ratio=1.02

TABU SEARCH

# Maximum non-improving iterations number.
ts.max_idle=-1

# Tabu tenure (number of iterations a move remain banned).
ts.tabu_tenure=120

# Coefficient of penalty for frequently banned moves.
# tt(m) = tt + ts.frequency_penalty_coeff * freq(m)
ts.frequency_penalty_coeff=0

# Increase ts.max_idle to ts.max_idle * by ts.max_idle_near_best_coeff
# if near the best (intensification); the meaning of "near" can be
# customized with hc.near_best_ratio.
ts.max_idle_near_best_coeff=1.5

# Consider a solution "near best" if the current solution has cost
# equal or less ts.near_best_ratio times the best solution cost.
ts.near_best_ratio=1.02

DEEP LOCAL SARCH

# Do nothing if the current solution has cost greater than
# dls.max_distance_from_best_ratio times the best solution cost.
dls.max_distance_from_best_ratio=-1
```