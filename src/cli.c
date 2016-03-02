#include "engine.h"

#include <stdio.h>

int main(int argc, char **argv) {
    un_init(1UL << 20UL);

    for (int i = 1; i < argc; ++i) {
        printf("example %s -> TODO", argv[i]);
    }

    return 0;
}
