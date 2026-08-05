/**
 * @file test_gbt27930.c
 * @brief Unit test suite for GB/T 27930 EV DC Fast Charging Protocol.
 */

#include "syntropic/proto/syn_gbt27930.h"
#include "unity/unity.h"

#include <string.h>

#if !defined(SYN_USE_GBT27930) || SYN_USE_GBT27930

static SYN_GBT27930_Session g_bms;
static SYN_GBT27930_Session g_charger;

static void gbt27930_test_setup(void)
{
    syn_gbt27930_init(&g_bms, SYN_GBT27930_ROLE_BMS);
    syn_gbt27930_init(&g_charger, SYN_GBT27930_ROLE_CHARGER);

    g_bms.bms_cfg.max_charge_volt_v = 4000; /* 400.0V */
    g_bms.bms_cfg.max_charge_curr_a = 1000; /* 100.0A */

    g_charger.charger_cfg.max_output_volt_v = 5000; /* 500.0V */
    g_charger.charger_cfg.min_output_volt_v = 2000; /* 200.0V */
}

static void test_gbt27930_init_and_idle(void)
{
    gbt27930_test_setup();
    TEST_ASSERT_EQUAL_INT(SYN_GBT27930_STATE_IDLE, g_bms.state);
    TEST_ASSERT_EQUAL_INT(SYN_GBT27930_STATE_IDLE, g_charger.state);

    SYN_CAN_Frame frame;
    TEST_ASSERT_FALSE(syn_gbt27930_step(&g_bms, 100, &frame));
}

static void test_gbt27930_handshake_flow(void)
{
    gbt27930_test_setup();
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_start_handshake(&g_bms));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_start_handshake(&g_charger));

    TEST_ASSERT_EQUAL_INT(SYN_GBT27930_STATE_HANDSHAKE, g_bms.state);
    TEST_ASSERT_EQUAL_INT(SYN_GBT27930_STATE_HANDSHAKE, g_charger.state);

    /* Step Charger -> produces CHM frame */
    SYN_CAN_Frame chm_frame;
    TEST_ASSERT_TRUE(syn_gbt27930_step(&g_charger, 250, &chm_frame));

    /* Process CHM at BMS */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_bms, &chm_frame));

    /* Step BMS -> produces BHM frame */
    SYN_CAN_Frame bhm_frame;
    TEST_ASSERT_TRUE(syn_gbt27930_step(&g_bms, 250, &bhm_frame));

    /* Process BHM at Charger */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_charger, &bhm_frame));

    /* Send CRM Recognition OK from Charger */
    SYN_CAN_Frame crm;
    memset(&crm, 0, sizeof(crm));
    crm.extended = true;
    crm.dlc = 8;
    crm.id = syn_j1939_id_pack(6, SYN_GBT27930_PGN_CRM, SYN_GBT27930_ADDR_CHARGER,
                               SYN_GBT27930_ADDR_BMS);
    crm.data[0] = 0x00; /* Recognized */

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_bms, &crm));
    TEST_ASSERT_EQUAL_INT(SYN_GBT27930_STATE_PARAM_CONFIG, g_bms.state);

    /* Send BRM Recognition from BMS */
    SYN_CAN_Frame brm;
    memset(&brm, 0, sizeof(brm));
    brm.extended = true;
    brm.dlc = 8;
    brm.id = syn_j1939_id_pack(6, SYN_GBT27930_PGN_BRM, SYN_GBT27930_ADDR_BMS,
                               SYN_GBT27930_ADDR_CHARGER);

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_charger, &brm));
    TEST_ASSERT_EQUAL_INT(SYN_GBT27930_STATE_PARAM_CONFIG, g_charger.state);
}

