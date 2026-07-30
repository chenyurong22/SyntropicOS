/**
 * @file test_structs.c
 * @brief WASM test source for struct layout, pointers, and nested structs.
 */

static int t = 0;

static int fail(void)
{
    return t;
}

typedef struct {
    unsigned char id;
    int value;
    float scale;
} Point;

typedef struct {
    Point origin;
    Point target;
    int active;
} Line;

int run_tests(void)
{
    /* 1. Basic struct allocation and member access */
    t++;
    Point p = {.id = 42, .value = 100, .scale = 2.5f};
    if (p.id != 42 || p.value != 100 || p.scale != 2.5f)
        return fail();

    /* 2. Nested struct assignment */
    t++;
    Line l = {.origin = {.id = 1, .value = 10, .scale = 1.0f},
              .target = {.id = 2, .value = 50, .scale = 5.0f},
              .active = 1};
    if (l.origin.id != 1 || l.target.value != 50 || l.active != 1)
        return fail();

    /* 3. Struct pointer mutation */
    t++;
    Point *pp = &l.target;
    pp->value += 20;
    if (l.target.value != 70)
        return fail();

    return 0;
}
