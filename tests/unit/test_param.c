/**
 * @file test_param.c
 * @brief Unity tests for syn_param.
 */

#include "mocks/mock_port.h"
#include "syntropic/storage/syn_param.h"
#include "syntropic/syntropic.h"
#include "unity/unity.h"

typedef struct {
    uint16_t brightness;
    int16_t offset;
    uint8_t mode;
    uint8_t _pad;
} TestParams;

static void test_param_store(void)
{
    /* Erase flash */
    memset(mock_flash, 0xFF, sizeof(mock_flash));

    SYN_ParamStore store;
    /* 4 sectors of 1024 bytes each */
    SYN_Status st = syn_param_init(&store, 0, 4, sizeof(TestParams));
    TEST_ASSERT_EQUAL(SYN_ERROR, st);

    /* Save defaults */
    TestParams params = {.brightness = 80, .offset = -10, .mode = 3};
    st = syn_param_save(&store, &params);
    TEST_ASSERT_EQUAL(SYN_OK, st);

    /* Load back */
    TestParams loaded;
    memset(&loaded, 0, sizeof(loaded));
    st = syn_param_load(&store, &loaded);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_EQUAL_INT(80, loaded.brightness);
    TEST_ASSERT_EQUAL_INT(-10, loaded.offset);
    TEST_ASSERT_EQUAL_INT(3, loaded.mode);

    /* Save again (goes to next slot) */
    params.brightness = 90;
    st = syn_param_save(&store, &params);
    TEST_ASSERT_EQUAL(SYN_OK, st);

    /* Re-init and load (should find latest) */
    SYN_ParamStore store2;
    st = syn_param_init(&store2, 0, 4, sizeof(TestParams));
    TEST_ASSERT_EQUAL(SYN_OK, st);

    memset(&loaded, 0, sizeof(loaded));
    st = syn_param_load(&store2, &loaded);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_EQUAL_INT(90, loaded.brightness);

    /* Wear leveling: save many times and verify it wraps sectors */
    int i;
    for (i = 0; i < 200; i++) {
        params.brightness = (uint16_t)(i & 0xFFFF);
        st = syn_param_save(&store, &params);
        TEST_ASSERT_EQUAL(SYN_OK, st);
    }

    memset(&loaded, 0, sizeof(loaded));
    st = syn_param_load(&store, &loaded);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_EQUAL_INT(199, loaded.brightness);

    /* Erase all */
    st = syn_param_erase_all(&store);
    TEST_ASSERT_EQUAL(SYN_OK, st);

    st = syn_param_load(&store, &loaded);
    TEST_ASSERT_EQUAL(SYN_ERROR, st);

    /* After erase_all, next_seq resets to 0 → write_count returns 0 */
    uint16_t wc = syn_param_write_count(&store);
    TEST_ASSERT_EQUAL_UINT16(0, wc);
}

/** data_size exceeds sector capacity — exercises line 132 (slots_per_sector == 0) */
static void test_param_data_too_large(void)
{
    mock_port_reset();
    SYN_ParamStore store;
    /* MOCK_FLASH_SECTOR=1024; set data_size >> sector so slot doesn't fit */
    SYN_Status st = syn_param_init(&store, 0, 1, 1024u);
    /* Should return SYN_ERROR (data too large for sector) */
    TEST_ASSERT_EQUAL(SYN_ERROR, st);
}

static void test_param_sector_wrap(void)
{
    mock_port_reset();
    SYN_ParamStore store;
    TestParams p = {.brightness = 100, .offset = 20, .mode = 1};

    /* 2 sectors of 1024 bytes */
    SYN_Status st = syn_param_init(&store, 0, 2, sizeof(TestParams));
    TEST_ASSERT_EQUAL(SYN_ERROR, st); /* blank flash */

    /* Perform enough saves to fill sector 0 and wrap to sector 1 */
    uint16_t slots_per_sec = store.slots_per_sector;
    for (uint16_t i = 0; i <= slots_per_sec; i++) {
        p.brightness = (uint16_t)i;
        st = syn_param_save(&store, &p);
        TEST_ASSERT_EQUAL(SYN_OK, st);
    }

    TestParams loaded = {0};
    st = syn_param_load(&store, &loaded);
    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_EQUAL_UINT16(slots_per_sec, loaded.brightness);
}

static void test_param_load_crc_corrupted_slot(void)
{
    mock_port_reset();
    SYN_ParamStore store;
    TestParams p = {.brightness = 100, .offset = 20, .mode = 1};

    syn_param_init(&store, 0, 2, sizeof(TestParams));
    syn_param_save(&store, &p);

    /* Corrupt payload CRC in flash */
    mock_flash[sizeof(SYN_ParamSlotHeader)] ^= 0xFF;

    TestParams loaded = {0};
    TEST_ASSERT_EQUAL(SYN_ERROR, syn_param_load(&store, &loaded));
}

static void test_param_corrupt_header_data_size(void)
{
    mock_port_reset();
    SYN_ParamStore store;
    TestParams p = {.brightness = 100, .offset = 20, .mode = 1};

    syn_param_init(&store, 0, 2, sizeof(p));
    syn_param_save(&store, &p);

    /* Corrupt slot header data_size in flash */
    SYN_ParamSlotHeader *hdr = (SYN_ParamSlotHeader *)mock_flash;
    hdr->data_size = (uint16_t)(sizeof(p) + 5);

    TestParams loaded;
    TEST_ASSERT_EQUAL(SYN_ERROR, syn_param_load(&store, &loaded));
}

static void test_param_flash_erase_error_returns_failure(void)
{
    mock_port_reset();
    SYN_ParamStore store;
    TestParams p = {.brightness = 100, .offset = 20, .mode = 1};
    syn_param_init(&store, 0, 2, sizeof(p));
    mock_flash_fail_at = 0;
    TEST_ASSERT_EQUAL(SYN_ERROR, syn_param_erase_all(&store));
    mock_flash_fail_at = 0;
    TEST_ASSERT_EQUAL(SYN_ERROR, syn_param_save(&store, &p));
    mock_flash_fail_at = -1;
}

void run_param_tests(void)
{
    RUN_TEST(test_param_store);
    RUN_TEST(test_param_data_too_large);
    RUN_TEST(test_param_sector_wrap);
    RUN_TEST(test_param_load_crc_corrupted_slot);
    RUN_TEST(test_param_corrupt_header_data_size);
    RUN_TEST(test_param_flash_erase_error_returns_failure);
}
