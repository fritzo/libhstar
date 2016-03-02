#include "engine.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#ifdef __GNUC__
#define likely(x) __builtin_expect(!!(x), true)
#define unlikely(x) __builtin_expect(!!(x), false)
#else  // __GNUC__
#warning "ignoring likely(-), unlikely(-)"
#define likely(x) (x)
#define unlikely(x) (x)
#endif  // __GNUC__

#if __STDC_VERSION__ < 201112L
#define static_assert(cond, message)
#endif  // __STDC_VERSION__ < 201112L

#ifdef NDEBUG
#define DEBUG 0
#else  // NDEBUG
#define DEBUG 1
#endif  // NDEBUG

#define UN_ERROR(...)                 \
    {                                 \
        fprintf(stderr, __VA_ARGS__); \
        fflush(stderr);               \
        abort();                      \
    }
#define UN_WARN(...)                  \
    {                                 \
        fprintf(stderr, __VA_ARGS__); \
        fflush(stderr);               \
    }

#define TODO(...) UN_ERROR("TODO " __VA_ARGS__)

#ifdef NDEBUG
#define UN_ASSERT(cond, ...)
#else  // NDEBUG
#define UN_ASSERT(cond, ...)                         \
    {                                                \
        if (unlikely(!(cond))) UN_ERROR(__VA_ARGS__) \
    }
#endif  // NDEBUG

#define UN_ASSERT_TRUE(cond) UN_ASSERT((cond), "failed assertion: " #cond)
#define UN_ASSERT_OP(x, op, y)                                               \
    UN_ASSERT((x)op(y), "expected " #x " " #op " " #y "; actual %lu vs %lu", \
              (x), (y))
#define UN_ASSERT_NE(x, y) UN_ASSERT_OP(x, !=, y)
#define UN_ASSERT_EQ(x, y) UN_ASSERT_OP(x, ==, y)
#define UN_ASSERT_LT(x, y) UN_ASSERT_OP(x, <, y)
#define UN_ASSERT_LE(x, y) UN_ASSERT_OP(x, <=, y)

#define UN_TUNE_CACHE_LINE_BYTES (64UL)

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

inline bool is_power_of_2(uint64_t x) {
    return ((x != 0UL) && !(x & (x - 1UL)));
}

typedef int Info;
static const Info Info_success = 0;

// ---------------------------------------------------------------------------
// Hash

typedef struct {
    Ob lhs;
    Ob rhs;
} ObPair;
static_assert(sizeof(ObPair) == 8, "OpPair has wrong size");

typedef union {
    Ob ob;
    ObPair ob_pair;
    uint8_t uint8s[8];
    uint16_t uint16s[4];
    uint32_t uint32s[2];
    uint64_t uint64s[1];
} Word;
static_assert(sizeof(Word) == 8, "Word has wrong size");

inline uint64_t Word_hash(Word word) { return hash_64(word.uint64s[0]); }

typedef union {
    Word key;
    uint8_t uint8s[16];
    uint16_t uint16s[8];
    uint32_t uint32s[4];
    uint64_t uint64s[2];
} Hash_Node;
static_assert(sizeof(Hash_Node) == 16, "Hash_Node has wrong size");

inline uint64_t Hash_Node_hash(const Hash_Node *node) {
    return hash_64(node->uint64s[0]);
}

typedef struct {
    Hash_Node *nodes;
    size_t mask;
    size_t count;
    size_t size;
} Hash;

static void Hash_validate(const Hash *hash) {
    UN_ASSERT_TRUE(is_power_of_2(hash->size));
    UN_ASSERT_LT(hash->count, hash->size);
    UN_ASSERT_EQ(hash->mask, hash->size - 1UL);
    UN_ASSERT_TRUE(hash->nodes);
}

static Info Hash_init(Hash *hash, size_t size) {
    if (unlikely(!is_power_of_2(size))) {
        fprintf(stderr, "expected size a power of 2, actual %lu", size);
        return 1;
    }
    const size_t bytes = sizeof(Hash_Node) * size;
    Info info = posix_memalign((void **)&(hash->nodes),
                               UN_TUNE_CACHE_LINE_BYTES, bytes);
    if (unlikely(info)) {
        fprintf(stderr, "posix_memalign failed");
        return info;
    }
    bzero(hash->nodes, bytes);
    hash->mask = size - 1UL;
    hash->count = 0;
    hash->size = size;
    if (DEBUG) Hash_validate(hash);
    return Info_success;
}

inline Hash_Node *Hash_insert_nogrow(Hash *hash,
                                     const Hash_Node *node_to_insert);

static Info Hash_grow(Hash *hash) {
    Hash grown;
    Info info = Hash_init(&grown, hash->size * 2UL);
    if (unlikely(info)) return info;
    for (Hash_Node *node = hash->nodes, *end = node + hash->size; node != end;
         ++node) {
        if (node->key.uint64s[0]) {
            Hash_insert_nogrow(&grown, node);
        }
    }
    free(hash->nodes);
    memcpy(hash, &grown, sizeof(Hash));
    return Info_success;
}

