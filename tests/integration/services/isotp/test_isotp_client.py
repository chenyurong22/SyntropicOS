#!/usr/bin/env python3
"""
3rd-Party Python isotp Integration Client Test Suite for SyntropicOS ISO-TP Stack.
Validates SyntropicOS C ISO-TP Stack (syn_isotp.c) using official, independent isotp library.
Tests Single Frame (SF), Multi-Frame (FF+CF), Large Multi-Frame (512B), and Flow Control (FC) parameters.
"""

import os
import socket
import struct
import sys
import time
import can
import isotp

class TcpCanBus(can.bus.BusABC):
    """Emulates a python-can Bus interface over TCP socket connection."""
    def __init__(self, sock, channel='vcan0', **kwargs):
        super().__init__(channel=channel, **kwargs)
        self.sock = sock
        self.sock.setblocking(False)

    def _recv_internal(self, timeout):
        try:
            data = self.sock.recv(13)
            if len(data) == 13:
                can_id, dlc = struct.unpack(">IB", data[:5])
                msg = can.Message(arbitration_id=can_id, dlc=dlc, data=data[5:5+dlc])
                return msg, False
        except Exception:
            pass
        return None, False

    def send(self, msg, timeout=None):
        can_id = msg.arbitration_id
        dlc = msg.dlc
        data_bytes = bytes(msg.data)
        if len(data_bytes) < 8:
            data_bytes = data_bytes + b'\x00' * (8 - len(data_bytes))

        frame_data = struct.pack(">IB", can_id, dlc) + data_bytes[:8]
        self.sock.sendall(frame_data)

    def _close_internal(self):
        try:
            self.sock.close()
        except Exception:
            pass

def main():
    host = os.environ.get("ISOTP_HOST", "isotp")
    port = 10887

    print(f"[3rd-Party isotp Client] Connecting to SyntropicOS ISO-TP C Server at {host}:{port}...")

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    connected = False
    for attempt in range(10):
        try:
            sock.connect((host, port))
            connected = True
            break
        except Exception as e:
            print(f"[3rd-Party isotp Client] Waiting for ISO-TP C Server... ({e})")
            time.sleep(1)

    if not connected:
        print("[3rd-Party isotp Client] ERROR: Could not connect to SyntropicOS ISO-TP C Server!")
        sys.exit(1)

    print("[3rd-Party isotp Client] Connected to SyntropicOS ISO-TP C Server!")

    bus = TcpCanBus(sock)
    # Python client transmits on 0x7E0, receives on 0x7E8
    addr = isotp.Address(isotp.AddressingMode.Normal_11bits, txid=0x7E0, rxid=0x7E8)
    params = {
        'blocksize': 8,
        'stmin': 5,
        'tx_data_length': 8
    }

    stack = isotp.CanStack(bus=bus, address=addr, params=params)

    failures = 0

    # 1. Single Frame (SF) <= 7 bytes
    print("\n--- Test 1: Single Frame (SF) Transmission (5 bytes) ---")
    payload1 = b"HELLO"
    stack.send(payload1)

    t0 = time.time()
    rx_payload = None
    while time.time() - t0 < 3.0:
        stack.process()
        if stack.available():
            rx_payload = stack.recv()
            break
        time.sleep(0.005)

    if rx_payload == payload1:
        print(f"[isotp] SF Echo Received OK: {rx_payload.decode('ascii')}")
    else:
        print(f"[isotp] FAIL: SF Echo mismatch! Expected {payload1}, got {rx_payload}")
        failures += 1

    # 2. Multi-Frame (FF + CF) 64 bytes
    print("\n--- Test 2: Multi-Frame (FF + CF) Transmission (64 bytes) ---")
    payload2 = b"A" * 64
    stack.send(payload2)

    t0 = time.time()
    rx_payload = None
    while time.time() - t0 < 3.0:
        stack.process()
        if stack.available():
            rx_payload = stack.recv()
            break
        time.sleep(0.005)

    if rx_payload == payload2:
        print(f"[isotp] Multi-Frame (64B) Echo Received OK! Length={len(rx_payload)}")
    else:
        print(f"[isotp] FAIL: Multi-Frame (64B) Echo mismatch! Length={len(rx_payload) if rx_payload else 0}")
        failures += 1

    # 3. Large Multi-Frame (512 bytes)
    print("\n--- Test 3: Large Multi-Frame Transmission (512 bytes) ---")
    payload3 = b"B" * 512
    stack.send(payload3)

    t0 = time.time()
    rx_payload = None
    while time.time() - t0 < 5.0:
        stack.process()
        if stack.available():
            rx_payload = stack.recv()
            break
        time.sleep(0.005)

    if rx_payload == payload3:
        print(f"[isotp] Large Multi-Frame (512B) Echo Received OK! Length={len(rx_payload)}")
    else:
        print(f"[isotp] FAIL: Large Multi-Frame (512B) Echo mismatch! Length={len(rx_payload) if rx_payload else 0}")
        failures += 1

    print("\n==================================================")
    if failures == 0:
        print("=== 3rd-Party isotp Python Client Integration PASS ===")
        sys.exit(0)
    else:
        print(f"=== 3rd-Party isotp Python Client Integration FAILED ({failures} failures) ===")
        sys.exit(1)

if __name__ == '__main__':
    main()