static void test_gbt27930_param_and_charging_loop(void)
{
    g_bms.state = SYN_GBT27930_STATE_PARAM_CONFIG;
    g_charger.state = SYN_GBT27930_STATE_PARAM_CONFIG;

    /* Ready exchange: CRO (Charger) and BRO (BMS) */
    g_bms.ready_for_charging = true;
    g_charger.ready_for_charging = true;

    SYN_CAN_Frame cro_frame;
    TEST_ASSERT_TRUE(syn_gbt27930_step(&g_charger, 250, &cro_frame));

    SYN_CAN_Frame bro_frame;
    TEST_ASSERT_TRUE(syn_gbt27930_step(&g_bms, 250, &bro_frame));

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_bms, &cro_frame));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_charger, &bro_frame));

    TEST_ASSERT_EQUAL_INT(SYN_GBT27930_STATE_CHARGING, g_bms.state);
    TEST_ASSERT_EQUAL_INT(SYN_GBT27930_STATE_CHARGING, g_charger.state);

    /* Active Charging Periodic Step */
    g_bms.telemetry.volt_demand_v = 3800; /* 380.0V */
    g_bms.telemetry.curr_demand_a = 500;  /* 50.0A */

    SYN_CAN_Frame bcl_frame;
    TEST_ASSERT_TRUE(syn_gbt27930_step(&g_bms, 50, &bcl_frame));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_charger, &bcl_frame));
    TEST_ASSERT_EQUAL_UINT16(3800, g_charger.telemetry.volt_demand_v);

    g_charger.telemetry.measured_volt_v = 3795;
    g_charger.telemetry.measured_curr_a = 498;
    SYN_CAN_Frame ccs_frame;
    TEST_ASSERT_TRUE(syn_gbt27930_step(&g_charger, 50, &ccs_frame));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_bms, &ccs_frame));
    TEST_ASSERT_EQUAL_UINT16(3795, g_bms.telemetry.measured_volt_v);
}

static void test_gbt27930_stop_and_error(void)
{
    g_bms.state = SYN_GBT27930_STATE_CHARGING;
    syn_gbt27930_stop_charging(&g_bms, 0x01); /* Stop reason SOC complete */
    TEST_ASSERT_EQUAL_INT(SYN_GBT27930_STATE_STOPPING, g_bms.state);

    SYN_CAN_Frame bst_frame;
    TEST_ASSERT_TRUE(syn_gbt27930_step(&g_bms, 10, &bst_frame));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_charger, &bst_frame));
    TEST_ASSERT_EQUAL_INT(SYN_GBT27930_STATE_STOPPING, g_charger.state);
    TEST_ASSERT_EQUAL_UINT8(0x01, g_charger.stop_reason);

    /* Error handling */
    SYN_CAN_Frame bem;
    memset(&bem, 0, sizeof(bem));
    bem.extended = true;
    bem.dlc = 8;
    bem.id = syn_j1939_id_pack(2, SYN_GBT27930_PGN_BEM, SYN_GBT27930_ADDR_BMS,
                               SYN_GBT27930_ADDR_CHARGER);
    bem.data[0] = 0xAA; /* Overvolt fault */

    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_charger, &bem));
    TEST_ASSERT_EQUAL_INT(SYN_GBT27930_STATE_ERROR, g_charger.state);
    TEST_ASSERT_EQUAL_UINT8(0xAA, g_charger.fault_code);

    SYN_CAN_Frame err_tx;
    TEST_ASSERT_TRUE(syn_gbt27930_step(&g_charger, 250, &err_tx));
    SYN_J1939_Header hdr;
    syn_j1939_id_unpack(err_tx.id, &hdr);
    TEST_ASSERT_EQUAL_HEX32(SYN_GBT27930_PGN_CEM, hdr.pgn);

    g_bms.state = SYN_GBT27930_STATE_ERROR;
    g_bms.fault_code = 0x11;
    TEST_ASSERT_TRUE(syn_gbt27930_step(&g_bms, 250, &err_tx));

    g_charger.state = SYN_GBT27930_STATE_STOPPING;
    g_charger.stop_reason = 0x02;
    TEST_ASSERT_FALSE(syn_gbt27930_step(&g_charger, 1, &err_tx));
    TEST_ASSERT_TRUE(syn_gbt27930_step(&g_charger, 10, &err_tx));

    g_bms.state = SYN_GBT27930_STATE_CHARGING;
    TEST_ASSERT_FALSE(syn_gbt27930_step(&g_bms, 1, &err_tx));

    g_bms.state = SYN_GBT27930_STATE_STOPPING;
    g_bms.stop_reason = 0x03;
    TEST_ASSERT_TRUE(syn_gbt27930_step(&g_bms, 10, &err_tx));

    g_bms.state = (SYN_GBT27930_State)99;
    TEST_ASSERT_FALSE(syn_gbt27930_step(&g_bms, 250, &err_tx));
    g_charger.state = (SYN_GBT27930_State)99;
    TEST_ASSERT_FALSE(syn_gbt27930_step(&g_charger, 250, &err_tx));
}

