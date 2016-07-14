#include <assert.h>
#include <greatest/greatest.h>
#include <stdio.h>
#include <stdlib.h>

#include "engine.h"

GREATEST_TEST test_framework(void) {
    PASS();
}

GREATEST_TEST test_engine_init(void) {
    SKIP();
    un_init();
    PASS();
}

GREATEST_TEST test_engine_test(void) {
    SKIP();
    un_init();
    int seed = 0;
    un_test(seed);
    PASS();
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();

    GREATEST_RUN_TEST(test_framework);
    GREATEST_RUN_TEST(test_engine_init);
    GREATEST_RUN_TEST(test_engine_test);

    GREATEST_MAIN_END();
}
