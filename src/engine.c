#include "engine.h"

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// static_assert should be defined in assert.h.
#if !defined(static_assert)
// #warning "ignoring static_assert(-,-)"
#define static_assert(cond, message) struct UN_SWALLOW_SEMICOLON
#endif  // !defined(static_assert)

#if defined(__GNUC__) || defined(__clang__)
#define likely(x) __builtin_expect(!!(x), true)
#define unlikely(x) __builtin_expect(!!(x), false)
#else  // defined(__GNUC__) || defined(__clang__)
// #warning "ignoring likely(-), unlikely(-)"
#define likely(x) (x)
#define unlikely(x) (x)
#endif  // defined(__GNUC__) || defined(__clang__)

#ifdef NDEBUG
#define DEBUG 0
#else  // NDEBUG
#define DEBUG 1
#endif  // NDEBUG

#define UN_UNUSED(x) ((void)(x))

#define UN_ERROR(...)                          \
    {                                          \
        fprintf(stderr, "ERROR " __VA_ARGS__); \
        fflush(stderr);                        \
        abort();                               \
    }
#define UN_WARN(...)                             \
    {                                            \
        fprintf(stderr, "WARNING " __VA_ARGS__); \
        fflush(stderr);                          \
    }

#define TODO(...) UN_ERROR("TODO " __VA_ARGS__)

#define UN_CHECK(cond, ...)                          \
    {                                                \
        if (unlikely(!(cond))) UN_ERROR(__VA_ARGS__) \
    }

#ifdef NDEBUG
#define UN_DCHECK(...)
#else  // NDEBUG
#define UN_DCHECK UN_CHECK
#endif  // NDEBUG

#define UN_CHECK_TRUE(cond) UN_CHECK((cond), "failed assertion: " #cond)
#define UN_CHECK_OP(x, op, y, fmt)                                            \
    UN_CHECK((x)op(y),                                                        \
             "expected " #x " " #op " " #y "; actual %" fmt " vs %" fmt, (x), \
             (y))
#define UN_CHECK_NE(x, y, fmt) UN_CHECK_OP(x, !=, y, fmt)
#define UN_CHECK_EQ(x, y, fmt) UN_CHECK_OP(x, ==, y, fmt)
#define UN_CHECK_LT(x, y, fmt) UN_CHECK_OP(x, <, y, fmt)
#define UN_CHECK_LE(x, y, fmt) UN_CHECK_OP(x, <=, y, fmt)

#define UN_DCHECK_TRUE(cond) UN_DCHECK((cond), "failed assertion: " #cond)
#define UN_DCHECK_OP(x, op, y, fmt)                                            \
    UN_DCHECK((x)op(y),                                                        \
              "expected " #x " " #op " " #y "; actual %" fmt " vs %" fmt, (x), \
              (y))
#define UN_DCHECK_NE(x, y, fmt) UN_DCHECK_OP(x, !=, y, fmt)
#define UN_DCHECK_EQ(x, y, fmt) UN_DCHECK_OP(x, ==, y, fmt)
#define UN_DCHECK_LT(x, y, fmt) UN_DCHECK_OP(x, <, y, fmt)
#define UN_DCHECK_LE(x, y, fmt) UN_DCHECK_OP(x, <=, y, fmt)

#define UN_CACHE_LINE_BYTES (64UL)
#define UN_INIT_CAPACITY (1024U)

static inline void *malloc_or_die(size_t size) {
    void *ptr = malloc(size);
    UN_CHECK(ptr, "out of memory, size = %lu", size);
    return ptr;
}

static inline void *realloc_or_die(void *ptr, size_t size) {
    ptr = realloc(ptr, size);
    UN_CHECK(ptr, "out of memory, size = %lu", size);
    return ptr;
}

static inline void *memalign_or_die(size_t alignment, size_t size) {
    void *ptr;
    int info = posix_memalign(&ptr, alignment, size);
    UN_CHECK(info == 0, "out of memory, size = %lu", size);
    return ptr;
}

