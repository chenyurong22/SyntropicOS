#!/usr/bin/env python3
"""
3rd-Party Python udsoncan Integration Client Test Suite for SyntropicOS UDS Server.
Validates SyntropicOS C UDS Server (syn_uds.c) using official, independent udsoncan library.
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
            0x9999: HexCodec(4),
        }
    }

    failures = 0

    with Client(conn, config=config) as client:
        # 1. DiagnosticSessionControl (0x10) - Extended Session
        print("\n--- Test 1: DiagnosticSessionControl (0x10) via udsoncan ---")
        try:
            res = client.change_session(DiagnosticSessionControl.Session.extendedDiagnosticSession)
            print(f"[udsoncan] Session Change OK: session=0x{res.service_data.session_echo:02X}")
        except Exception as e:
            print(f"[udsoncan] FAIL: Session Change failed: {e}")
            failures += 1

        # 2. SecurityAccess (0x27) - Challenge/Response Seed-Key
        print("\n--- Test 2: SecurityAccess (0x27) Seed/Key via udsoncan ---")
        try:
            res = client.unlock_security_access(level=1)
            print(f"[udsoncan] Security Access Unlocked OK!")
        except Exception as e:
            print(f"[udsoncan] FAIL: Security Access failed: {e}")
            failures += 1

        # 3. ReadDataByIdentifier (0x22) - VIN 0xF190
        print("\n--- Test 3: ReadDataByIdentifier (0x22) VIN 0xF190 via udsoncan ---")
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

        # 4. WriteDataByIdentifier (0x2E) - System Status 0x0100
        print("\n--- Test 4: WriteDataByIdentifier (0x2E) System Status 0x0100 via udsoncan ---")
        try:
            res = client.write_data_by_identifier(0x0100, b'\xAA\xBB\xCC\xDD')
            print(f"[udsoncan] Write DID 0x0100 OK!")
        except Exception as e:
            print(f"[udsoncan] FAIL: Write DID failed: {e}")
            failures += 1

        # 5. ReadDTCInformation (0x19) - Get Supported DTCs
        print("\n--- Test 5: ReadDTCInformation (0x19) Supported DTCs via udsoncan ---")
        try:
            res = client.get_supported_dtc()
            dtcs = res.service_data.dtcs
            print(f"[udsoncan] Read Supported DTCs OK: count={len(dtcs)}")
        except Exception as e:
            print(f"[udsoncan] FAIL: ReadDTCInformation failed: {e}")
            failures += 1

        # 6. ClearDiagnosticInformation (0x14) - Clear All DTCs
        print("\n--- Test 6: ClearDiagnosticInformation (0x14) via udsoncan ---")
        try:
            res = client.clear_dtc(0xFFFFFF)
            print(f"[udsoncan] Clear DTCs OK!")
        except Exception as e:
            print(f"[udsoncan] FAIL: Clear DTCs failed: {e}")
            failures += 1

        # 7. TesterPresent (0x3E)
        print("\n--- Test 7: TesterPresent (0x3E) via udsoncan ---")
        try:
            res = client.tester_present()
            print(f"[udsoncan] TesterPresent OK!")
        except Exception as e:
            print(f"[udsoncan] FAIL: TesterPresent failed: {e}")
            failures += 1

        # 8. Negative Response Code (NRC 0x31 Out of Range for invalid DID 0x9999)
        print("\n--- Test 8: NRC Validation (Invalid DID 0x9999) via udsoncan ---")
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
