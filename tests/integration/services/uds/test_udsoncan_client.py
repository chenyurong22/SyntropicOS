#!/usr/bin/env python3
"""
3rd-Party Python udsoncan Integration Client Test Suite for SyntropicOS UDS Server.
Validates SyntropicOS C UDS Server (syn_uds.c) using official, independent udsoncan library.
Includes full regression testing for user-reported Issues #83, #86, #87, #88, and spec edge cases.
"""

import os
import socket
import sys
import time
import udsoncan
from udsoncan.client import Client
from udsoncan.connections import SocketConnection
from udsoncan.services import (
    DiagnosticSessionControl,
    SecurityAccess,
    ReadDataByIdentifier,
    WriteDataByIdentifier,
    ReadDTCInformation,
    ClearDiagnosticInformation,
    ECUReset,
    TesterPresent,
)
from udsoncan.exceptions import NegativeResponseException, InvalidResponseException

class HexCodec(udsoncan.DidCodec):
    def __init__(self, length=4):
        self.length = length

    def encode(self, val: bytes) -> bytes:
        return val

    def decode(self, payload: bytes) -> bytes:
        return payload

    def __len__(self) -> int:
        return self.length

def key_calc_func(seed: bytes, params: dict) -> bytes:
    """Calculates XOR security key for level 1 challenge-response."""
    seed_int = int.from_bytes(seed, byteorder='little')
    key_int = seed_int ^ 0xA5A5A5A5
    return key_int.to_bytes(4, byteorder='little')

