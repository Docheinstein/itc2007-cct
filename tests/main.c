#include <glib.h>
#include <stdio.h>
#include <utils/io_utils.h>
#include <heuristics/neighbourhoods/swap.h>
#include "renderer/renderer.h"
#include "heuristics/neighbourhoods/swap.h"
#include "utils/array_utils.h"
#include "utils/str_utils.h"
#include "utils/os_utils.h"
#include "utils/rand_utils.h"
#include "model/model_parser.h"
#include "solution/solution_parser.h"
#include "solution/solution.h"
#include "log/verbose.h"
#include "finder/feasible_solution_finder.h"

#define VERBOSITY 2


#define GLIB_TEST(func_name) static void func_name()
#define GLIB_TEST_ARG(func_name) static void func_name(gconstpointer arg)

#define GLIB_ADD_TEST(test_name, func_name) g_test_add_func(test_name, func_name)
#define GLIB_ADD_TEST_ARG(test_name, func_name, arg) g_test_add_data_func(test_name, arg, func_name)

#define g_assert_eqstr(a, b) g_assert_cmpstr(a, ==, b)
#define g_assert_cmpbool(a, op, b) g_assert_cmpint(a, op, b)

#define BUFLEN 256

GLIB_TEST(test_strpos) {
    g_assert_cmpint(strpos("Hello", 'H'), ==, 0);
    g_assert_cmpint(strpos("Hello", 'o'), ==, 4);
    g_assert_cmpint(strpos("Hello", 'y'), <, 0);
}

GLIB_TEST(test_strltrim) {
    g_assert_eqstr(strltrim("  something"), "something");
    g_assert_eqstr(strltrim("\n\tsomething"), "something");
    g_assert_eqstr(strltrim("something"), "something");
    g_assert_eqstr(strltrim("something  "), "something  ");
    g_assert_eqstr(strltrim(""), "");
    g_assert_eqstr(strltrim(" "), "");
}

GLIB_TEST(test_strrtrim) {
    char s1[] = "something  ";
    char s2[] = "something\n\t";
    char s3[] = "something";
    char s4[] = "  something";
    char s5[] = "";
    char s6[] = " ";

    g_assert_eqstr(strrtrim(s1), "something");
    g_assert_eqstr(strrtrim(s2), "something");
    g_assert_eqstr(strrtrim(s3), "something");
    g_assert_eqstr(strrtrim(s4), "  something");
    g_assert_eqstr(strltrim(s5), "");
    g_assert_eqstr(strltrim(s6), "");
}

GLIB_TEST(test_strtrim) {
    char s1[] = " something  ";
    char s2[] = "\tsomething\n\t";
    char s3[] = "\nsomething";
    char s4[] = "  something";
    char s5[] = "";
    char s6[] = " ";
    char s7[] = " some thing  ";

    g_assert_eqstr(strtrim(s1), "something");
    g_assert_eqstr(strtrim(s2), "something");
    g_assert_eqstr(strtrim(s3), "something");
    g_assert_eqstr(strtrim(s4), "something");
    g_assert_eqstr(strtrim(s5), "");
    g_assert_eqstr(strtrim(s6), "");
    g_assert_eqstr(strtrim(s7), "some thing");
}

GLIB_TEST(test_strltrim_chars) {
    g_assert_eqstr(strltrim_chars("XXsomething", "X"), "something");
    g_assert_eqstr(strltrim_chars("\n\tsomething", "\n\t"), "something");
    g_assert_eqstr(strltrim_chars("something", "X"), "something");
    g_assert_eqstr(strltrim_chars("something  ", " "), "something  ");
    g_assert_eqstr(strltrim_chars("", "X"), "");
    g_assert_eqstr(strltrim_chars(" ", " "), "");
}

GLIB_TEST(test_strrtrim_chars) {
    char s1[] = "something  X";
    char s2[] = "something\n\t";
    char s3[] = "something";
    char s4[] = "  something";
    char s5[] = "";
    char s6[] = " ";

    g_assert_eqstr(strrtrim_chars(s1, "X"), "something  ");
    g_assert_eqstr(strrtrim_chars(s2, "\n\t"), "something");
    g_assert_eqstr(strrtrim_chars(s3, "X"), "something");
    g_assert_eqstr(strrtrim_chars(s4, "X"), "  something");
    g_assert_eqstr(strrtrim_chars(s5, " "), "");
    g_assert_eqstr(strrtrim_chars(s6, " "), "");
}

