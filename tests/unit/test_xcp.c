/**
 * @file test_xcp.c
 * @brief Unit tests for ASAM XCP slave protocol implementation.
 */

#include "syntropic/proto/syn_xcp.h"
#include "unity/unity.h"

#include <string.h>

static SYN_XCP_Slave g_xcp_slave;

static void test_xcp_init_and_connect_disconnect(void)
{
    TEST_ASSERT_FALSE(syn_xcp_init(NULL, 0x1234U));
    TEST_ASSERT_TRUE(syn_xcp_init(&g_xcp_slave, 0x1234U));
    TEST_ASSERT_EQUAL_HEX16(0x1234U, g_xcp_slave.station_id);

    uint8_t cto[8] = {0};
    uint8_t dto[8] = {0};

    /* Command before CONNECT -> ERR_NOT_CONNECTED */
    cto[0] = SYN_XCP_CMD_GET_STATUS;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_ERR, dto[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_ERR_NOT_CONNECTED, dto[1]);

    /* CONNECT */
    cto[0] = SYN_XCP_CMD_CONNECT;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_RES, dto[0]);
    TEST_ASSERT_TRUE(g_xcp_slave.connected);

    /* GET_STATUS */
    cto[0] = SYN_XCP_CMD_GET_STATUS;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_RES, dto[0]);
    TEST_ASSERT_EQUAL_HEX8(0xFF, dto[2]); /* Unlocked resources */

    /* GET_COMM_MODE_INFO */
    cto[0] = SYN_XCP_CMD_GET_COMM_MODE_INFO;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_RES, dto[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01, dto[2]);

    /* GET_ID */
    cto[0] = SYN_XCP_CMD_GET_ID;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_RES, dto[0]);

    /* GET_SEED & UNLOCK */
    cto[0] = SYN_XCP_CMD_GET_SEED;
    cto[1] = 0x00;
    cto[2] = SYN_XCP_RESOURCE_CAL_PAG;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_RES, dto[0]);
    TEST_ASSERT_EQUAL_HEX8(0x04, dto[1]);

    /* UNLOCK syntax error vs success */
    cto[0] = SYN_XCP_CMD_UNLOCK;
    cto[1] = 0x00;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_ERR, dto[0]);

    cto[1] = 0x04;
    cto[2] = 0x12;
    cto[3] = 0x34;
    cto[4] = 0x56;
    cto[5] = 0x78;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_RES, dto[0]);

    /* DISCONNECT */
    cto[0] = SYN_XCP_CMD_DISCONNECT;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_FALSE(g_xcp_slave.connected);
}

static void test_xcp_mta_upload_download(void)
{
    syn_xcp_init(&g_xcp_slave, 0x1234U);
    uint8_t cto[8] = {0};
    uint8_t dto[8] = {0};
    uint8_t buffer[16] = {0};

    cto[0] = SYN_XCP_CMD_CONNECT;
    syn_xcp_process_cto(&g_xcp_slave, cto, dto);

    /* SET_MTA with 0 addr */
    TEST_ASSERT_TRUE(syn_xcp_set_mta(&g_xcp_slave, 0x01, (uintptr_t)buffer));
    cto[0] = SYN_XCP_CMD_SET_MTA;
    cto[3] = 0x01;
    cto[4] = 0;
    cto[5] = 0;
    cto[6] = 0;
    cto[7] = 0;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_RES, dto[0]);

    /* SET_MTA with non-zero dummy addr */
    cto[4] = 0x10;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_RES, dto[0]);

    /* Restore MTA pointer */
    syn_xcp_set_mta(&g_xcp_slave, 0x01, (uintptr_t)buffer);

    /* DOWNLOAD 4 bytes */
    cto[0] = SYN_XCP_CMD_DOWNLOAD;
    cto[1] = 0x04;
    cto[2] = 0xDE;
    cto[3] = 0xAD;
    cto[4] = 0xBE;
    cto[5] = 0xEF;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_RES, dto[0]);
    TEST_ASSERT_EQUAL_HEX8(0xDE, buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(0xAD, buffer[1]);
    TEST_ASSERT_EQUAL_HEX8(0xBE, buffer[2]);
    TEST_ASSERT_EQUAL_HEX8(0xEF, buffer[3]);

    /* DOWNLOAD_MAX 7 bytes */
    cto[0] = SYN_XCP_CMD_DOWNLOAD_MAX;
    cto[1] = 0x11;
    cto[2] = 0x22;
    cto[3] = 0x33;
    cto[4] = 0x44;
    cto[5] = 0x55;
    cto[6] = 0x66;
    cto[7] = 0x77;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_RES, dto[0]);
    TEST_ASSERT_EQUAL_HEX8(0x11, buffer[4]);
    TEST_ASSERT_EQUAL_HEX8(0x77, buffer[10]);

    /* UPLOAD 4 bytes */
    syn_xcp_set_mta(&g_xcp_slave, 0x01, (uintptr_t)buffer);
    cto[0] = SYN_XCP_CMD_UPLOAD;
    cto[1] = 0x04;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_RES, dto[0]);
    TEST_ASSERT_EQUAL_HEX8(0xDE, dto[1]);
    TEST_ASSERT_EQUAL_HEX8(0xAD, dto[2]);

    /* SHORT_UPLOAD */
    syn_xcp_set_mta(&g_xcp_slave, 0x01, (uintptr_t)(buffer + 4));
    cto[0] = SYN_XCP_CMD_SHORT_UPLOAD;
    cto[1] = 0x02;
    cto[3] = 0x01;
    cto[4] = 0;
    cto[5] = 0;
    cto[6] = 0;
    cto[7] = 0;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_RES, dto[0]);
    TEST_ASSERT_EQUAL_HEX8(0x11, dto[1]);
    TEST_ASSERT_EQUAL_HEX8(0x22, dto[2]);

    /* Out of range UPLOAD / DOWNLOAD */
    cto[0] = SYN_XCP_CMD_UPLOAD;
    cto[1] = 0x08;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_ERR, dto[0]);

    cto[0] = SYN_XCP_CMD_DOWNLOAD;
    cto[1] = 0x08;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_ERR, dto[0]);

    cto[0] = SYN_XCP_CMD_SHORT_UPLOAD;
    cto[1] = 0x08;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_ERR, dto[0]);
}

