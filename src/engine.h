#pragma once

/* Ob is a 1-based pointer type; 0 denotes null. */
typedef uint32_t Ob;

Ob hstar_simplify_app(Ob lhs, Ob rhs);
Ob hstar_reduce_app(Ob lhs, Ob rhs, int* budget);

Ob hstar_simplify(Ob ob);
Ob hstar_reduce(Ob ob, int* budget);
