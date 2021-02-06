#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <finder/feasible_solution_finder.h>
#include <model/model_parser.h>
#include <utils/io_utils.h>
#include <utils/time_utils.h>
#include <log/verbose.h>
#include <utils/rand_utils.h>
#include <utils/mem_utils.h>

#define VERBOSITY 0

static const char *DATASETS[] = {
        "datasets/comp01.ctt",
        "datasets/comp02.ctt",
        "datasets/comp03.ctt",
        "datasets/comp04.ctt",
        "datasets/comp05.ctt",
        "datasets/comp06.ctt",
        "datasets/comp07.ctt",
};

#define COUNT_DIFFERENT_SOLUTIONS 1

void test_finder(model *model, double ranking_randomness, int trials) {
    feasible_solution_finder finder;
    feasible_solution_finder_init(&finder);

    solution s;
    solution_init(&s, model);

    feasible_solution_finder_config finder_conf;
    feasible_solution_finder_config_default(&finder_conf);
    finder_conf.ranking_randomness = ranking_randomness;

    int successes = 0;

    GHashTable *sols = g_hash_table_new(g_int64_hash, g_int_equal);
    unsigned long long *hashes = mallocx(trials, sizeof(long));

    long start = ms();
    for (int i = 0; i < trials; i++) {
        bool success = feasible_solution_finder_try_find(&finder, &finder_conf, &s);
        successes += success;
#if COUNT_DIFFERENT_SOLUTIONS
        if (success) {
            hashes[i] = solution_fingerprint(&s);
            g_hash_table_add(sols, &hashes[i]);
        }
#endif
        solution_clear(&s);
    }

    long end = ms();

    char tmp[32] = {'\0'};
#if COUNT_DIFFERENT_SOLUTIONS
    snprintf(tmp, 32, "(%d different)", g_hash_table_size(sols));
#endif

    print("%s(r=%5f)  %5f%%   %d/%d feasible solutions %s found in %ldms (%ld sol/ms)",
          model->_filename, ranking_randomness, (double) 100 * successes / trials,
          successes, trials, tmp, end - start, trials / (end - start));

    g_hash_table_destroy(sols);
    free(hashes);
}

void test_finder_single(const char *dataset, double r_from, double r_to, double r_inc, int trials) {
    model m;
    model_init(&m);
    if (!parse_model(&m, dataset))
        return;
    double r = r_from;
    do {
        test_finder(&m, r, trials);
        r += r_inc;
    } while (r < r_to);
}

void test_finder_multi(const char **datasets, double r_from, double r_to, double r_inc, int trials) {
    const char **dataset = datasets;
    while (dataset) {
        test_finder_single(*dataset, r_from, r_to, r_inc, trials);
        dataset++;
    }
}

void tests() {
    test_finder_single("datasets/comp03.ctt", 0.33, 0.33, 0.00, 100000);
//    test_finder_multi(DATASETS, 0.20, 0.60, 0.02, 1000);
}

int main(int argc, char **argv) {
    set_verbosity(VERBOSITY);
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    rand_set_seed(ts.tv_nsec ^ ts.tv_sec);
    tests();
    print("seed: %u", rand_get_seed());
    return 0;
}