// Adapted from
// https://github.com/google/farmhash/blob/master/src/farmhash.h#L167
static inline uint64_t hash_64(uint64_t key) {
    // Murmur-inspired hashing.
    const uint64_t kMul = 0x9ddfea08eb382d69ULL;
    uint64_t b = key * kMul;
    b ^= (b >> 44);
    b *= kMul;
    b ^= (b >> 41);
    b *= kMul;
    return b;
}

static inline bool is_power_of_2(uint64_t x) {
    return ((x != 0UL) && !(x & (x - 1UL)));
}

// ---------------------------------------------------------------------------
// Signature

#define UN_TOP 1U
#define UN_BOT 2U
#define UN_I 3U
#define UN_K 4U
#define UN_B 5U
#define UN_C 6U
#define UN_S 7U
#define UN_VARS_BEGIN 8U
#define UN_VARS_END (UN_VARS_BEGIN + 32U)

static inline bool is_var(Ob ob) {
    return UN_VARS_BEGIN <= ob && ob < UN_VARS_END;
}

// ---------------------------------------------------------------------------
// AbsList

typedef struct {
    Ob key;
    Ob val;
} AbsList_Node;

// zero is valid and initialized.
typedef struct {
    AbsList_Node *nodes;
    size_t size;
} AbsList;

static inline void AbsList_init(AbsList *list, size_t size) {
    UN_DCHECK_TRUE(list->nodes == NULL);
    UN_DCHECK_TRUE(list->size == 0UL);
    list->nodes = malloc_or_die(size * sizeof(AbsList_Node));
    list->size = size;
}

static inline void AbsList_clear(AbsList *list) {
    free(list->nodes);
    bzero(list, sizeof(AbsList));
}

// ---------------------------------------------------------------------------
// Carrier

typedef struct {
    Ob obs[2];  // Either {next} if free or {lhs, rhs} if allocated.
    AbsList abs;
} Carrier_Node;

typedef struct {
    Carrier_Node *nodes;
    Ob free_range;
    Ob free_list;
    Ob capacity;
} Carrier;

static void Carrier_init(Carrier *carrier, size_t capacity) {
    carrier->free_range = 1U;
    carrier->free_list = 0U;
    carrier->capacity = capacity - 1;  // Position 0 is disallowed.
    carrier->nodes = malloc_or_die(capacity * sizeof(Ob));
}

static Ob Carrier_alloc(Carrier *carrier) {
    // Check for recycled obs.
    {
        Ob ob = carrier->free_list;
        if (ob) {
            carrier->free_list = carrier->nodes[ob].obs[0];
            carrier->nodes[ob].obs[0] = 0;
            return ob;
        }
    }
    // Maybe allocate more space.
    if (unlikely(carrier->free_range == carrier->capacity)) {
        carrier->capacity *= 2UL;
        carrier->nodes = realloc_or_die(carrier->nodes, carrier->capacity);
        bzero(carrier->nodes + carrier->free_range,
              carrier->free_range * sizeof(Ob));
    }
    return carrier->free_range++;
}

static void Carrier_free(Carrier *carrier, Ob ob) {
    UN_DCHECK_TRUE(0U < ob);
    UN_DCHECK_TRUE(ob < carrier->free_range);
    Carrier_Node *node = carrier->nodes + ob;
    AbsList_clear(&node->abs);
    bzero(node, sizeof(Carrier_Node));
    node->obs[0] = carrier->free_list;
    carrier->free_list = ob;
}

static void Carrier_test(unsigned int seed) {
    srand(seed);
    for (int init_capacity = 1; init_capacity < 10; ++init_capacity) {
        Carrier carrier;
        Carrier_init(&carrier, init_capacity);
        Ob ob;
        ob = Carrier_alloc(&carrier);
        UN_CHECK_EQ(ob, 1U, "u");

        Carrier_free(&carrier, 1);
        ob = Carrier_alloc(&carrier);
        UN_CHECK_EQ(ob, 1U, "u");
        ob = Carrier_alloc(&carrier);
        UN_CHECK_EQ(ob, 2U, "u");

        Carrier_free(&carrier, 1);
        ob = Carrier_alloc(&carrier);
        UN_CHECK_EQ(ob, 1U, "u");
        ob = Carrier_alloc(&carrier);
        UN_CHECK_EQ(ob, 3U, "u");
        ob = Carrier_alloc(&carrier);
        UN_CHECK_EQ(ob, 4U, "u");

        Carrier_free(&carrier, 3);
        Carrier_free(&carrier, 1);
        Carrier_free(&carrier, 2);
        ob = Carrier_alloc(&carrier);
        UN_CHECK_EQ(ob, 2U, "u");
        Carrier_free(&carrier, 2);
        ob = Carrier_alloc(&carrier);
        UN_CHECK_EQ(ob, 2U, "u");
        Carrier_free(&carrier, 1);
        ob = Carrier_alloc(&carrier);
        UN_CHECK_EQ(ob, 1U, "u");
        Carrier_free(&carrier, 3);
        ob = Carrier_alloc(&carrier);
        UN_CHECK_EQ(ob, 3U, "u");
        Carrier_free(&carrier, 5);
    }
}