inline uint64_t Hash_bucket(const Hash *hash, Word key) {
    return Word_hash(key) & hash->mask & 0xFFFFFFFCUL;
}

// Returns pointer if found, else NULL.
static Hash_Node *Hash_find(const Hash *hash, Word key) {
    uint64_t pos = Hash_bucket(hash, key);
    Hash_Node *node = hash->nodes + pos;
    UN_ASSERT_LT(hash->count, hash->size);  // Required for termination.
    while (key.uint64s[0] != node->uint64s[0]) {
        if (!(node->uint64s[0])) return NULL;
        pos = (pos + 1UL) & hash->mask;
        node = hash->nodes + pos;
    }
    return node;
}

// Returns pointer on success, else NULL.
static Hash_Node *Hash_insert(Hash *hash, const Hash_Node *node) {
    if (unlikely(hash->count * 2UL == hash->size)) {
        Info info = Hash_grow(hash);
        if (unlikely(info)) {
            return NULL;
        }
    }
    return Hash_insert_nogrow(hash, node);
}

// Returns pointer to inserted node. Assumes node is not already inserted.
inline Hash_Node *Hash_insert_nogrow(Hash *hash,
                                     const Hash_Node *node_to_insert) {
    uint64_t pos = Hash_bucket(hash, node_to_insert->key);
    Hash_Node *node = hash->nodes + pos;
    UN_ASSERT_LT(hash->count, hash->size);  // Required for termination.
    while (node->uint64s[0]) {
        UN_ASSERT_NE(node->uint64s[0], node_to_insert->uint64s[0]);
        pos = (pos + 1UL) & hash->mask;
        node = hash->nodes + pos;
    }
    memcpy(node, node_to_insert, sizeof(Hash_Node));
    ++(hash->count);
    return node;
}

// ---------------------------------------------------------------------------
// Inverse Hash
typedef struct {
    // TODO
} InverseHash;

// ---------------------------------------------------------------------------
// Structure
// We need the following structure:
// - app_LRv : Pair Ob -partial-> Ob
// - app_Lrv : Ob -> List (Pair Ob)
// - app_Rlv : Ob -> List (Pair Ob)
// - app_Vlr : Ob -> List (Pair Ob)
// - abs_LRv : Pair Ob -partial-> Ob
// - abs_Lrv : Ob -> List (Pair Ob)
// - abs_Vlr : Ob -> List (Pair Ob)
// - reduced : Ob -partial-> Ob
// - pending : ReprioritizableQueue Ob

typedef struct {
    Hash hash;  // A shared associative array.

    InverseHash app_Lrv;
    InverseHash app_Rlv;
    InverseHash app_Vlr;

    Hash abs_LRv;
} Structure;

Info Structure_init(Structure *structure, size_t size) {
    Info info = Hash_init(&structure->hash, size);
    if (unlikely(info)) return info;
    TODO("init other structure");
}

void Structure_validate(const Structure * structure) {
    Hash_validate(&structure->hash);
}

// A single global instance.
static Structure g_structure;

// ---------------------------------------------------------------------------
// Reduction algorithms

static Ob make_app(Ob lhs, Ob rhs) {
    UN_ASSERT_TRUE(lhs);
    UN_ASSERT_TRUE(rhs);
    TODO("")
}

static Ob simplify_app(Ob lhs, Ob rhs) {
    UN_ASSERT_TRUE(lhs);
    UN_ASSERT_TRUE(rhs);
    Hash_Node node_to_insert;
    node_to_insert.key.uint32s[0] = lhs;
    node_to_insert.key.uint32s[1] = rhs;
    {
        const Hash_Node* node =
            Hash_find(&g_structure.hash, node_to_insert.key);
        if (node) return node - g_structure.hash.nodes;
    }
    TODO("implement");
    const Hash_Node* node = Hash_insert(&g_structure.hash, &node_to_insert);
    if (unlikely(!node)) return 0;
    return node - g_structure.hash.nodes;
}

static Ob compute_app(Ob lhs, Ob rhs, int budget) {
    if (unlikely(!budget)) return simplify_app(lhs, rhs);
    UN_ASSERT_TRUE(lhs);
    UN_ASSERT_TRUE(rhs);
    TODO("implement");
    return make_app(lhs, rhs);
}

// ---------------------------------------------------------------------------
// Interface

// Returns 0 on success.
int un_init(size_t size) {
    Info info = Structure_init(&g_structure, size);
    if (unlikely(info)) return info;
    return Info_success;
}

Ob un_simplify_app(Ob lhs, Ob rhs) {
    if (unlikely(!lhs)) {
        UN_WARN("lhs is null");
        return 0U;
    }
    if (unlikely(!rhs)) {
        UN_WARN("rhs is null");
        return 0U;
    }

    return simplify_app(lhs, rhs);
}

Ob un_compute_app(Ob lhs, Ob rhs, int budget) {
    if (unlikely(!lhs)) {
        UN_WARN("lhs is null");
        return 0U;
    }
    if (unlikely(!rhs)) {
        UN_WARN("rhs is null");
        return 0U;
    }

    return compute_app(lhs, rhs, budget);
}
