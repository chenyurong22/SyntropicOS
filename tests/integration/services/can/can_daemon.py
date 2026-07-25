#!/usr/bin/env python3
"""
Virtual CAN & CANopen Node Daemon for SyntropicOS Integration Tests
"""
import subprocess
import time
import sys

def setup_vcan():
    try:
        subprocess.run(["ip", "link", "add", "dev", "vcan0", "type", "vcan"], check=False)
        subprocess.run(["ip", "link", "set", "up", "dev", "vcan0"], check=False)
        print("[CAN Daemon] vcan0 interface setup complete", flush=True)
    except Exception as e:
        print(f"[CAN Daemon] Failed to setup vcan0: {e}", file=sys.stderr, flush=True)

def run_can_loop():
    setup_vcan()
    
    # Try importing python-can or fallback to socketcan via raw python sockets
    import socket
    import struct

    try:
        sock = socket.socket(socket.AF_CAN, socket.SOCK_RAW, socket.CAN_RAW)
        sock.bind(("vcan0",))
        print("[CAN Daemon] Raw SocketCAN bound to vcan0. Emitting CANopen NMT Heartbeats...", flush=True)
    except Exception as e:
        print(f"[CAN Daemon] SocketCAN bind failed (not running with CAP_NET_ADMIN?): {e}", file=sys.stderr, flush=True)
        return

    last_hb = 0
    while True:
        now = time.time()
        # Emit Heartbeat every 1.0s (COB-ID 0x701, Node-ID 1, State=0x05 Operational)
        if now - last_hb >= 1.0:
            cob_id = 0x701
            data = b'\x05' # Operational state
            can_pkt = struct.pack("=IB3x8s", cob_id, len(data), data.ljust(8, b'\x00'))
            try:
                sock.send(can_pkt)
            except Exception:
                pass
            last_hb = now

        # Non-blocking poll for SDO requests (COB-ID 0x601)
        sock.settimeout(0.1)
        try:
            pkt = sock.recv(16)
            if pkt and len(pkt) >= 16:
                cob_id, can_dlc, data = struct.unpack("=IB3x8s", pkt)
                if cob_id == 0x601: # SDO Request Node 1
                    # Respond with SDO Upload (Read) Response (COB-ID 0x581)
                    # Index 0x1000 (Device Type = 0x00020192 CiA 402 drive)
                    res_cob_id = 0x581
                    res_data = b'\x43\x00\x10\x00\x92\x01\x02\x00'
                    res_pkt = struct.pack("=IB3x8s", res_cob_id, 8, res_data)
                    sock.send(res_pkt)
                    print("[CAN Daemon] Responded to SDO Read Index 0x1000 (CiA 402 Drive)", flush=True)
        except socket.timeout:
            pass
        except Exception as e:
            print(f"[CAN Daemon] Loop error: {e}", file=sys.stderr, flush=True)

if __name__ == '__main__':
    run_can_loop()
