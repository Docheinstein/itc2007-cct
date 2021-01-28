#include <stdio.h>
#include <solution/solution.h>
#include <model/model_parser.h>
#include "munit/munit.h"
#include <log/verbose.h>
#include <utils/str_utils.h>
#include <utils/def_utils.h>
#include <utils/io_utils.h>
#include <utils/array_utils.h>
#include <log/debug.h>
#include <utils/random_utils.h>
#include <heuristics/neighbourhoods/swap.h>
#include <finder/feasible_solution_finder.h>
#include <renderer/renderer.h>
#include "utils/os_utils.h"

#define BUFLEN 128

#define MUNIT_TEST(name) static MunitResult name(const MunitParameter params[], void* data)
#define MUNIT_TESTS_END { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }


// ===================== BEGIN TESTS =====================

MUNIT_TEST(test_strpos) {
    munit_assert_int(strpos("Hello", 'H'), ==, 0);
    munit_assert_int(strpos("Hello", 'o'), ==, 4);
    munit_assert_int(strpos("Hello", 'y'), <, 0);
    return MUNIT_OK;
}

MUNIT_TEST(test_strltrim) {
    munit_assert_string_equal(strltrim("  something"), "something");
    munit_assert_string_equal(strltrim("\n\tsomething"), "something");
    munit_assert_string_equal(strltrim("something"), "something");
    munit_assert_string_equal(strltrim("something  "), "something  ");
    munit_assert_string_equal(strltrim(""), "");
    munit_assert_string_equal(strltrim(" "), "");

    return MUNIT_OK;
}

MUNIT_TEST(test_strrtrim) {
    char s1[] = "something  ";
    char s2[] = "something\n\t";
    char s3[] = "something";
    char s4[] = "  something";
    char s5[] = "";
    char s6[] = " ";

    munit_assert_string_equal(strrtrim(s1), "something");
    munit_assert_string_equal(strrtrim(s2), "something");
    munit_assert_string_equal(strrtrim(s3), "something");
    munit_assert_string_equal(strrtrim(s4), "  something");
    munit_assert_string_equal(strltrim(s5), "");
    munit_assert_string_equal(strltrim(s6), "");

    return MUNIT_OK;
}

MUNIT_TEST(test_strtrim) {
    char s1[] = " something  ";
    char s2[] = "\tsomething\n\t";
    char s3[] = "\nsomething";
    char s4[] = "  something";
    char s5[] = "";
    char s6[] = " ";
    char s7[] = " some thing  ";

    munit_assert_string_equal(strtrim(s1), "something");
    munit_assert_string_equal(strtrim(s2), "something");
    munit_assert_string_equal(strtrim(s3), "something");
    munit_assert_string_equal(strtrim(s4), "something");
    munit_assert_string_equal(strtrim(s5), "");
    munit_assert_string_equal(strtrim(s6), "");
    munit_assert_string_equal(strtrim(s7), "some thing");

    return MUNIT_OK;
}

MUNIT_TEST(test_strltrim_chars) {
    munit_assert_string_equal(strltrim_chars("XXsomething", "X"), "something");
    munit_assert_string_equal(strltrim_chars("\n\tsomething", "\n\t"), "something");
    munit_assert_string_equal(strltrim_chars("something", "X"), "something");
    munit_assert_string_equal(strltrim_chars("something  ", " "), "something  ");
    munit_assert_string_equal(strltrim_chars("", "X"), "");
    munit_assert_string_equal(strltrim_chars(" ", " "), "");

    return MUNIT_OK;
}

MUNIT_TEST(test_strrtrim_chars) {
    char s1[] = "something  X";
    char s2[] = "something\n\t";
    char s3[] = "something";
    char s4[] = "  something";
    char s5[] = "";
    char s6[] = " ";

    munit_assert_string_equal(strrtrim_chars(s1, "X"), "something  ");
    munit_assert_string_equal(strrtrim_chars(s2, "\n\t"), "something");
    munit_assert_string_equal(strrtrim_chars(s3, "X"), "something");
    munit_assert_string_equal(strrtrim_chars(s4, "X"), "  something");
    munit_assert_string_equal(strrtrim_chars(s5, " "), "");
    munit_assert_string_equal(strrtrim_chars(s6, " "), "");

    return MUNIT_OK;
}