static void test_gbt27930_all_pgns_coverage(void)
{
    gbt27930_test_setup();

    /* BCP (BMS Charging Parameter) */
    g_charger.state = SYN_GBT27930_STATE_PARAM_CONFIG;
    SYN_CAN_Frame bcp;
    memset(&bcp, 0, sizeof(bcp));
    bcp.extended = true;
    bcp.dlc = 8;
    bcp.id = syn_j1939_id_pack(6, SYN_GBT27930_PGN_BCP, SYN_GBT27930_ADDR_BMS,
                               SYN_GBT27930_ADDR_CHARGER);
    bcp.data[0] = 0xE0;
    bcp.data[1] = 0x0F; /* 406.4V */
    bcp.data[2] = 0x64;
    bcp.data[3] = 0x00; /* 10.0A */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_charger, &bcp));

    /* CML (Charger Max Output) */
    g_bms.state = SYN_GBT27930_STATE_PARAM_CONFIG;
    SYN_CAN_Frame cml;
    memset(&cml, 0, sizeof(cml));
    cml.extended = true;
    cml.dlc = 8;
    cml.id = syn_j1939_id_pack(6, SYN_GBT27930_PGN_CML, SYN_GBT27930_ADDR_CHARGER,
                               SYN_GBT27930_ADDR_BMS);
    cml.data[0] = 0x88;
    cml.data[1] = 0x13; /* 500.0V */
    cml.data[2] = 0xD0;
    cml.data[3] = 0x07; /* 200.0V */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_bms, &cml));

    /* BCS (BMS Overall Status) */
    g_charger.state = SYN_GBT27930_STATE_CHARGING;
    SYN_CAN_Frame bcs;
    memset(&bcs, 0, sizeof(bcs));
    bcs.extended = true;
    bcs.dlc = 8;
    bcs.id = syn_j1939_id_pack(6, SYN_GBT27930_PGN_BCS, SYN_GBT27930_ADDR_BMS,
                               SYN_GBT27930_ADDR_CHARGER);
    bcs.data[0] = 0x90;
    bcs.data[1] = 0x0F; /* 398.4V */
    bcs.data[2] = 0x32;
    bcs.data[3] = 0x00; /* 5.0A */
    bcs.data[6] = 85;   /* SOC 85% */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_charger, &bcs));
    TEST_ASSERT_EQUAL_UINT8(85, g_charger.telemetry.soc_percent);

    /* CST & CEM processing */
    SYN_CAN_Frame cst;
    memset(&cst, 0, sizeof(cst));
    cst.extended = true;
    cst.dlc = 8;
    cst.id = syn_j1939_id_pack(4, SYN_GBT27930_PGN_CST, SYN_GBT27930_ADDR_CHARGER,
                               SYN_GBT27930_ADDR_BMS);
    cst.data[0] = 0x02;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_bms, &cst));
    TEST_ASSERT_EQUAL_INT(SYN_GBT27930_STATE_STOPPING, g_bms.state);

    SYN_CAN_Frame cem;
    memset(&cem, 0, sizeof(cem));
    cem.extended = true;
    cem.dlc = 8;
    cem.id = syn_j1939_id_pack(2, SYN_GBT27930_PGN_CEM, SYN_GBT27930_ADDR_CHARGER,
                               SYN_GBT27930_ADDR_BMS);
    cem.data[0] = 0x55;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_bms, &cem));
    TEST_ASSERT_EQUAL_INT(SYN_GBT27930_STATE_ERROR, g_bms.state);
}