GLIB_TEST(test_strtrim_chars) {
    char s1[] = " something  ";
    char s2[] = "\tsomething\n\t ";
    char s3[] = "\nsomething";
    char s4[] = "  something";
    char s5[] = "";
    char s6[] = " ";
    char s7[] = " some thing  ";

    g_assert_eqstr(strtrim_chars(s1, " "), "something");
    g_assert_eqstr(strtrim_chars(s2, " \t\n"), "something");
    g_assert_eqstr(strtrim_chars(s3, " "), "\nsomething");
    g_assert_eqstr(strtrim_chars(s4, " "), "something");
    g_assert_eqstr(strtrim_chars(s5, " "), "");
    g_assert_eqstr(strtrim_chars(s6, " "), "");
    g_assert_eqstr(strtrim_chars(s7, " "), "some thing");
}

GLIB_TEST(test_strtoint) {
    int value;

    g_assert_true(strtoint("10", &value));
    g_assert_cmpint(value, ==, 10);

    g_assert_true(strtoint("+10", &value));
    g_assert_cmpint(value, ==, 10);

    g_assert_true(strtoint("-10", &value));
    g_assert_cmpint(value, ==, -10);

    g_assert_false(strtoint("10a", &value));
    g_assert_false(strtoint("a10", &value));
    g_assert_false(strtoint("abc", &value));
}

GLIB_TEST(test_strappend) {
    char s[BUFLEN];

    snprintf(s, BUFLEN, "First");
    strappend(s, BUFLEN, "Second");
    g_assert_eqstr(s, "FirstSecond");

    char s2[4] = "he";
    strappend(s2, 4, "hello");
    g_assert_eqstr(s2, "heh");

    char s3[BUFLEN] = "";
    strappend(s3, BUFLEN, "hello");
    g_assert_eqstr(s3, "hello");

    const char s4[BUFLEN] = "First";
    const char s5[BUFLEN] = "Second";
    char s6[BUFLEN] = "";

    strappend(s6, BUFLEN, s4);
    strappend(s6, BUFLEN, s5);
    g_assert_eqstr(s6, "FirstSecond");

    char s11[BUFLEN];
    snprintf(s11, BUFLEN, "First");
    strappend(s11, BUFLEN, "Second %d", 10);
    g_assert_eqstr(s11, "FirstSecond 10");

    char s12[5] = "he";
    strappend(s12, 5, "%s%d", "h", 10);
    g_assert_eqstr(s12, "heh1");
}

GLIB_TEST(test_strappend_realloc) {
    size_t size = 8;
    char *s1 = malloc(sizeof(char) * size);
    char *s2 = s1;
    snprintf(s1, 8, "buffer");
    strappend_realloc(&s2, &size, "!");
    g_assert_cmpint(size, ==, 8);
    g_assert_true(s1 == s2);
    g_assert_eqstr(s2, "buffer!");
    free(s1);

    char *s11 = malloc(sizeof(char) * size);
    char *s12 = s11;
    snprintf(s11, size, "buffer");
    strappend_realloc(&s12, &size, "full");
    g_assert_cmpint(size, >, 8);
    g_assert_eqstr(s12, "bufferfull");
    free(s12);
}

GLIB_TEST(test_strsplit) {
    const int MAX_TOKENS = 16;
    char *tokens[MAX_TOKENS];

    char s[] = "this is a space separated  string";
    int n_tokens = strsplit(s, " ", tokens, MAX_TOKENS);

    g_assert_cmpint(n_tokens, ==, 6);
    g_assert_eqstr(tokens[0], "this");
    g_assert_eqstr(tokens[1], "is");
    g_assert_eqstr(tokens[2], "a");
    g_assert_eqstr(tokens[3], "space");
    g_assert_eqstr(tokens[4], "separated");
    g_assert_eqstr(tokens[5], "string");


    char s2[] = "dog cat elephant pig snake";
    n_tokens = strsplit(s2, " ", tokens, 4);

    g_assert_cmpint(n_tokens, ==, 4);
    g_assert_eqstr(tokens[0], "dog");
    g_assert_eqstr(tokens[1], "cat");
    g_assert_eqstr(tokens[2], "elephant");
    g_assert_eqstr(tokens[3], "pig");
    g_assert_eqstr(tokens[4], "separated");
    g_assert_eqstr(tokens[5], "string");
}