// ---------------------------------------------------------------------------
// Hash

typedef struct {
    Ob lhs;
    Ob rhs;
} ObPair;
static_assert(sizeof(Ob) == 4, "Ob has wrong size");
static_assert(sizeof(ObPair) == 8, "ObPair has wrong size");

typedef union {
    Ob ob;
    ObPair ob_pair;
    uint8_t uint8s[8];
    uint16_t uint16s[4];
    uint32_t uint32s[2];
    uint64_t uint64s[1];
} Word;
static_assert(sizeof(Word) == 8, "Word has wrong size");

static inline uint64_t Word_hash(Word word) { return hash_64(word.uint64s[0]); }

typedef union {
    Word key;
    uint8_t uint8s[16];
    uint16_t uint16s[8];
    uint32_t uint32s[4];
    uint64_t uint64s[2];
} Hash_Node;
static_assert(sizeof(Hash_Node) == 16, "Hash_Node has wrong size");

typedef struct {
    Hash_Node *nodes;
    size_t mask;
    size_t count;
    size_t size;
} Hash;

static void Hash_validate(const Hash *hash) {
    UN_CHECK_TRUE(is_power_of_2(hash->size));
    UN_CHECK_LT(hash->count, hash->size, "lu")
    UN_CHECK_EQ(hash->mask, hash->size - 1UL, "lu")
    UN_CHECK_TRUE(hash->nodes);
}

static void Hash_init(Hash *hash, size_t size) {
    UN_CHECK(is_power_of_2(size), "expected size a power of 2, actual %zu",
             size);
    const size_t bytes = sizeof(Hash_Node) * size;
    hash->nodes = memalign_or_die(UN_CACHE_LINE_BYTES, bytes);
    bzero(hash->nodes, bytes);
    hash->mask = size - 1UL;
    hash->count = 0;
    hash->size = size;
    if (DEBUG) Hash_validate(hash);
}

static inline Hash_Node *Hash_insert_nogrow(Hash *hash,
                                            const Hash_Node *node_to_insert);

static void Hash_grow(Hash *hash) {
    Hash grown;
    Hash_init(&grown, hash->size * 2UL);
    for (Hash_Node *node = hash->nodes, *end = node + hash->size; node != end;
         ++node) {
        if (node->key.uint64s[0]) {
            Hash_insert_nogrow(&grown, node);
        }
    }
    free(hash->nodes);
    memcpy(hash, &grown, sizeof(Hash));
}

static inline uint64_t Hash_bucket(const Hash *hash, Word key) {
    return Word_hash(key) & hash->mask & 0xFFFFFFFCUL;
}

// Returns pointer if found, else NULL.
static Hash_Node *Hash_find(const Hash *hash, Word key) {
    uint64_t pos = Hash_bucket(hash, key);
    Hash_Node *node = hash->nodes + pos;
    UN_DCHECK_LT(hash->count, hash->size, "zu")  // Required for termination.
    while (key.uint64s[0] != node->uint64s[0]) {
        if (!(node->uint64s[0])) return NULL;
        pos = (pos + 1UL) & hash->mask;
        node = hash->nodes + pos;
    }
    return node;
}

static Hash_Node *Hash_insert(Hash *hash, const Hash_Node *node) {
    if (unlikely(hash->count * 2UL == hash->size)) {
        Hash_grow(hash);
    }
    return Hash_insert_nogrow(hash, node);
}