static void test_gbt27930_edge_cases_and_nulls(void)
{
    syn_gbt27930_init(NULL, SYN_GBT27930_ROLE_BMS);
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_gbt27930_start_handshake(NULL));
    syn_gbt27930_stop_charging(NULL, 0x01);

    SYN_CAN_Frame frame;
    memset(&frame, 0, sizeof(frame));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_gbt27930_process_rx_frame(NULL, &frame));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_gbt27930_process_rx_frame(&g_bms, NULL));

    /* Invalid J1939 ID unpack (e.g. non 29-bit std frame) */
    frame.extended = false;
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_gbt27930_process_rx_frame(&g_bms, &frame));

    TEST_ASSERT_FALSE(syn_gbt27930_step(NULL, 10, &frame));
    TEST_ASSERT_FALSE(syn_gbt27930_step(&g_bms, 10, NULL));

    /* Unknown PGN frame */
    frame.extended = true;
    frame.id = syn_j1939_id_pack(6, 0x00FF99, SYN_GBT27930_ADDR_CHARGER, SYN_GBT27930_ADDR_BMS);
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_bms, &frame));
}

static void test_gbt27930_timeouts(void)
{
    gbt27930_test_setup();

    /* 1. Handshake Phase Timeout (5,000ms limit) */
    syn_gbt27930_start_handshake(&g_bms);
    TEST_ASSERT_EQUAL_INT(SYN_GBT27930_STATE_HANDSHAKE, g_bms.state);

    SYN_CAN_Frame tx;
    /* Step 4,900ms -> still in handshake phase */
    syn_gbt27930_step(&g_bms, 4900, &tx);
    TEST_ASSERT_EQUAL_INT(SYN_GBT27930_STATE_HANDSHAKE, g_bms.state);

    /* Step additional 200ms -> 5,100ms total -> timeout triggers ERROR state & BEM frame */
    TEST_ASSERT_TRUE(syn_gbt27930_step(&g_bms, 200, &tx));
    TEST_ASSERT_EQUAL_INT(SYN_GBT27930_STATE_ERROR, g_bms.state);
    TEST_ASSERT_EQUAL_UINT8(0x01, g_bms.fault_code);

    SYN_J1939_Header hdr;
    syn_j1939_id_unpack(tx.id, &hdr);
    TEST_ASSERT_EQUAL_HEX32(SYN_GBT27930_PGN_BEM, hdr.pgn);

    /* 2. Charging Phase Timeout (1,000ms limit) */
    g_charger.state = SYN_GBT27930_STATE_CHARGING;
    syn_gbt27930_step(&g_charger, 900, &tx);
    TEST_ASSERT_EQUAL_INT(SYN_GBT27930_STATE_CHARGING, g_charger.state);

    /* Step additional 200ms -> 1,100ms total -> timeout triggers ERROR state & CEM frame */
    TEST_ASSERT_TRUE(syn_gbt27930_step(&g_charger, 200, &tx));
    TEST_ASSERT_EQUAL_INT(SYN_GBT27930_STATE_ERROR, g_charger.state);
    TEST_ASSERT_EQUAL_UINT8(0x01, g_charger.fault_code);

    syn_j1939_id_unpack(tx.id, &hdr);
    TEST_ASSERT_EQUAL_HEX32(SYN_GBT27930_PGN_CEM, hdr.pgn);
}