GLIB_TEST(test_strjoin) {
    char *strings[] = {
        "first",
        "second",
        "third"
    };

    char *s = strjoin(strings, 3, ", ");
    g_assert_eqstr(s, "first, second, third");
    free(s);

    char *s2 = strjoin(strings, 3, "");
    g_assert_eqstr(s, "firstsecondthird");
    free(s2);
}


GLIB_TEST(test_pathjoin) {
    char *s;

    s = pathjoin("/tmp", "file");
    g_assert_eqstr(s, "/tmp" PATH_SEP_STR "file");
    free(s);


    s = pathjoin("/tmp/", "file");
    g_assert_eqstr(s, "/tmp" PATH_SEP_STR "file");
    free(s);

    s = pathjoin("/tmp/", "/file");
    g_assert_eqstr(s, "/tmp" PATH_SEP_STR "file");
    free(s);
}


GLIB_TEST(test_mkdirs) {
    if (!isdir("/tmp/dir")) {
        g_assert_true(mkdirs("/tmp/dir"));
    }
    g_assert_true(isdir("/tmp/dir"));
}


GLIB_TEST_ARG(test_parser) {
    const char * dataset = arg;

    model m;
    model_init(&m);

    g_assert_true(parse_model(&m, dataset));

    g_assert_eqstr("ToyExample", m.name);
    g_assert_cmpint(m.n_courses, ==, 4);
    g_assert_cmpint(m.n_rooms, ==, 2);
    g_assert_cmpint(m.n_days, ==, 5);
    g_assert_cmpint(m.n_slots, ==, 4);
    g_assert_cmpint(m.n_curriculas, ==, 2);
    g_assert_cmpint(m.n_unavailability_constraints, ==, 8);


    g_assert_eqstr("SceCosC", model_course_by_id(&m, "SceCosC")->id);
    g_assert_eqstr("ArcTec", model_course_by_id(&m, "ArcTec")->id);
    g_assert_eqstr("TecCos", model_course_by_id(&m, "TecCos")->id);
    g_assert_eqstr("Geotec", model_course_by_id(&m, "Geotec")->id);

    g_assert_eqstr("A", model_room_by_id(&m, "A")->id);
    g_assert_eqstr("B", model_room_by_id(&m, "B")->id);

    g_assert_eqstr("Cur1", model_curricula_by_id(&m, "Cur1")->id);
    g_assert_eqstr("Cur2", model_curricula_by_id(&m, "Cur2")->id);

    model_destroy(&m);
}

GLIB_TEST_ARG(test_solution_parser) {
    const char * model_file = ((const char **) arg)[0];
    const char * solution_file = ((const char **) arg)[1];

    model m;
    model_init(&m);

    g_assert_true(parse_model(&m, model_file));

    solution s;
    solution_init(&s, &m);

    g_assert_true(parse_solution(&s, solution_file));

    model_destroy(&m);
    solution_destroy(&s);
}

typedef struct test_solution_params {
    const char *model_file;
    const char *solution_file;
    bool h1, h2, h3, h4;
    int s1, s2, s3, s4;
} test_solution_params;

GLIB_TEST_ARG(test_solution_cost) {
    test_solution_params * params = (test_solution_params *) arg;

    model m;
    model_init(&m);

    g_assert_true(parse_model(&m, params->model_file));

    solution s;
    solution_init(&s, &m);

    g_assert_true(parse_solution(&s, params->solution_file));

    g_assert_cmpint(params->h1, ==, solution_satisfy_lectures(&s));
    g_assert_cmpint(params->h2, ==, solution_satisfy_room_occupancy(&s));
    g_assert_cmpint(params->h3, ==, solution_satisfy_availabilities(&s));
    g_assert_cmpint(params->h4, ==, solution_satisfy_conflicts(&s));

    g_assert_cmpint(params->s1, ==, solution_room_capacity_cost(&s));
    g_assert_cmpint(params->s2, ==, solution_min_working_days_cost(&s));
    g_assert_cmpint(params->s3, ==, solution_curriculum_compactness_cost(&s));
    g_assert_cmpint(params->s4, ==, solution_room_stability_cost(&s));

    model_destroy(&m);
    solution_destroy(&s);
}

GLIB_TEST_ARG(test_finder) {
    const char *model_file = (const char *) arg;

    model m;
    model_init(&m);

    g_assert_true(parse_model(&m, model_file));

    solution s;
    solution_init(&s, &m);

    feasible_solution_finder finder;
    feasible_solution_finder_init(&finder);

    feasible_solution_finder_config finder_config;
    feasible_solution_finder_config_default(&finder_config);

    g_assert_true(feasible_solution_finder_find(&finder, &finder_config, &s));
    g_assert_true(solution_satisfy_hard_constraints(&s));

    feasible_solution_finder_destroy(&finder);

    model_destroy(&m);
    solution_destroy(&s);
}


