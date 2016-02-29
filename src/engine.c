#include "engine.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#ifdef __GNUG__
#define likely(x) __builtin_expect(!!(x), true)
#define unlikely(x) __builtin_expect(!!(x), false)
#else  // __GNUG__
#warning "ignoring likely(-), unlikely(-)"
#define likely(x) (x)
#define unlikely(x) (x)
#endif  // __GNUG__

#if NDEBUG
#define HSTAR_ASSERT(cond, ...)
#else  // NDEBUG
#define HSTAR_ASSERT(cond, ...) \
  { if (unlikely(!(cond)) { fprintf(stderr, ##__VA_ARGS__ ); }
#endif  // NDEBUG
#define HSTAR_ASSERT_EQ(x, y)                                                \
  HSTAR_ASSERT((x) == (y), "expected " #x " == " #y "; actual %ull vs %ull", \
               (x), (y))

#define HSTAR_CACHE_LINE_BYTES (64UL)

// Adapted from
// https://github.com/google/farmhash/blob/master/src/farmhash.h#L167
inline uint64_t hash_64(uint64_t key) {
  // Murmur-inspired hashing.
  const uint64_t kMul = 0x9ddfea08eb382d69ULL;
  uint64_t b = key * kMul;
  b ^= (b >> 44);
  b *= kMul;
  b ^= (b >> 41);
  b *= kMul;
  return b;
}

/*----------------------------------------------------------------------------*/
/* hstar_bin_fun */

struct ob_pair_t {
    Ob lhs;
    Ob rhs;
};

struct hstar_bin_fun_node_t {
  Ob lhs;
  Ob rhs;
  Ob val;
  Ob padding[1];
};

uint64_t hstar_bin_fun_node_key(

struct hstar_bin_fun_t {
  hstar_bin_fun_node_t* buckets;
  size_t bucket_count;
};

// Returns 0 on success.
int hstar_bin_fun_init(hstar_bin_fun_t* fun, size_t bucket_count) {
  size_t bytes = sizeof(hstar_bin_fun_node_t) * bucket_count;
  int info = posix_memalign(&(fun->buckets), HSTAR_CACHE_LINE_BYTES, bytes);
  if (info != 0) {
    fprintf(stderr, "posix_memalign failed");
    return info;
  }
  fun->bucket_count = bucket_count;
  bzero(fun->buckets, bytes);
  return 0;
}

size_t hstar_bin_fun_find(Ob lhs, Ob rhs) {

}

/*----------------------------------------------------------------------------*/

struct hstar_db_t {
    hstar_bin_fun_t app_LRv;
    hstar_bin_fun_t app_LRv;
};

static hstar_db_t db;