// Returns pointer to inserted node. Assumes node is not already inserted.
static inline Hash_Node *Hash_insert_nogrow(Hash *hash,
                                            const Hash_Node *node_to_insert) {
    uint64_t pos = Hash_bucket(hash, node_to_insert->key);
    Hash_Node *node = hash->nodes + pos;
    UN_DCHECK_LT(hash->count, hash->size, "zu")  // Required for termination.
    while (node->uint64s[0]) {
        UN_DCHECK_NE(node->uint64s[0], node_to_insert->uint64s[0], PRIu64)
        pos = (pos + 1UL) & hash->mask;
        node = hash->nodes + pos;
    }
    memcpy(node, node_to_insert, sizeof(Hash_Node));
    ++(hash->count);
    return node;
}

// ---------------------------------------------------------------------------
// Inverse Hash

// TODO
typedef union {
    Word key;
    uint8_t uint8s[16];
    uint16_t uint16s[8];
    uint32_t uint32s[4];
    uint64_t uint64s[2];
} InverseHash_Node;
static_assert(sizeof(InverseHash_Node) == 16, "Hash_Node has wrong size");

// TODO
typedef struct {
    InverseHash_Node *nodes;
    size_t mask;
    size_t count;
    size_t size;
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
    Carrier carrier;

    Hash hash;  // A shared associative array.

    InverseHash app_Lrv;
    InverseHash app_Rlv;
    InverseHash app_Vlr;

    Hash abs_LRv;
} Structure;

void Structure_init(Structure *structure) {
    Carrier_init(&structure->carrier, UN_INIT_CAPACITY);
    Hash_init(&structure->hash, UN_INIT_CAPACITY);

    // Init constants.
    Ob ob;
    ob = Carrier_alloc(&structure->carrier);
    UN_CHECK_EQ(ob, UN_TOP, "u");
    ob = Carrier_alloc(&structure->carrier);
    UN_CHECK_EQ(ob, UN_BOT, "u");
    ob = Carrier_alloc(&structure->carrier);
    UN_CHECK_EQ(ob, UN_I, "u");
    ob = Carrier_alloc(&structure->carrier);
    UN_CHECK_EQ(ob, UN_K, "u");
    ob = Carrier_alloc(&structure->carrier);
    UN_CHECK_EQ(ob, UN_B, "u");
    ob = Carrier_alloc(&structure->carrier);
    UN_CHECK_EQ(ob, UN_C, "u");
    ob = Carrier_alloc(&structure->carrier);
    UN_CHECK_EQ(ob, UN_S, "u");

    // Init variables.
    for (Ob var = UN_VARS_BEGIN; var != UN_VARS_END; ++var) {
        ob = Carrier_alloc(&structure->carrier);
        UN_CHECK_EQ(ob, var, "u");
        Carrier_Node *node = structure->carrier.nodes + ob;

        // Set \x.x = I.
        AbsList_init(&node->abs, 1);
        node->abs.nodes[0].key = var;
        node->abs.nodes[0].val = UN_I;
    }

    TODO("init other structure");
}

void Structure_validate(const Structure *structure) {
    Hash_validate(&structure->hash);
}

// A single global instance.
static Structure g_structure;

// ---------------------------------------------------------------------------
// ObStack

typedef struct {
    Ob *data;
    uint32_t size;
    uint32_t capacity;
} ObStack;

static inline void ObStack_init(ObStack *stack) {
    size_t bytes = UN_CACHE_LINE_BYTES;
    stack->data = malloc_or_die(bytes);
    stack->size = 0;
    stack->capacity = bytes / sizeof(Ob);
}

static inline void ObStack_delete(ObStack *stack) { free(stack->data); }

static inline void ObStack_push(ObStack *stack, Ob ob) {
    if (unlikely(stack->size == stack->capacity)) {
        stack->capacity *= 2UL;
        UN_CHECK(stack->capacity, "stack is too large");
        stack->data = realloc_or_die(stack->data, stack->capacity);
    }
    stack->data[stack->size++] = ob;
}

