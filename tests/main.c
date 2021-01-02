#include <stdio.h>
#include <solution.h>
#include <parser.h>
#include "utils.h"
#include "munit/munit.h"
#include <unistd.h>
#include <log/verbose.h>

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
    char *s1 = mallocx(sizeof(char) * size);
    char *s2 = s1;
    snprintf(s1, 8, "buffer");
    strappend_realloc(&s2, &size, "!");
    munit_assert_int(size, ==, 8);
    munit_assert_ptr_equal(s1, s2);
    munit_assert_string_equal(s2, "buffer!");
    free(s1);

    char *s11 = mallocx(sizeof(char) * size);
    char *s12 = s11;
    snprintf(s11, size, "buffer");
    strappend_realloc(&s12, &size, "full");
    munit_assert_int(size, >, 8);
    munit_assert_string_equal(s12, "bufferfull");
    free(s12);

    return MUNIT_OK;
}

MUNIT_TEST(test_parser) {
    model m;
    model_init(&m);
    parse_model("datasets/toy.ctt", &m);

    munit_assert_int(m.n_courses, ==, 4);
    munit_assert_int(m.n_rooms, ==, 2);
    munit_assert_int(m.n_days, ==, 5);
    munit_assert_int(m.n_periods_per_day, ==, 4);
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

#define ASSIGNMENT(course, room, day, day_period) \
    assignment_new(                               \
        (model_course_by_id(&m, course)),         \
        (model_room_by_id(&m, room)),             \
        (day),                                    \
        (day_period)                              \
    )

MUNIT_TEST(test_toy_solution) {
    solution sol;
    solution_init(&sol);

    model m;
    model_init(&m);
    parse_model("datasets/toy.ctt", &m);

    FILE *file = fopen("tests/solutions/toy.ctt.sol", "r");
    munit_assert_not_null("tests/solutions/toy.ctt.sol");

    char line[256];
    while (fgets(line, LENGTH(line), file)) {
        line[strlen(line) - 1] = '\0';
        if (strempty(line))
            continue;
        char *F[4];
        bool ok;
        munit_assert_int(strsplit(line, " ", F, 4), ==, 4);
        solution_add_assignment(&sol,
            ASSIGNMENT(F[0], F[1], strtoint(F[2], &ok), strtoint(F[3], &ok))
        );
        munit_assert_true(ok);
    }
    char *output = strtrim(solution_to_string(&sol));
    char *expected_solution = strtrim(fileread("tests/solutions/toy.ctt.sol"));

    munit_assert_string_equal(output, expected_solution);

    free(output);
    free(expected_solution);
    model_destroy(&m);
    solution_destroy(&sol);

    return MUNIT_OK;
}

// ======================= END TESTS =======================

static MunitTest tests[] = {
    { "test_strpos", test_strpos, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_strltrim", test_strltrim, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_strrtrim", test_strrtrim, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_strtrim", test_strtrim, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_strtoint", test_strtoint, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_strappend", test_strappend, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_strappend", test_strappend, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_strsplit", test_strsplit, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_strjoin", test_strjoin, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
//    { "test_strmake", test_strmake, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_strappend_realloc", test_strappend_realloc, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_parser", test_parser, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_toy_solution", test_toy_solution, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    MUNIT_TESTS_END
};

static const MunitSuite suite = {
    "", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
};

int main (int argc, char **argv) {
//    set_verbose(true);
    return munit_suite_main(&suite, NULL, argc, argv);
}