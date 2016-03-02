#pragma once

#include <stdint.h>
#include <stddef.h>

/* Ob is a 1-based pointer type; 0 denotes null. */
typedef uint32_t Ob;

// Returns 0 on success.
int un_init(size_t size);

Ob un_simplify_app(Ob lhs, Ob rhs);
Ob un_reduce_app(Ob lhs, Ob rhs, int *budget);

Ob un_simplify(Ob ob);
Ob un_reduce(Ob ob, int *budget);
