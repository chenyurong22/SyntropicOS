/**
 * @file test_slab.c
 * @brief Unit test suite for Multi-Class Slab Allocator (syn_slab).
 */

#include "syntropic/util/syn_slab.h"
#include "unity/unity.h"

void test_slab_alloc_free_stats(void)
{
    uint8_t backing[1024];
    SYN_SlabAllocator slab;

    size_t sizes[] = {16, 64, 256};
    size_t counts[] = {8, 4, 2};

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_slab_init(&slab, backing, sizeof(backing), sizes, counts, 3));

    void *p16_a = syn_slab_alloc(&slab, 10);
    void *p16_b = syn_slab_alloc(&slab, 16);
    void *p64 = syn_slab_alloc(&slab, 50);
    void *p256 = syn_slab_alloc(&slab, 200);

    TEST_ASSERT_NOT_NULL(p16_a);
    TEST_ASSERT_NOT_NULL(p16_b);
    TEST_ASSERT_NOT_NULL(p64);
    TEST_ASSERT_NOT_NULL(p256);

    SYN_SlabStats stats;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_slab_get_stats(&slab, &stats));
    TEST_ASSERT_EQUAL_UINT32(4, stats.total_allocs);

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_slab_free(&slab, p16_a));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_slab_free(&slab, p16_b));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_slab_free(&slab, p64));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_slab_free(&slab, p256));

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_slab_get_stats(&slab, &stats));
    TEST_ASSERT_EQUAL_UINT32(0, stats.total_allocs);

    /* Test invalid free pointer */
    uint8_t dummy = 0;
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_slab_free(&slab, &dummy));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_slab_free(&slab, NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM,
                          syn_slab_init(NULL, backing, sizeof(backing), sizes, counts, 3));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_slab_get_stats(NULL, &stats));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_slab_get_stats(&slab, NULL));
}

void test_slab_edge_cases_and_exhaustion(void)
{
    uint8_t backing[256];
    SYN_SlabAllocator slab;

    size_t sizes[] = {16};
    size_t counts[] = {2};

    /* Insufficient backing memory */
    size_t big_counts[] = {100};
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM,
                          syn_slab_init(&slab, backing, sizeof(backing), sizes, big_counts, 1));

    /* Exceed max classes */
    size_t many_sizes[10] = {16, 32, 48, 64, 80, 96, 112, 128, 144, 160};
    size_t many_counts[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_slab_init(&slab, backing, sizeof(backing),
                                                           many_sizes, many_counts, 10));

    /* Valid init with 2 blocks of 16B */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_slab_init(&slab, backing, sizeof(backing), sizes, counts, 1));

    /* Alloc zero size */
    TEST_ASSERT_NULL(syn_slab_alloc(&slab, 0));
    TEST_ASSERT_NULL(syn_slab_alloc(NULL, 10));

    /* Alloc larger than max class */
    TEST_ASSERT_NULL(syn_slab_alloc(&slab, 500));

    /* Exhaust class capacity */
    void *b1 = syn_slab_alloc(&slab, 16);
    void *b2 = syn_slab_alloc(&slab, 16);
    TEST_ASSERT_NOT_NULL(b1);
    TEST_ASSERT_NOT_NULL(b2);

    /* 3rd allocation fails */
    TEST_ASSERT_NULL(syn_slab_alloc(&slab, 16));

    /* Misaligned pointer free */
    uint8_t *misaligned = ((uint8_t *)b1) + 1;
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_slab_free(&slab, misaligned));

    /* Free b1 and re-allocate */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_slab_free(&slab, b1));
    void *b3 = syn_slab_alloc(&slab, 16);
    TEST_ASSERT_NOT_NULL(b3);

    syn_slab_free(&slab, b2);
    syn_slab_free(&slab, b3);
}

static void test_slab_null_params(void)
{
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_slab_init(NULL, NULL, 0, NULL, NULL, 0));
    TEST_ASSERT_NULL(syn_slab_alloc(NULL, 16));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_slab_free(NULL, NULL));
}

void run_slab_tests(void)
{
    RUN_TEST(test_slab_alloc_free_stats);
    RUN_TEST(test_slab_edge_cases_and_exhaustion);
    RUN_TEST(test_slab_null_params);
}