MUNIT_TEST(test_strtrim_chars) {
    char s1[] = " something  ";
    char s2[] = "\tsomething\n\t ";
    char s3[] = "\nsomething";
    char s4[] = "  something";
    char s5[] = "";
    char s6[] = " ";
    char s7[] = " some thing  ";

    munit_assert_string_equal(strtrim_chars(s1, " "), "something");
    munit_assert_string_equal(strtrim_chars(s2, " \t\n"), "something");
    munit_assert_string_equal(strtrim_chars(s3, " "), "\nsomething");
    munit_assert_string_equal(strtrim_chars(s4, " "), "something");
    munit_assert_string_equal(strtrim_chars(s5, " "), "");
    munit_assert_string_equal(strtrim_chars(s6, " "), "");
    munit_assert_string_equal(strtrim_chars(s7, " "), "some thing");

    return MUNIT_OK;
}

MUNIT_TEST(test_strtoint) {
    bool ok;

    munit_assert_int(strtoint("10", &ok), ==, 10); munit_assert_true(ok);
    munit_assert_int(strtoint("+10", &ok), ==, 10); munit_assert_true(ok);
    munit_assert_int(strtoint("0", &ok), ==, 0); munit_assert_true(ok);
    munit_assert_int(strtoint("-10", &ok), ==, -10); munit_assert_true(ok);

    strtoint("10a", &ok); munit_assert_false(ok);
    strtoint("a10", &ok); munit_assert_false(ok);
    strtoint("abc", &ok); munit_assert_false(ok);


    return MUNIT_OK;
}

MUNIT_TEST(test_strappend) {
    char s[BUFLEN];

    snprintf(s, BUFLEN, "First");
    strappend(s, BUFLEN, "Second");
    munit_assert_string_equal(s, "FirstSecond");

    char s2[4] = "he";
    strappend(s2, 4, "hello");
    munit_assert_string_equal(s2, "heh");

    char s3[BUFLEN] = "";
    strappend(s3, BUFLEN, "hello");
    munit_assert_string_equal(s3, "hello");

    const char s4[BUFLEN] = "First";
    const char s5[BUFLEN] = "Second";
    char s6[BUFLEN] = "";

    strappend(s6, BUFLEN, s4);
    strappend(s6, BUFLEN, s5);
    munit_assert_string_equal(s6, "FirstSecond");

    char s11[BUFLEN];
    snprintf(s11, BUFLEN, "First");
    strappend(s11, BUFLEN, "Second %d", 10);
    munit_assert_string_equal(s11, "FirstSecond 10");

    char s12[5] = "he";
    strappend(s12, 5, "%s%d", "h", 10);
    munit_assert_string_equal(s12, "heh1");

    return MUNIT_OK;
}

MUNIT_TEST(test_strsplit) {
    const int MAX_TOKENS = 16;
    char *tokens[MAX_TOKENS];

    char s[] = "this is a space separated  string";
    int n_tokens = strsplit(s, " ", tokens, MAX_TOKENS);

    munit_assert_int(n_tokens, ==, 6);
    munit_assert_string_equal(tokens[0], "this");
    munit_assert_string_equal(tokens[1], "is");
    munit_assert_string_equal(tokens[2], "a");
    munit_assert_string_equal(tokens[3], "space");
    munit_assert_string_equal(tokens[4], "separated");
    munit_assert_string_equal(tokens[5], "string");


    char s2[] = "dog cat elephant pig snake";
    n_tokens = strsplit(s2, " ", tokens, 4);

    munit_assert_int(n_tokens, ==, 4);
    munit_assert_string_equal(tokens[0], "dog");
    munit_assert_string_equal(tokens[1], "cat");
    munit_assert_string_equal(tokens[2], "elephant");
    munit_assert_string_equal(tokens[3], "pig");
    munit_assert_string_equal(tokens[4], "separated");
    munit_assert_string_equal(tokens[5], "string");

    return MUNIT_OK;
}