static void test_xcp_daq_list_streaming(void)
{
    syn_xcp_init(&g_xcp_slave, 0x1234U);
    uint8_t cto[8] = {0};
    uint8_t dto[8] = {0};
    uint16_t var1 = 0x1234U;
    uint32_t var2 = 0xABCDEF01U;

    cto[0] = SYN_XCP_CMD_CONNECT;
    syn_xcp_process_cto(&g_xcp_slave, cto, dto);

    /* SET_DAQ_PTR List 0, ODT 0, Element 0 */
    cto[0] = SYN_XCP_CMD_SET_DAQ_PTR;
    cto[2] = 0x00;
    cto[3] = 0x00;
    cto[4] = 0x00;
    cto[5] = 0x00;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_RES, dto[0]);

    /* WRITE_DAQ element 0 (var1, 2 bytes) */
    cto[0] = SYN_XCP_CMD_WRITE_DAQ;
    cto[2] = 0x02;
    cto[3] = 0x01;
    syn_xcp_set_mta(&g_xcp_slave, 0x01, (uintptr_t)&var1);
    cto[4] = 0;
    cto[5] = 0;
    cto[6] = 0;
    cto[7] = 0;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_RES, dto[0]);

    /* WRITE_DAQ element 1 (var2, 4 bytes) */
    cto[0] = SYN_XCP_CMD_WRITE_DAQ;
    cto[2] = 0x04;
    cto[3] = 0x01;
    syn_xcp_set_mta(&g_xcp_slave, 0x01, (uintptr_t)&var2);
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_RES, dto[0]);

    /* SET_DAQ_LIST_MODE with Prescaler = 2 */
    cto[0] = SYN_XCP_CMD_SET_DAQ_LIST_MODE;
    cto[1] = 0x01;
    cto[2] = 0x00;
    cto[3] = 0x00;
    cto[4] = 0x01; /* Event channel 1 */
    cto[5] = 0x02; /* Prescaler 2 */
    cto[6] = 0x00;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_RES, dto[0]);

    /* START_STOP_DAQ_LIST Start */
    cto[0] = SYN_XCP_CMD_START_STOP_DAQ_LIST;
    cto[1] = 0x01; /* Start */
    cto[2] = 0x00;
    cto[3] = 0x00;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_RES, dto[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01, dto[1]); /* ODT count */

    /* First tick: cycle_counter = 1 (< 2), service_daq returns false */
    uint8_t daq_dto[8] = {0};
    uint8_t list_idx = 0;
    uint8_t odt_idx = 0;
    TEST_ASSERT_FALSE(syn_xcp_service_daq(&g_xcp_slave, 0x01U, daq_dto, &list_idx, &odt_idx));

    /* Second tick: cycle_counter = 2, service_daq returns true */
    TEST_ASSERT_TRUE(syn_xcp_service_daq(&g_xcp_slave, 0x01U, daq_dto, &list_idx, &odt_idx));
    TEST_ASSERT_EQUAL_HEX8(0x00, daq_dto[0]); /* ODT index 0 */
    TEST_ASSERT_EQUAL_HEX8(0x34, daq_dto[1]); /* var1 byte 0 */
    TEST_ASSERT_EQUAL_HEX8(0x12, daq_dto[2]); /* var1 byte 1 */
    TEST_ASSERT_EQUAL_HEX8(0x01, daq_dto[3]); /* var2 byte 0 */
    TEST_ASSERT_EQUAL_HEX8(0xEF, daq_dto[4]); /* var2 byte 1 */

    /* START_STOP_SYNCH Stop */
    cto[0] = SYN_XCP_CMD_START_STOP_SYNCH;
    cto[1] = 0x00; /* Stop */
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_RES, dto[0]);
    TEST_ASSERT_FALSE(g_xcp_slave.daq_lists[0].running);
}

