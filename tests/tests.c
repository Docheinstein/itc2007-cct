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


// ======================= END TESTS =======================

static MunitTest tests[] = {
    { "test_strpos", test_strpos, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    MUNIT_TESTS_END
};

static const MunitSuite suite = {
    "tests", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
};

int main (int argc, char **argv) {
    return munit_suite_main(&suite, NULL, argc, argv);
}

#endif