MUNIT_TEST(test_strjoin) {
    char *strings[] = {
        "first",
        "second",
        "third"
    };

    char *s = strjoin(strings, 3, ", ");
    munit_assert_string_equal(s, "first, second, third");
    free(s);

    char *s2 = strjoin(strings, 3, "");
    munit_assert_string_equal(s, "firstsecondthird");
    free(s2);

    return MUNIT_OK;
}

//MUNIT_TEST(test_strmake) {
//    char * s = strmake("String of unknown length created dynamically");
//    munit_assert_string_equal(s, "String of unknown length created dynamically");
//    free(s);
//
//    return MUNIT_OK;
//}

MUNIT_TEST(test_strappend_realloc) {
    size_t size = 8;
    char *s1 = malloc(sizeof(char) * size);
    char *s2 = s1;
    snprintf(s1, 8, "buffer");
    strappend_realloc(&s2, &size, "!");
    munit_assert_int(size, ==, 8);
    munit_assert_ptr_equal(s1, s2);
    munit_assert_string_equal(s2, "buffer!");
    free(s1);

    char *s11 = malloc(sizeof(char) * size);
    char *s12 = s11;
    snprintf(s11, size, "buffer");
    strappend_realloc(&s12, &size, "full");
    munit_assert_int(size, >, 8);
    munit_assert_string_equal(s12, "bufferfull");
    free(s12);

    return MUNIT_OK;
}

MUNIT_TEST(test_pathjoin) {
    char *s;

    s = pathjoin("/tmp", "file");
    munit_assert_string_equal(s, "/tmp" PATH_SEP_STR "file");
    free(s);


    s = pathjoin("/tmp/", "file");
    munit_assert_string_equal(s, "/tmp" PATH_SEP_STR "file");
    free(s);

    s = pathjoin("/tmp/", "/file");
    munit_assert_string_equal(s, "/tmp" PATH_SEP_STR "file");
    free(s);

    return MUNIT_OK;
}


MUNIT_TEST(test_mkdirs) {
    if (!isdir("/tmp/dir")) {
        munit_assert_true(mkdirs("/tmp/dir"));
    }
    munit_assert_true(isdir("/tmp/dir"));
    return MUNIT_OK;
}


MUNIT_TEST(test_parser) {
    model m;
    model_init(&m);

    model_parser parser;
    model_parser_init(&parser);
    munit_assert_true(model_parser_parse(&parser, "datasets/toy.ctt", &m));

    munit_assert_int(m.n_courses, ==, 4);
    munit_assert_int(m.n_rooms, ==, 2);
    munit_assert_int(m.n_days, ==, 5);
    munit_assert_int(m.n_slots, ==, 4);
    munit_assert_int(m.n_curriculas, ==, 2);
    munit_assert_int(m.n_unavailability_constraints, ==, 8);

    munit_assert_string_equal("SceCosC", model_course_by_id(&m, "SceCosC")->id);
    munit_assert_string_equal("ArcTec", model_course_by_id(&m, "ArcTec")->id);
    munit_assert_string_equal("TecCos", model_course_by_id(&m, "TecCos")->id);
    munit_assert_string_equal("Geotec", model_course_by_id(&m, "Geotec")->id);

    munit_assert_string_equal("A", model_room_by_id(&m, "A")->id);
    munit_assert_string_equal("B", model_room_by_id(&m, "B")->id);

    munit_assert_string_equal("Cur1", model_curricula_by_id(&m, "Cur1")->id);
    munit_assert_string_equal("Cur2", model_curricula_by_id(&m, "Cur2")->id);

    model_destroy(&m);

    return MUNIT_OK;
}

MUNIT_TEST(test_solution_parser) {
    model m;
    model_init(&m);

    model_parser parser;
    model_parser_init(&parser);

    munit_assert_true(model_parser_parse(&parser, "datasets/toy.ctt", &m));

    solution sol;
    solution_init(&sol, &m);

    solution_parser sol_parser;
    solution_parser_init(&sol_parser);

    munit_assert_true(solution_parser_parse(&sol_parser, "tests/solutions/toy.ctt.sol", &sol));

    model_parser_destroy(&parser);
    solution_parser_destroy(&sol_parser);
    model_destroy(&m);
    solution_destroy(&sol);

    return MUNIT_OK;
}