static void parse_model_and_find_solution(model *m, solution *s, const char *model_file) {
    model_init(m);
    parse_model(m, model_file);

    solution_init(s, m);

    feasible_solution_finder finder;
    feasible_solution_finder_init(&finder);

    feasible_solution_finder_config finder_config;
    feasible_solution_finder_config_default(&finder_config);

    feasible_solution_finder_find(&finder, &finder_config, s);

    feasible_solution_finder_destroy(&finder);
}


#define PROLOGUE(filename) \
    model m; \
    solution s; \
    parse_model_and_find_solution(&m, &s, filename); \
    MODEL(&m);


#define EPILOGUE() \
    model_destroy(&m); \
    solution_destroy(&s);


GLIB_TEST_ARG(test_swap_iter_next) {
    const char *model_file = (const char *) arg;
    PROLOGUE(model_file);

    swap_iter iter;
    swap_iter_init(&iter, &s);

    while (swap_iter_next(&iter)) {
        g_assert_true(s.timetable_crds[INDEX4(
                iter.move.helper.c1, C, iter.move.helper.r1, R,
                iter.move.helper.d1, D, iter.move.helper.s1, S)]);
        g_assert_true(swap_move_is_effective(&iter.move));
    }

    EPILOGUE();
}

typedef struct test_swap_effectiveness_params {
    const char *model_file;
    int trials;
} test_swap_effectiveness_params;


GLIB_TEST_ARG(test_swap_effectiveness) {
    test_swap_effectiveness_params *params = (test_swap_effectiveness_params *) arg;
    PROLOGUE(params->model_file);

    swap_move mv;
    swap_result result;

    unsigned long long h = solution_fingerprint(&s);

    for (int i = 0; i < params->trials; i++) {
        swap_move_generate_random_raw(&s, &mv);
        swap_predict(&s, &mv,
                     NEIGHBOURHOOD_PREDICT_FEASIBILITY_ALWAYS,
                     NEIGHBOURHOOD_PREDICT_COST_NEVER,
                     &result);

        if (!result.feasible)
            continue;

        swap_perform(&s, &mv, NEIGHBOURHOOD_PERFORM_ALWAYS, NULL);

        unsigned long long h2 = solution_fingerprint(&s);

        if (swap_move_is_effective(&mv))
            g_assert_cmpuint(h, !=, h2);
        else
            g_assert_cmpuint(h, ==, h2);
        h = h2;
    }

    EPILOGUE();
}


typedef struct test_swap_reverse_params {
    const char *model_file;
    int trials;
} test_swap_reverse_params;


GLIB_TEST_ARG(test_swap_reverse) {
    test_swap_reverse_params *params = (test_swap_reverse_params *) arg;
    PROLOGUE(params->model_file);

    swap_move mv;

    unsigned long long h = solution_fingerprint(&s);

    for (int i = 0; i < params->trials; i++) {
        swap_move_generate_random_feasible_effective(&s, &mv);
        swap_perform(&s, &mv, NEIGHBOURHOOD_PERFORM_ALWAYS, NULL);
        swap_move mv_back;
        swap_move_reverse(&mv, &mv_back);
        swap_perform(&s, &mv_back, NEIGHBOURHOOD_PERFORM_ALWAYS, NULL);
        unsigned long long h2 = solution_fingerprint(&s);
        g_assert_cmpuint(h, ==, h2);

    }

    EPILOGUE();
}


typedef struct test_swap_feasibility_params {
    const char *model_file;
    int trials;
} test_swap_feasibility_params;


GLIB_TEST_ARG(test_swap_feasibility) {
    test_swap_feasibility_params *params = (test_swap_feasibility_params *) arg;
    PROLOGUE(params->model_file);

    swap_move mv;
    swap_result result;

    for (int i = 0; i < params->trials; i++) {
        swap_move_generate_random_extended(&s, &mv, true, false);
        swap_predict(&s, &mv,
                     NEIGHBOURHOOD_PREDICT_FEASIBILITY_ALWAYS,
                     NEIGHBOURHOOD_PREDICT_COST_NEVER,
                     &result);
        swap_perform(&s, &mv, NEIGHBOURHOOD_PERFORM_ALWAYS, NULL);

        bool h1 = solution_satisfy_lectures(&s);
        bool h2 = solution_satisfy_room_occupancy(&s);
        bool h3 = solution_satisfy_conflicts(&s);
        bool h4 = solution_satisfy_availabilities(&s);
        if (result.feasible)
            g_assert_true(h1 && h2 && h3 && h4);
        else
            g_assert_false(h1 && h2 && h3 && h4);

        swap_move mv_back;
        swap_move_reverse(&mv, &mv_back);
        swap_perform(&s, &mv_back, NEIGHBOURHOOD_PERFORM_ALWAYS, NULL);
    }

    EPILOGUE();
}

