#ifdef TEST

#include <stdio.h>
#include "utils.h"
#include "munit/munit.h"

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

// ======================= END TESTS =======================

static MunitTest tests[] = {
    { "test_strpos", test_strpos, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_strltrim", test_strltrim, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_strrtrim", test_strrtrim, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { "test_strtrim", test_strtrim, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    MUNIT_TESTS_END
};

static const MunitSuite suite = {
    "", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
};

int main (int argc, char **argv) {
    return munit_suite_main(&suite, NULL, argc, argv);
}

#endif