MUNIT_TEST(test_solution_to_string) {
    model m;
    model_init(&m);

    model_parser parser;
    model_parser_init(&parser);

    munit_assert_true(model_parser_parse(&parser, "datasets/toy.ctt", &m));

    solution sol;
    solution_init(&sol, &m);

    solution_parser sol_parser;
    solution_parser_init(&sol_parser);

    munit_assert_true(solution_parser_parse(&sol_parser, "tests/solutions/toy.ctt.sol", &sol));

    char *output = strtrim(solution_to_string(&sol));
    char *expected_solution = strtrim(fileread("tests/solutions/toy.ctt.sol"));

    munit_assert_string_equal(output, expected_solution);

    free(output);
    free(expected_solution);

    model_parser_destroy(&parser);
    solution_parser_destroy(&sol_parser);
    model_destroy(&m);
    solution_destroy(&sol);

    return MUNIT_OK;
}

static MunitResult test_solution(const char *dataset_file, const char *solution_file,
                                  bool h1, bool h2, bool h3, bool h4,
                                  int s1, int s2, int s3, int s4) {

    model m;
    model_init(&m);

    model_parser parser;
    model_parser_init(&parser);

    munit_assert_true(model_parser_parse(&parser, dataset_file, &m));

    solution sol;
    solution_init(&sol, &m);

    solution_parser sol_parser;
    solution_parser_init(&sol_parser);

    munit_assert_true(solution_parser_parse(&sol_parser, solution_file, &sol));

    munit_assert(h1 == solution_satisfy_lectures(&sol));
    munit_assert(h2 == solution_satisfy_room_occupancy(&sol));
    munit_assert(h3 == solution_satisfy_availabilities(&sol));
    munit_assert(h4 == solution_satisfy_conflicts(&sol));

    munit_assert_int(s1, ==, solution_room_capacity_cost(&sol));
    munit_assert_int(s2, ==, solution_min_working_days_cost(&sol));
    munit_assert_int(s3, ==, solution_curriculum_compactness_cost(&sol));
    munit_assert_int(s4, ==, solution_room_stability_cost(&sol));

    model_parser_destroy(&parser);
    solution_parser_destroy(&sol_parser);
    model_destroy(&m);
    solution_destroy(&sol);

    return MUNIT_OK;
}


MUNIT_TEST(test_solution_cost_toy) {
    return test_solution("datasets/toy.ctt", "tests/solutions/toy.ctt.sol",
                         true, true, true, true, 0, 0, 0 , 0);
}

MUNIT_TEST(test_solution_cost_comp01) {
    return test_solution("datasets/comp01.ctt", "tests/solutions/comp01.ctt.sol",
                         true, true, true, true, 4, 0, 0, 1);
}

MUNIT_TEST(test_solution_cost_comp01_463) {
    return test_solution("datasets/comp01.ctt", "tests/solutions/comp01.ctt.sol.463",
                         true, true, true, true, 424, 0, 0, 39);
}

MUNIT_TEST(test_solution_cost_comp01_57) {
    return test_solution("datasets/comp01.ctt", "tests/solutions/comp01.ctt.sol.57",
                         true, true, true, true, 30, 0, 6, 21);
}

MUNIT_TEST(test_solution_cost_toytoy_39) {
    return test_solution("datasets/toytoy.ctt", "tests/solutions/toytoy.ctt.sol.39",
                         true, true, true, true, 18, 15, 4, 2);
}