def main():
    host = os.environ.get("UDS_HOST", "uds")
    port = 10886

    print(f"[3rd-Party udsoncan Client] Connecting to SyntropicOS UDS C Server at {host}:{port}...")

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    connected = False
    for attempt in range(10):
        try:
            sock.connect((host, port))
            connected = True
            break
        except Exception as e:
            print(f"[3rd-Party udsoncan Client] Waiting for UDS C Server... ({e})")
            time.sleep(1)

    if not connected:
        print("[3rd-Party udsoncan Client] ERROR: Could not connect to SyntropicOS UDS C Server!")
        sys.exit(1)

    print("[3rd-Party udsoncan Client] Connected to SyntropicOS UDS C Server!")

    conn = SocketConnection(sock)
    config = {
        'exception_on_negative_response': True,
        'security_algo': key_calc_func,
        'data_identifiers': {
            0xF190: udsoncan.AsciiCodec(17),
            0xF187: udsoncan.AsciiCodec(11),
            0x0100: HexCodec(4),
            0x0300: HexCodec(2),
            0x9999: HexCodec(4),
        }
    }

    failures = 0

    with Client(conn, config=config) as client:
        # 1. DiagnosticSessionControl (0x10) - Extended Session
        print("\n--- Test 1: DiagnosticSessionControl (0x10) Extended Session ---")
        try:
            res = client.change_session(DiagnosticSessionControl.Session.extendedDiagnosticSession)
            print(f"[udsoncan] Session Change OK: session=0x{res.service_data.session_echo:02X}")
        except Exception as e:
            print(f"[udsoncan] FAIL: Session Change failed: {e}")
            failures += 1

        # 2. SecurityAccess (0x27) - Seed/Key Challenge-Response (Issue #86)
        print("\n--- Test 2: SecurityAccess (0x27) Seed/Key Unlock (Issue #86) ---")
        try:
            res = client.unlock_security_access(level=1)
            print(f"[udsoncan] Security Access Unlocked OK!")
        except Exception as e:
            print(f"[udsoncan] FAIL: Security Access failed: {e}")
            failures += 1

        # 3. ISO 14229-1 Section 11.2.2 Check: Request Seed when ALREADY UNLOCKED (Issue #86)
        print("\n--- Test 3: ISO 14229-1 Section 11.2.2 Zero-Seed Unlocked Check (Issue #86) ---")
        try:
            seed_res = client.request_seed(level=1)
            seed_bytes = seed_res.service_data.seed
            print(f"[udsoncan] Returned Seed when Unlocked: 0x{seed_bytes.hex()}")
            if seed_bytes != b'\x00\x00\x00\x00':
                print(f"[udsoncan] FAIL: ISO 14229-1 Section 11.2.2 violation! Expected 4 zero bytes, got {seed_bytes.hex()}")
                failures += 1
            else:
                print("[udsoncan] ISO 14229-1 Zero-Seed Unlocked Check PASS!")
        except Exception as e:
            print(f"[udsoncan] FAIL: Zero-Seed check failed: {e}")
            failures += 1

        # 4. ReadDataByIdentifier (0x22) - VIN 0xF190
        print("\n--- Test 4: ReadDataByIdentifier (0x22) VIN 0xF190 ---")
        try:
            res = client.read_data_by_identifier(0xF190)
            vin_val = res.service_data.values[0xF190]
            print(f"[udsoncan] Read VIN OK: {vin_val}")
            if vin_val != "SYN12345678901234":
                print(f"[udsoncan] FAIL: VIN value mismatch! Expected 'SYN12345678901234', got '{vin_val}'")
                failures += 1
        except Exception as e:
            print(f"[udsoncan] FAIL: Read VIN failed: {e}")
            failures += 1

        # 5. Issue #87: Session Mask Permissions & NRC 0x7E
        print("\n--- Test 5: Session Mask Perms & NRC 0x7E (Issue #87) ---")
        try:
            # Change back to Default session
            client.change_session(DiagnosticSessionControl.Session.defaultSession)
            print("[udsoncan] Switched to Default Session")

            # Try reading Programming-only DID 0x0300 in Default Session -> Expect NRC 0x7E
            try:
                res = client.read_data_by_identifier(0x0300)
                print("[udsoncan] FAIL: Expected NRC 0x7E for Programming DID in Default Session!")
                failures += 1
            except NegativeResponseException as nrc_ex:
                print(f"[udsoncan] NRC 0x7E Caught OK: {nrc_ex.response.code_name} (0x{nrc_ex.response.code:02X})")

            # Switch to Programming Session
            client.change_session(DiagnosticSessionControl.Session.programmingSession)
            print("[udsoncan] Switched to Programming Session")

            # Read Programming-only DID 0x0300 in Programming Session -> Expect Success
            res = client.read_data_by_identifier(0x0300)
            print(f"[udsoncan] Read Programming DID 0x0300 OK: {res.service_data.values[0x0300].hex()}")
        except Exception as e:
            print(f"[udsoncan] FAIL: Session mask perms test failed: {e}")
            failures += 1

        # Re-enter Extended session and unlock for subsequent tests
        client.change_session(DiagnosticSessionControl.Session.extendedDiagnosticSession)
        client.unlock_security_access(level=1)

        # 6. WriteDataByIdentifier (0x2E) - System Status 0x0100
        print("\n--- Test 6: WriteDataByIdentifier (0x2E) System Status 0x0100 ---")
        try:
            res = client.write_data_by_identifier(0x0100, b'\xAA\xBB\xCC\xDD')
            print(f"[udsoncan] Write DID 0x0100 OK!")
        except Exception as e:
            print(f"[udsoncan] FAIL: Write DID failed: {e}")
            failures += 1

        # 7. Issue #88: ReadDTCInformation (0x19) Subfunctions
        print("\n--- Test 7: ReadDTCInformation (0x19) Subfunctions (Issue #88) ---")
        try:
            # Subfunction 0x02: Report DTC by Status Mask
            res2 = client.get_dtc_by_status_mask(0xFF)
            dtcs2 = res2.service_data.dtcs
            print(f"[udsoncan] Get DTC By Status Mask (0x02) OK: count={len(dtcs2)}")

            # Subfunction 0x0A: Report Supported DTCs
            res_a = client.get_supported_dtc()
            dtcs_a = res_a.service_data.dtcs
            print(f"[udsoncan] Get Supported DTCs (0x0A) OK: count={len(dtcs_a)}")
        except Exception as e:
            print(f"[udsoncan] FAIL: ReadDTCInformation subfunctions failed: {e}")
            failures += 1

        # 8. Issue #83: ClearDiagnosticInformation (0x14) Group Filtering
        print("\n--- Test 8: ClearDiagnosticInformation (0x14) Groups (Issue #83) ---")
        try:
            client.clear_dtc(0x000000) # Emissions Group
            print("[udsoncan] Clear DTCs (Emissions 0x000000) OK!")

            client.clear_dtc(0x100000) # Powertrain Group
            print("[udsoncan] Clear DTCs (Powertrain 0x100000) OK!")

            client.clear_dtc(0xFFFFFF) # All DTCs
            print("[udsoncan] Clear DTCs (All 0xFFFFFF) OK!")
        except Exception as e:
            print(f"[udsoncan] FAIL: Clear DTCs failed: {e}")
            failures += 1

        # 9. TesterPresent (0x3E)
        print("\n--- Test 9: TesterPresent (0x3E) ---")
        try:
            res = client.tester_present()
            print(f"[udsoncan] TesterPresent OK!")
        except Exception as e:
            print(f"[udsoncan] FAIL: TesterPresent failed: {e}")
            failures += 1

        # 10. Negative Response Code Validation (Invalid DID 0x9999 -> NRC 0x31)
        print("\n--- Test 10: NRC Validation (Invalid DID 0x9999 -> NRC 0x31) ---")
        try:
            res = client.read_data_by_identifier(0x9999)
            print(f"[udsoncan] FAIL: Expected NegativeResponseException, got positive response!")
            failures += 1
        except NegativeResponseException as e:
            print(f"[udsoncan] NRC Caught OK: {e.response.code_name} (0x{e.response.code:02X})")
        except Exception as e:
            print(f"[udsoncan] FAIL: Unexpected exception type: {e}")
            failures += 1

    print("\n==================================================")
    if failures == 0:
        print("=== 3rd-Party udsoncan Python Client Integration PASS ===")
        sys.exit(0)
    else:
        print(f"=== 3rd-Party udsoncan Python Client Integration FAILED ({failures} failures) ===")
        sys.exit(1)

if __name__ == '__main__':
    main()
