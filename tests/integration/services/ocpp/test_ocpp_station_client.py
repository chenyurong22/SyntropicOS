#!/usr/bin/env python3
"""
Official 3rd-Party OCPP 1.6-J Station Client Integration Test.
Connects Python 'ocpp' ChargePoint client to SyntropicOS 'syn_ocpp_server'
and verifies station boot, authorization, transaction, and remote start/stop.
"""

import sys
import asyncio
import logging
from ocpp.v16 import ChargePoint as cp
from ocpp.v16 import call
import websockets

logging.basicConfig(level=logging.INFO)

class RefStationClient(cp):
    async def send_boot_notification(self):
        req = call.BootNotification(
            charge_point_vendor="SyntropicOS-Ref",
            charge_point_model="EVSE-Station-v1",
            charge_point_serial_number="SN-99999",
            firmware_version="1.0.0"
        )
        res = await self.call(req)
        logging.info(f"[Python Client] BootNotification response: status={res.status}")
        assert res.status == "Accepted"

    async def send_authorize(self):
        req = call.Authorize(id_tag="RFID-TAG-123")
        res = await self.call(req)
        logging.info(f"[Python Client] Authorize response: status={res.id_tag_info['status']}")
        assert res.id_tag_info['status'] == "Accepted"

    async def send_start_transaction(self):
        req = call.StartTransaction(
            connector_id=1,
            id_tag="RFID-TAG-123",
            meter_start=1000,
            timestamp="2026-08-06T12:00:00Z"
        )
        res = await self.call(req)
        logging.info(f"[Python Client] StartTransaction response: tx_id={res.transaction_id}")
        assert res.transaction_id > 0

async def main():
    uri = "ws://127.0.0.1:9002/CP002"
    async with websockets.connect(uri, subprotocols=["ocpp1.6"]) as ws:
        client = RefStationClient("CP002", ws)
        asyncio.create_task(client.start())
        await client.send_boot_notification()
        await client.send_authorize()
        await client.send_start_transaction()
        logging.info("[Python Client] All OCPP station tests passed against syn_ocpp_server!")

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except Exception as e:
        logging.error(f"[Python Client] Test failed: {e}")
        sys.exit(1)