MUNIT_TEST(test_index_rindex) {
    const int C = rand_int_range(1, 30);
    const int R = rand_int_range(1, 30);
    const int D = rand_int_range(1, 30);
    const int S = rand_int_range(1, 30);

    int i = 0;
    for (int d = 0; d < D; d++) {
        for (int s = 0; s < S; s++) {
            munit_assert_int(INDEX2(d, D, s, S), ==, i);
            munit_assert_int(RINDEX2_0(i, D, S), ==, d);
            munit_assert_int(RINDEX2_1(i, D, S), ==, s);
            i++;
        }
    }


    i = 0;
    for (int r = 0; r < R; r++) {
        for (int d = 0; d < D; d++) {
            for (int s = 0; s < S; s++) {
                munit_assert_int(INDEX3(r, R, d, D, s, S), ==, i);
                munit_assert_int(RINDEX3_0(i, R, D, S), ==, r);
                munit_assert_int(RINDEX3_1(i, R, D, S), ==, d);
                munit_assert_int(RINDEX3_2(i, R, D, S), ==, s);
                i++;
            }
        }
    }


    i = 0;
    for (int c = 0; c < C; c++) {
        for (int r = 0; r < R; r++) {
            for (int d = 0; d < D; d++) {
                for (int s = 0; s < S; s++) {
                    munit_assert_int(INDEX4(c, C, r, R, d, D, s, S), ==, i);
                    munit_assert_int(RINDEX4_0(i, C, R, D, S), ==, c);
                    munit_assert_int(RINDEX4_1(i, C, R, D, S), ==, r);
                    munit_assert_int(RINDEX4_2(i, C, R, D, S), ==, d);
                    munit_assert_int(RINDEX4_3(i, C, R, D, S), ==, s);
                    i++;
                }
            }
        }
    }

    return MUNIT_OK;
}

static void find_feasible_solution(model *m, solution *s, const char * dataset_file) {
    model_init(m);

    model_parser parser;
    model_parser_init(&parser);
    munit_assert_true(model_parser_parse(&parser, dataset_file, m));

    feasible_solution_finder finder;
    feasible_solution_finder_init(&finder);

    feasible_solution_finder_config finder_config;
    feasible_solution_finder_config_init(&finder_config);

    solution_init(s, m);

    feasible_solution_finder_find(&finder, &finder_config, s);
    munit_assert_true(solution_satisfy_hard_constraints(s));
}

MUNIT_TEST(test_feasible_solution_finder_find_toy) {
    model m;
    solution s;
    find_feasible_solution(&m, &s, "datasets/toy.ctt");
    model_destroy(&m);
    solution_destroy(&s);
    return MUNIT_OK;
}

MUNIT_TEST(test_feasible_solution_finder_find_comp01) {
    model m;
    solution s;
    find_feasible_solution(&m, &s, "datasets/comp01.ctt");
    model_destroy(&m);
    solution_destroy(&s);
    return MUNIT_OK;
}