typedef struct test_swap_cost_params {
    const char *model_file;
    int trials;
} test_swap_cost_params;


GLIB_TEST_ARG(test_swap_cost) {
    test_swap_cost_params *params = (test_swap_cost_params *) arg;
    PROLOGUE(params->model_file);

    swap_move mv;
    swap_result result;

    int rc = solution_room_capacity_cost(&s);
    int mwd = solution_min_working_days_cost(&s);
    int cc = solution_curriculum_compactness_cost(&s);
    int rs = solution_room_stability_cost(&s);

    for (int i = 0; i < params->trials; i++) {
        swap_move_generate_random_feasible_effective(&s, &mv);
        swap_predict(&s, &mv,
                     NEIGHBOURHOOD_PREDICT_FEASIBILITY_NEVER,
                     NEIGHBOURHOOD_PREDICT_COST_ALWAYS,
                     &result);
//        print("Moving %s from (%s,%d,%d) to (%s,%d,%d) - cost (rc=%d, mwd=%d, cc=%d, rs=%d)",
//              model->courses[mv.helper.c1].id, model->rooms[mv.helper.r1].id, mv.helper.d1, mv.helper.s1,
//              model->rooms[mv.r2].id, mv.d2, mv.s2,
//              result.delta.room_capacity_cost, result.delta.min_working_days_cost,
//              result.delta.curriculum_compactness_cost, result.delta.room_stability_cost);
        swap_perform(&s, &mv, NEIGHBOURHOOD_PERFORM_ALWAYS, NULL);
        g_assert_cmpint(rc + result.delta.room_capacity_cost, ==, solution_room_capacity_cost(&s));
        g_assert_cmpint(mwd + result.delta.min_working_days_cost, ==, solution_min_working_days_cost(&s));
        g_assert_cmpint(cc + result.delta.curriculum_compactness_cost, ==, solution_curriculum_compactness_cost(&s));
        g_assert_cmpint(rs + result.delta.room_stability_cost, ==, solution_room_stability_cost(&s));
        rc += result.delta.room_capacity_cost;
        mwd += result.delta.min_working_days_cost;
        cc += result.delta.curriculum_compactness_cost;
        rs += result.delta.room_stability_cost;
    }

    EPILOGUE();
}