static void test_gbt27930_short_dlc_branches(void)
{
    gbt27930_test_setup();
    SYN_CAN_Frame f;
    memset(&f, 0, sizeof(f));
    f.extended = true;
    f.dlc = 1; /* Short DLC < expected */

    /* BHM dlc < 2 */
    g_charger.state = SYN_GBT27930_STATE_HANDSHAKE;
    f.id = syn_j1939_id_pack(6, SYN_GBT27930_PGN_BHM, SYN_GBT27930_ADDR_BMS,
                             SYN_GBT27930_ADDR_CHARGER);
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_charger, &f));

    /* CRM data != 0x00 */
    g_bms.state = SYN_GBT27930_STATE_HANDSHAKE;
    f.id = syn_j1939_id_pack(6, SYN_GBT27930_PGN_CRM, SYN_GBT27930_ADDR_CHARGER,
                             SYN_GBT27930_ADDR_BMS);
    f.data[0] = 0xAA;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_bms, &f));

    /* BCP dlc < 6 */
    g_charger.state = SYN_GBT27930_STATE_PARAM_CONFIG;
    f.id = syn_j1939_id_pack(6, SYN_GBT27930_PGN_BCP, SYN_GBT27930_ADDR_BMS,
                             SYN_GBT27930_ADDR_CHARGER);
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_charger, &f));

    /* CML dlc < 8 */
    g_bms.state = SYN_GBT27930_STATE_PARAM_CONFIG;
    f.id = syn_j1939_id_pack(6, SYN_GBT27930_PGN_CML, SYN_GBT27930_ADDR_CHARGER,
                             SYN_GBT27930_ADDR_BMS);
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_bms, &f));

    /* BRO data != 0xAA */
    g_charger.state = SYN_GBT27930_STATE_PARAM_CONFIG;
    f.id = syn_j1939_id_pack(6, SYN_GBT27930_PGN_BRO, SYN_GBT27930_ADDR_BMS,
                             SYN_GBT27930_ADDR_CHARGER);
    f.data[0] = 0x00;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_charger, &f));

    /* CRO data != 0xAA */
    g_bms.state = SYN_GBT27930_STATE_PARAM_CONFIG;
    f.id = syn_j1939_id_pack(6, SYN_GBT27930_PGN_CRO, SYN_GBT27930_ADDR_CHARGER,
                             SYN_GBT27930_ADDR_BMS);
    f.data[0] = 0x00;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_bms, &f));

    /* BCL dlc < 5 */
    g_charger.state = SYN_GBT27930_STATE_CHARGING;
    f.id = syn_j1939_id_pack(6, SYN_GBT27930_PGN_BCL, SYN_GBT27930_ADDR_BMS,
                             SYN_GBT27930_ADDR_CHARGER);
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_charger, &f));

    /* BCS dlc < 7 */
    f.id = syn_j1939_id_pack(6, SYN_GBT27930_PGN_BCS, SYN_GBT27930_ADDR_BMS,
                             SYN_GBT27930_ADDR_CHARGER);
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_charger, &f));

    /* CCS dlc < 6 */
    g_bms.state = SYN_GBT27930_STATE_CHARGING;
    f.id = syn_j1939_id_pack(6, SYN_GBT27930_PGN_CCS, SYN_GBT27930_ADDR_CHARGER,
                             SYN_GBT27930_ADDR_BMS);
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_bms, &f));

    /* BST / BEM dlc = 0 */
    f.dlc = 0;
    f.id = syn_j1939_id_pack(4, SYN_GBT27930_PGN_BST, SYN_GBT27930_ADDR_BMS,
                             SYN_GBT27930_ADDR_CHARGER);
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_charger, &f));

    f.id = syn_j1939_id_pack(2, SYN_GBT27930_PGN_BEM, SYN_GBT27930_ADDR_BMS,
                             SYN_GBT27930_ADDR_CHARGER);
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_gbt27930_process_rx_frame(&g_charger, &f));
}

void run_gbt27930_tests(void)
{
    RUN_TEST(test_gbt27930_init_and_idle);
    RUN_TEST(test_gbt27930_handshake_flow);
    RUN_TEST(test_gbt27930_param_and_charging_loop);
    RUN_TEST(test_gbt27930_stop_and_error);
    RUN_TEST(test_gbt27930_all_pgns_coverage);
    RUN_TEST(test_gbt27930_edge_cases_and_nulls);
    RUN_TEST(test_gbt27930_timeouts);
    RUN_TEST(test_gbt27930_short_dlc_branches);
}

#endif /* SYN_USE_GBT27930 */