static MunitResult test_compute_neighbourhood_swap_cost_constraints(const char *dataset_file) {
    model m;
    model_init(&m);

    model_parser parser;
    model_parser_init(&parser);
    munit_assert_true(model_parser_parse(&parser, dataset_file, &m));

    feasible_solution_finder finder;
    feasible_solution_finder_init(&finder);

    feasible_solution_finder_config finder_config;
    feasible_solution_finder_config_init(&finder_config);

    solution sol;
    solution_init(&sol, &m);

    feasible_solution_finder_find(&finder, &finder_config, &sol);

    swap_result swap_result;
    int cost = solution_cost(&sol);

    bool improved;
    do {
        improved = false;

        swap_iter iter;
        swap_iter_init(&iter, &sol);

        swap_move mv;

        while (swap_iter_next(&iter, &mv)) {
            int c_rm = solution_room_capacity_cost(&sol);
            int c_mwd = solution_min_working_days_cost(&sol);
            int c_cc = solution_curriculum_compactness_cost(&sol);
            int c_rs = solution_room_stability_cost(&sol);

//            render_solution(&sol, "/tmp/sol-before.png");
//            write_solution(&sol, "/tmp/sol-before.txt");

            neighbourhood_swap(&sol, &mv,
                               NEIGHBOURHOOD_PREDICT_ALWAYS,
                               NEIGHBOURHOOD_PREDICT_ALWAYS,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PERFORM_ALWAYS,
                               &swap_result);

//            render_solution(&sol, "/tmp/sol-after.png");
//            write_solution(&sol, "/tmp/sol-after.txt");

            int c_rm_after = solution_room_capacity_cost(&sol);
            int c_mwd_after = solution_min_working_days_cost(&sol);
            int c_cc_after = solution_curriculum_compactness_cost(&sol);
            int c_rs_after = solution_room_stability_cost(&sol);

//            print("Is feasible: %d", swap_result.feasible);
//            print("mv: c1=%d, r1=%d, d1=%d, s1=%d, _c2=%d, r2=%d, d2=%d, s2=%d",
//                  mv.c1, mv.r1, mv.d1, mv.s1, mv._c2, mv.r2, mv.d2, mv.s2);
            munit_assert_int(c_rm_after, ==, c_rm + swap_result.delta_cost_room_capacity);
            munit_assert_int(c_mwd_after, ==, c_mwd + swap_result.delta_cost_min_working_days);
            munit_assert_int(c_cc_after, ==, c_cc + swap_result.delta_cost_curriculum_compactness);
            munit_assert_int(c_rs_after, ==, c_rs + swap_result.delta_cost_room_stability);

            if (swap_result.feasible) {
                munit_assert_true(solution_satisfy_hard_constraints(&sol));

                if (cost + swap_result.delta_cost < cost) {
                    cost = cost + swap_result.delta_cost;
                    improved = true;
                    break;
                }
            }

            neighbourhood_swap_back(&sol, &mv, &swap_result, true);
            munit_assert_true(solution_satisfy_hard_constraints(&sol));
        }
        swap_iter_destroy(&iter);
    } while (cost > 0 && improved);

    solution_destroy(&sol);
    model_destroy(&m);
    return MUNIT_OK;
}

static MunitResult test_neighbourhood_iter_next(const char *dataset_file) {
    model m;
    model_init(&m);

    model_parser parser;
    model_parser_init(&parser);
    munit_assert_true(model_parser_parse(&parser, dataset_file, &m));

    feasible_solution_finder finder;
    feasible_solution_finder_init(&finder);

    feasible_solution_finder_config finder_config;
    feasible_solution_finder_config_init(&finder_config);

    solution sol;
    solution_init(&sol, &m);

    feasible_solution_finder_find(&finder, &finder_config, &sol);

    swap_iter iter;
    swap_iter_init(&iter, &sol);

    swap_move mv;

    while (swap_iter_next(&iter, &mv)) {
        munit_assert_true(solution_get(&sol, mv.c1, mv.r1, mv.d1, mv.s1));
    }

    swap_iter_destroy(&iter);

    solution_destroy(&sol);
    model_destroy(&m);

    return MUNIT_OK;
}

MUNIT_TEST(test_neighbourhood_iter_next_comp01) {
    return test_neighbourhood_iter_next("datasets/comp01.ctt");
}


static MunitResult test_compute_neighbourhood_swap_perform_if_feasible_and_better(const char *dataset_file) {

    model m;
    model_init(&m);

    model_parser parser;
    model_parser_init(&parser);
    munit_assert_true(model_parser_parse(&parser, dataset_file, &m));

    feasible_solution_finder finder;
    feasible_solution_finder_init(&finder);

    feasible_solution_finder_config finder_config;
    feasible_solution_finder_config_init(&finder_config);

    solution sol;
    solution_init(&sol, &m);

    feasible_solution_finder_find(&finder, &finder_config, &sol);

    swap_result swap_result;
    int cost = solution_cost(&sol);

    bool improved;

    do {
        int c1, r1, d1, s1, r2, d2, s2;
        improved = false;

        solution sol_neigh;
        solution_init(&sol_neigh, &m);
        solution_copy(&sol_neigh, &sol);

        swap_iter iter;
        swap_iter_init(&iter, &sol_neigh);

        swap_move mv;

        while (swap_iter_next(&iter, &mv)) {
            bool restart = false;

            if (neighbourhood_swap(&sol_neigh, &mv,
                               NEIGHBOURHOOD_PREDICT_ALWAYS,
                               NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PERFORM_IF_FEASIBLE_AND_BETTER,
                               &swap_result)) {
                munit_assert_true(solution_satisfy_hard_constraints(&sol_neigh));
                munit_assert_true(swap_result.feasible);
                int neigh_cost = cost + swap_result.delta_cost;
                munit_assert_int(neigh_cost, <, cost);
                cost = neigh_cost;
                solution_copy(&sol, &sol_neigh);
                improved = true;
                restart = true;
            } else {
                if (swap_result.feasible) {
                    munit_assert_true(solution_satisfy_hard_constraints(&sol_neigh));
                    munit_assert_true(swap_result.feasible);
                    int neigh_cost = cost + swap_result.delta_cost;
                    munit_assert_int(neigh_cost, >=, cost);
                } else {
                    munit_assert_true(solution_satisfy_hard_constraints(&sol_neigh));
                    munit_assert_false(swap_result.feasible);
                }
            }

            if (restart)
                break;
        }

        solution_destroy(&sol_neigh);
        swap_iter_destroy(&iter);
    } while (cost > 0 && improved);

    solution_destroy(&sol);
    model_destroy(&m);
    return MUNIT_OK;
}