int main(int argc, char *argv[]) {
    set_verbosity(0  );

    unsigned int my_seed;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    my_seed = ts.tv_nsec ^ ts.tv_sec;

    for (int i = 0; i < argc - 1; i++) {
        if (streq(argv[i], "--my-seed"))
            if (!strtouint(argv[i + 1], &my_seed))
                my_seed = ts.tv_nsec ^ ts.tv_sec;
    }
    rand_set_seed(my_seed);
    printf("# my seed: %u\n", my_seed);

    g_test_init(&argc, &argv, NULL);

    GLIB_ADD_TEST("/str/strpos", test_strpos);
    GLIB_ADD_TEST("/str/strltrim", test_strltrim);
    GLIB_ADD_TEST("/str/strrtrim", test_strrtrim);
    GLIB_ADD_TEST("/str/strtrim", test_strtrim);
    GLIB_ADD_TEST("/str/strltrim_chars", test_strltrim_chars);
    GLIB_ADD_TEST("/str/strrtrim_chars", test_strrtrim_chars);
    GLIB_ADD_TEST("/str/strtrim_chars", test_strtrim_chars);
    GLIB_ADD_TEST("/str/strtoint", test_strtoint);
    GLIB_ADD_TEST("/str/strappend", test_strappend);
    GLIB_ADD_TEST("/str/strappend_realloc", test_strappend_realloc);
    GLIB_ADD_TEST("/str/strsplit", test_strsplit);
    GLIB_ADD_TEST("/str/strjoin", test_strjoin);

    GLIB_ADD_TEST("/os/pathjoin", test_pathjoin);
    GLIB_ADD_TEST("/os/mkdirs", test_mkdirs);

    GLIB_ADD_TEST_ARG("/itc/test_parser/toy", test_parser, "datasets/toy.ctt");

    const char *_1[] = {"datasets/toy.ctt", "tests/solutions/toy.ctt.sol"};
    GLIB_ADD_TEST_ARG("/itc/solution_parser/toy", test_solution_parser, _1);

    test_solution_params _2 = {
        .model_file = "datasets/toy.ctt",
        .solution_file = "tests/solutions/toy.ctt.sol",
        .h1 = true, .h2 = true, .h3 = true, .h4 = true,
        .s1 = 0, .s2 = 0, .s3 = 0, .s4 = 0
    };
    GLIB_ADD_TEST_ARG("/itc/solution_cost/toy/0", test_solution_cost, &_2);

    test_solution_params _3 = {
        .model_file = "datasets/comp01.ctt",
        .solution_file = "tests/solutions/comp01.ctt.sol",
        .h1 = true, .h2 = true, .h3 = true, .h4 = true,
        .s1 = 4, .s2 = 0, .s3 = 0, .s4 = 1
    };
    GLIB_ADD_TEST_ARG("/itc/solution_cost/comp01/0", test_solution_cost, &_3);

    test_solution_params _4 = {
        .model_file = "datasets/comp01.ctt",
        .solution_file = "tests/solutions/comp01.ctt.sol.463",
        .h1 = true, .h2 = true, .h3 = true, .h4 = true,
        .s1 = 424, .s2 = 0, .s3 = 0, .s4 = 39
    };
    GLIB_ADD_TEST_ARG("/itc/solution_cost/comp01/463", test_solution_cost, &_4);

    GLIB_ADD_TEST_ARG("/itc/finder/toy", test_finder, "datasets/toy.ctt");
    GLIB_ADD_TEST_ARG("/itc/finder/comp03", test_finder, "datasets/comp03.ctt");

    GLIB_ADD_TEST_ARG("/itc/swap_iter_next/comp01", test_swap_iter_next, "datasets/comp01.ctt");

    test_swap_effectiveness_params _5 = {
        .model_file = "datasets/toy.ctt",
        .trials = 10000
    };
    GLIB_ADD_TEST_ARG("/itc/swap_effectiveness/toy", test_swap_effectiveness, &_5);

    test_swap_effectiveness_params _6 = {
        .model_file = "datasets/comp01.ctt",
        .trials = 5000
    };
    GLIB_ADD_TEST_ARG("/itc/swap_effectiveness/comp01", test_swap_effectiveness, &_6);

    test_swap_effectiveness_params _7 = {
        .model_file = "datasets/comp03.ctt",
        .trials = 2000
    };
    GLIB_ADD_TEST_ARG("/itc/swap_effectiveness/comp03", test_swap_effectiveness, &_7);

    test_swap_feasibility_params _8 = {
        .model_file = "datasets/comp01.ctt",
        .trials = 10000
    };
    GLIB_ADD_TEST_ARG("/itc/swap_reverse/comp01", test_swap_reverse, &_8);

    test_swap_feasibility_params _9 = {
        .model_file = "datasets/comp01.ctt",
        .trials = 10000
    };
    GLIB_ADD_TEST_ARG("/itc/swap_feasibility/comp01", test_swap_feasibility, &_9);

    test_swap_feasibility_params _10 = {
        .model_file = "datasets/comp03.ctt",
        .trials = 5000
    };
    GLIB_ADD_TEST_ARG("/itc/swap_feasibility/comp03", test_swap_feasibility, &_10);

    test_swap_cost_params _11 = {
        .model_file = "datasets/comp01.ctt",
        .trials = 50000
    };
    GLIB_ADD_TEST_ARG("/itc/swap_cost/comp01", test_swap_cost, &_11);

    test_swap_cost_params _12 = {
        .model_file = "datasets/comp03.ctt",
        .trials = 50000
    };
    GLIB_ADD_TEST_ARG("/itc/swap_cost/comp03", test_swap_cost, &_12);

    test_swap_cost_params _13 = {
        .model_file = "datasets/comp04.ctt",
        .trials = 50000
    };
    GLIB_ADD_TEST_ARG("/itc/swap_cost/comp04", test_swap_cost, &_13);

    test_swap_cost_params _14 = {
        .model_file = "datasets/comp07.ctt",
        .trials = 50000
    };
    GLIB_ADD_TEST_ARG("/itc/swap_cost/comp07", test_swap_cost, &_14);

    g_test_run();
}