static inline Ob ObStack_try_pop(ObStack *stack) {
    return stack->size ? stack->data[--stack->size] : 0U;
}

// ---------------------------------------------------------------------------
// Reduction algorithms

static inline Hash_Node *find_app(Ob lhs, Ob rhs) {
    Word key = {.uint32s = {lhs, rhs}};
    return Hash_find(&g_structure.hash, key);
}

static Ob make_app(Ob lhs, Ob rhs) {
    UN_DCHECK_TRUE(lhs);
    UN_DCHECK_TRUE(rhs);
    UN_UNUSED(lhs);
    UN_UNUSED(rhs);
    TODO("")
}

static Ob simplify(Ob ob);

static Ob simplify_app(Ob lhs, Ob rhs) {
    UN_DCHECK_TRUE(lhs);
    UN_DCHECK_TRUE(rhs);

    // First check cache.
    {
        const Hash_Node *node = find_app(lhs, rhs);
        if (node) return node - g_structure.hash.nodes;
    }

    // Linearly beta-eta reduce.
    Ob head = lhs;
    {
        ObStack args;
        ObStack_init(&args);
        ObStack_push(&args, rhs);
        while (!is_var(head)) {
            const Carrier_Node *head_node = g_structure.carrier.nodes + head;
            while (head_node->obs[0] && head_node->obs[1]) {
                head = head_node->obs[0];
                ObStack_push(&args, head_node->obs[1]);
                head_node = g_structure.carrier.nodes + head;
            }
            switch (head) {
                case UN_I:
                    TODO("reduce");
                    break;
                default:
                    UN_ERROR("uidentified ob: %u", head);
            }
        }

        // Simpilfy args.
        Ob arg;
        while ((arg = ObStack_try_pop(&args))) {
            arg = simplify(arg);
            head = make_app(head, arg);
        }
        ObStack_delete(&args);
    }

    // Abstract variables.
    TODO("implement");

    // Save value.
    Hash_Node node_to_insert = {.uint32s = {lhs, rhs}};
    const Hash_Node *node = Hash_insert(&g_structure.hash, &node_to_insert);
    if (unlikely(!node)) return 0;
    return node - g_structure.hash.nodes;
}

static Ob compute_app(Ob lhs, Ob rhs, int *budget) {
    if (unlikely(!*budget)) return simplify_app(lhs, rhs);
    UN_DCHECK_TRUE(lhs);
    UN_DCHECK_TRUE(rhs);
    TODO("implement");
    return make_app(lhs, rhs);
}

static Ob simplify(Ob ob) {
    const Ob lhs = g_structure.carrier.nodes[ob].obs[0];
    const Ob rhs = g_structure.carrier.nodes[ob].obs[0];
    return (lhs && rhs) ? simplify_app(lhs, rhs) : ob;
}

static Ob compute(Ob ob, int *budget) {
    const Ob lhs = g_structure.carrier.nodes[ob].obs[0];
    const Ob rhs = g_structure.carrier.nodes[ob].obs[0];
    return (lhs && rhs) ? compute_app(lhs, rhs, budget) : ob;
}

// -----------------------------------------------------------------------
// Interface

// Returns 0 on success.
void un_init() {
    static bool initialized = false;
    if (unlikely(!initialized)) {
        Structure_init(&g_structure);
        initialized = true;
    }
}

Ob un_simplify(Ob ob) {
    UN_CHECK(ob, "ob is null");
    return simplify(ob);
}

Ob un_simplify_app(Ob lhs, Ob rhs) {
    UN_CHECK(lhs, "lhs is null");
    UN_CHECK(rhs, "rhs is null");
    return simplify_app(lhs, rhs);
}

Ob un_compute(Ob ob, int *budget) {
    UN_CHECK(ob, "ob is null");
    UN_CHECK(budget, "budget is null");
    return compute(ob, budget);
}

Ob un_compute_app(Ob lhs, Ob rhs, int *budget) {
    UN_CHECK(lhs, "lhs is null");
    UN_CHECK(rhs, "rhs is null");
    UN_CHECK(budget, "budget is null");
    return compute_app(lhs, rhs, budget);
}

void un_test(unsigned int seed) { Carrier_test(seed); }
