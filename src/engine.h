#pragma once

#include <stdint.h>
#include <stddef.h>

/* Ob is a 1-based pointer type; 0 denotes null. */
typedef uint32_t Ob;

// Must be called before other un_* methods.
// This is safe to call repeatedly.
void un_init();

Ob un_simplify(Ob ob);
Ob un_simplify_app(Ob lhs, Ob rhs);

Ob un_compute(Ob ob, int *budget);
Ob un_compute_app(Ob lhs, Ob rhs, int *budget);

void un_test(unsigned int seed);
