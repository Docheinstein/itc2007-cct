# ITC2007 CCT

University Project.

Solver of the "*Curriculum-Based Course Timetabling*" problem (3rd track of [*International Timetabling Competition 2007*](http://www.cs.qub.ac.uk/itc2007/)).

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

## Building
```
./build.sh
```

Alternatively:

```
mkdir build
cd build
cmake ..
make
```

## Usage

```
build/itc2007-cct ...
```