static void test_xcp_bounds_and_null_checks(void)
{
    uint8_t cto[8] = {0};
    uint8_t dto[8] = {0};

    TEST_ASSERT_FALSE(syn_xcp_set_mta(NULL, 0, 0));
    TEST_ASSERT_FALSE(syn_xcp_process_cto(NULL, cto, dto));
    TEST_ASSERT_FALSE(syn_xcp_process_cto(&g_xcp_slave, NULL, dto));
    TEST_ASSERT_FALSE(syn_xcp_process_cto(&g_xcp_slave, cto, NULL));

    uint8_t l = 0, o = 0;
    TEST_ASSERT_FALSE(syn_xcp_service_daq(NULL, 1, dto, &l, &o));
    TEST_ASSERT_FALSE(syn_xcp_service_daq(&g_xcp_slave, 1, NULL, &l, &o));
    TEST_ASSERT_FALSE(syn_xcp_service_daq(&g_xcp_slave, 1, dto, NULL, &o));
    TEST_ASSERT_FALSE(syn_xcp_service_daq(&g_xcp_slave, 1, dto, &l, NULL));

    syn_xcp_init(&g_xcp_slave, 0x1234U);
    cto[0] = SYN_XCP_CMD_CONNECT;
    syn_xcp_process_cto(&g_xcp_slave, cto, dto);

    /* SYNCH command */
    cto[0] = SYN_XCP_CMD_SYNCH;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_ERR, dto[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_ERR_CMD_SYNTAX, dto[1]);

    /* Unknown command fallback */
    cto[0] = 0x11;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_ERR, dto[0]);
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_ERR_CMD_UNKNOWN, dto[1]);

    /* Out of bounds SET_DAQ_PTR */
    cto[0] = SYN_XCP_CMD_SET_DAQ_PTR;
    cto[2] = 0xFF;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_ERR, dto[0]);

    /* Out of bounds WRITE_DAQ */
    g_xcp_slave.current_daq_ptr_idx = SYN_XCP_MAX_ENTRIES_PER_ODT;
    cto[0] = SYN_XCP_CMD_WRITE_DAQ;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_ERR, dto[0]);

    /* Out of bounds SET_DAQ_LIST_MODE */
    cto[0] = SYN_XCP_CMD_SET_DAQ_LIST_MODE;
    cto[2] = 0xFF;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_ERR, dto[0]);

    /* Out of bounds START_STOP_DAQ_LIST */
    cto[0] = SYN_XCP_CMD_START_STOP_DAQ_LIST;
    cto[2] = 0xFF;
    TEST_ASSERT_TRUE(syn_xcp_process_cto(&g_xcp_slave, cto, dto));
    TEST_ASSERT_EQUAL_HEX8(SYN_XCP_PID_ERR, dto[0]);

    /* service_daq when list not running */
    TEST_ASSERT_FALSE(syn_xcp_service_daq(&g_xcp_slave, 1, dto, &l, &o));
}

void run_xcp_tests(void)
{
    RUN_TEST(test_xcp_init_and_connect_disconnect);
    RUN_TEST(test_xcp_mta_upload_download);
    RUN_TEST(test_xcp_daq_list_streaming);
    RUN_TEST(test_xcp_bounds_and_null_checks);
}