static MunitResult test_neighbourhood_swap_helper(const char *dataset_file) {
    model m;
    solution s;
    find_feasible_solution(&m, &s, "datasets/comp01.ctt");    bool improved;

    MODEL(&m);
    do {
        improved = false;

        swap_iter iter;
        swap_iter_init(&iter, &s);

        swap_move mv;
        struct swap_result res;

        while (swap_iter_next(&iter, &mv)) {
            neighbourhood_swap(&s, &mv,
                               NEIGHBOURHOOD_PREDICT_ALWAYS,
                               NEIGHBOURHOOD_PREDICT_IF_FEASIBLE,
                               NEIGHBOURHOOD_PREDICT_NEVER,
                               NEIGHBOURHOOD_PERFORM_NEVER,
                               &res);
            if (res.feasible && res.delta_cost < 0) {
//                solution s_copy;
//                solution_init(&s_copy, s.model);
//                solution_copy(&s_copy, &s);
//
//                const solution_helper * helper_before_swap = solution_get_helper(&s_copy);
//
                neighbourhood_swap(&s, &mv,
                   NEIGHBOURHOOD_PREDICT_NEVER,
                   NEIGHBOURHOOD_PREDICT_ALWAYS,
                   NEIGHBOURHOOD_PREDICT_NEVER,
                   NEIGHBOURHOOD_PERFORM_ALWAYS,
                   &res);
                /*
                const solution_helper * h1 = s.helper;
                s.helper = NULL; // memory leak...
                const solution_helper * h2 = solution_get_helper(&s);

                munit_assert_int(memcmp(h1->c_rds, h2->c_rds, R * D * S * sizeof(int)), ==, 0);
                munit_assert_int(memcmp(h1->r_cds, h2->r_cds, C * D * S * sizeof(int)), ==, 0);
                munit_assert_int(memcmp(h1->sum_cr, h2->sum_cr, C * R * sizeof(int)), ==, 0);
                munit_assert_int(memcmp(h1->timetable_cdsr, h2->timetable_cdsr, C * D * S * R * sizeof(bool)), ==, 0);
                munit_assert_int(memcmp(h1->sum_cds, h2->sum_cds, C * D * S * sizeof(int)), ==, 0);
                munit_assert_int(memcmp(h1->sum_cd, h2->sum_cd, C * D  * sizeof(int)), ==, 0);
                munit_assert_int(memcmp(h1->timetable_rdsc, h2->timetable_rdsc, R * D * S * C  * sizeof(bool)), ==, 0);
                munit_assert_int(memcmp(h1->sum_rds, h2->sum_rds, R * D * S * sizeof(int)), ==, 0);
                munit_assert_int(memcmp(h1->timetable_qdscr, h2->timetable_qdscr, Q * D * S * C * R * sizeof(bool)), ==, 0);
                munit_assert_int(memcmp(h1->sum_qds, h2->sum_qds, Q * D * S * sizeof(int)), ==, 0);
                munit_assert_int(memcmp(h1->timetable_tdscr, h2->timetable_tdscr, T * D * S * C * R * sizeof(bool)), ==, 0);
                munit_assert_int(memcmp(h1->sum_tds, h2->sum_tds, T * D * S * sizeof(int)), ==, 0);
                */
                improved = true;
                break;
            }
        }

        swap_iter_destroy(&iter);
    } while (improved);

    model_destroy(&m);
    solution_destroy(&s);
    return MUNIT_OK;
}

