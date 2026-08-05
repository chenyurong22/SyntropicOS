/**
 * @file main.c
 * @brief GB/T 27930 EV DC Fast Charger (EVSE) ↔ Vehicle (BMS) Protocol Demo.
 */

#include "syntropic/proto/syn_gbt27930.h"
#include <stdio.h>
#include <string.h>

static SYN_GBT27930_Session g_charger;
static SYN_GBT27930_Session g_bms;

int main(void)
{
    printf("=== SyntropicOS GB/T 27930 EV DC Fast Charger Demo ===\n");

    /* Initialize Off-Board Charger Node (EVSE, 0x56) */
    syn_gbt27930_init(&g_charger, SYN_GBT27930_ROLE_CHARGER);
    g_charger.charger_cfg.max_output_volt_v = 7500; /* 750.0 V */
    g_charger.charger_cfg.min_output_volt_v = 2000; /* 200.0 V */
    g_charger.charger_cfg.max_output_curr_a = 2500; /* 250.0 A */

    /* Initialize Vehicle BMS Node (0xF4) */
    syn_gbt27930_init(&g_bms, SYN_GBT27930_ROLE_BMS);
    g_bms.bms_cfg.battery_type = 0x06;         /* NMC Lithium */
    g_bms.bms_cfg.max_charge_volt_v = 4200;    /* 420.0 V */
    g_bms.bms_cfg.max_charge_curr_a = 1500;    /* 150.0 A */

    /* Start Handshake Phase */
    printf("[System] Initiating GB/T 27930 Handshake Phase (Phase 1)...\n");
    syn_gbt27930_start_handshake(&g_charger);
    syn_gbt27930_start_handshake(&g_bms);

    SYN_CAN_Frame bus_frame;

    /* Step Charger -> CHM Frame */
    if (syn_gbt27930_step(&g_charger, 250, &bus_frame)) {
        printf("[EVSE -> CAN] Handshake CHM Frame Transmitted (PGN 0x002600)\n");
        syn_gbt27930_process_rx_frame(&g_bms, &bus_frame);
    }

    /* Step BMS -> BHM Frame */
    if (syn_gbt27930_step(&g_bms, 250, &bus_frame)) {
        printf("[BMS -> CAN] Handshake BHM Frame Transmitted (PGN 0x002700, Max Volt: %u.%uV)\n",
               g_bms.bms_cfg.max_charge_volt_v / 10, g_bms.bms_cfg.max_charge_volt_v % 10);
        syn_gbt27930_process_rx_frame(&g_charger, &bus_frame);
    }

    /* Send CRM (Charger Recognition) -> Moves BMS to Parameter Config Phase */
    SYN_CAN_Frame crm;
    memset(&crm, 0, sizeof(crm));
    crm.extended = true;
    crm.dlc = 8;
    crm.id = syn_j1939_id_pack(6, SYN_GBT27930_PGN_CRM, SYN_GBT27930_ADDR_CHARGER, SYN_GBT27930_ADDR_BMS);
    crm.data[0] = 0x00; /* Recognition Success */
    syn_gbt27930_process_rx_frame(&g_bms, &crm);

    /* Send BRM (BMS Recognition) -> Moves Charger to Parameter Config Phase */
    SYN_CAN_Frame brm;
    memset(&brm, 0, sizeof(brm));
    brm.extended = true;
    brm.dlc = 8;
    brm.id = syn_j1939_id_pack(6, SYN_GBT27930_PGN_BRM, SYN_GBT27930_ADDR_BMS, SYN_GBT27930_ADDR_CHARGER);
    syn_gbt27930_process_rx_frame(&g_charger, &brm);

    printf("[System] Transitioned to Parameter Configuration Phase (Phase 2 & 3)...\n");
    g_bms.ready_for_charging = true;
    g_charger.ready_for_charging = true;

    /* Exchange Ready States (BRO & CRO) */
    if (syn_gbt27930_step(&g_bms, 250, &bus_frame)) {
        printf("[BMS -> CAN] BMS Ready BRO Frame Transmitted (0xAA)\n");
        syn_gbt27930_process_rx_frame(&g_charger, &bus_frame);
    }
    if (syn_gbt27930_step(&g_charger, 250, &bus_frame)) {
        printf("[EVSE -> CAN] Charger Output Ready CRO Frame Transmitted (0xAA)\n");
        syn_gbt27930_process_rx_frame(&g_bms, &bus_frame);
    }

    printf("[System] Both Nodes Ready! Transitioning to Active Charging Phase (Phase 4)...\n");

    /* Active Charging Loop Simulation */
    g_bms.telemetry.volt_demand_v = 4000; /* Demand 400.0 V */
    g_bms.telemetry.curr_demand_a = 1000; /* Demand 100.0 A */

    if (syn_gbt27930_step(&g_bms, 50, &bus_frame)) {
        printf("[BMS -> CAN] Charging Demand BCL Frame (400.0V, 100.0A Constant Current)\n");
        syn_gbt27930_process_rx_frame(&g_charger, &bus_frame);
    }

    g_charger.telemetry.measured_volt_v = 3998;
    g_charger.telemetry.measured_curr_a = 995;
    if (syn_gbt27930_step(&g_charger, 50, &bus_frame)) {
        printf("[EVSE -> CAN] Measured Charger Status CCS Frame (399.8V Output, 99.5A Output)\n");
        syn_gbt27930_process_rx_frame(&g_bms, &bus_frame);
    }

    /* Stop Charging Simulation */
    printf("[System] Battery Reached Target SOC (100%%). Stopping Charging...\n");
    syn_gbt27930_stop_charging(&g_bms, 0x01);
    if (syn_gbt27930_step(&g_bms, 10, &bus_frame)) {
        printf("[BMS -> CAN] BMS Stop Charging BST Frame Transmitted\n");
        syn_gbt27930_process_rx_frame(&g_charger, &bus_frame);
    }

    printf("=== GB/T 27930 EV DC Fast Charger Demo Completed Successfully ===\n");
    return 0;
}