MUNIT_TEST(test_neighbourhood_swap_cost_constraints_toytoy) {
    return test_compute_neighbourhood_swap_cost_constraints("datasets/toytoy.ctt");
}

MUNIT_TEST(test_neighbourhood_swap_cost_constraints_comp01) {
    return test_compute_neighbourhood_swap_cost_constraints("datasets/comp01.ctt");
}

MUNIT_TEST(test_neighbourhood_swap_perform_if_feasible_and_better_toytoy) {
    return test_compute_neighbourhood_swap_perform_if_feasible_and_better("datasets/toytoy.ctt");
}

MUNIT_TEST(test_neighbourhood_swap_helper_toytoy) {
    return test_neighbourhood_swap_helper("datasets/toytoy.ctt");
}

MUNIT_TEST(test_neighbourhood_swap_helper_comp01) {
    return test_neighbourhood_swap_helper("datasets/comp01.ctt");
}

// ======================= END TESTS =======================

static MunitTest tests[] = {
    { "test_strpos", test_strpos, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_strltrim", test_strltrim, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_strrtrim", test_strrtrim, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_strtrim", test_strtrim, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_strltrim_chars", test_strltrim_chars, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_strrtrim_chars", test_strrtrim_chars, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_strtrim_chars", test_strtrim_chars, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_strtoint", test_strtoint, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_strappend", test_strappend, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_strappend", test_strappend, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_strsplit", test_strsplit, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_strjoin", test_strjoin, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_strappend_realloc", test_strappend_realloc, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_pathjoin", test_pathjoin, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_mkdirs", test_mkdirs, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_parser", test_parser, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_solution_parser", test_solution_parser, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_solution_to_string", test_solution_to_string, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_solution_cost_toy", test_solution_cost_toy, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_solution_cost_comp01", test_solution_cost_comp01, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_solution_cost_toytoy_39", test_solution_cost_toytoy_39, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_solution_cost_comp01_463", test_solution_cost_comp01_463, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_solution_cost_comp01_57", test_solution_cost_comp01_57, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_index_rindex", test_index_rindex, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_feasible_solution_finder_find_toy", test_feasible_solution_finder_find_toy, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_feasible_solution_finder_find_comp01", test_feasible_solution_finder_find_comp01, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_neighbourhood_iter_next_comp01", test_neighbourhood_iter_next_comp01, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"test_neighbourhood_swap_cost_constraints_toytoy", test_neighbourhood_swap_cost_constraints_toytoy, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"test_neighbourhood_swap_cost_constraints_comp01", test_neighbourhood_swap_cost_constraints_comp01, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"test_neighbourhood_swap_perform_if_feasible_and_better_toytoy", test_neighbourhood_swap_perform_if_feasible_and_better_toytoy, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"test_neighbourhood_swap_helper_toytoy", test_neighbourhood_swap_helper_toytoy, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    MUNIT_TESTS_END
};

static MunitTest test_single[] = {
    { "test_strsplit", test_strsplit, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},

//    { "test_neighbourhood_swap_helper_toytoy", test_neighbourhood_swap_helper_toytoy, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
//    { "test_neighbourhood_swap_helper_comp01", test_neighbourhood_swap_helper_comp01, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    MUNIT_TESTS_END
};

//
//static const MunitSuite suite = {
//    "", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
//};

static const MunitSuite suite = {
    "", test_single, NULL, 1, MUNIT_SUITE_OPTION_NONE
};

int main (int argc, char **argv) {
//    set_verbose(true);
    munit_suite_main(&suite, NULL, argc, argv);
}