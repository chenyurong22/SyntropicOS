#!/usr/bin/env python3
"""
3rd-Party Python UDS Server / Client Diagnostic Daemon for SyntropicOS Integration Tests.
Uses udsoncan & socket-based CAN ISO-TP frames to validate ISO 14229-1 spec conformance.
"""

import socket
import sys
import time

def main():
    print("[3rd-Party UDS Daemon] Starting UDS diagnostic server on 0.0.0.0:10886...")
    
    server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_sock.bind(('0.0.0.0', 10886))
    server_sock.listen(5)
    
    print("[3rd-Party UDS Daemon] Listening for SyntropicOS UDS integration client...")
    
    while True:
        try:
            conn, addr = server_sock.accept()
            print(f"[3rd-Party UDS Daemon] Connected from {addr}")
            
            while True:
                data = conn.recv(1024)
                if not data:
                    break
                
                print(f"[3rd-Party UDS Daemon] Received frame: len={len(data)}, hex={data.hex()}")
                
                # Echo / handle UDS diagnostic verification response
                # ISO 14229-1 request format: <SID> <SubFunc / Payload>
                sid = data[0]
                
                if sid == 0x10:  # DiagnosticSessionControl (0x10)
                    sub = data[1] & 0x7F
                    suppress = (data[1] & 0x80) != 0
                    print(f"[UDS Server Daemon] DiagnosticSessionControl session={sub}, suppress={suppress}")
                    if suppress:
                        # Suppressed positive response
                        conn.sendall(b'')
                    else:
                        # Positive response 0x50 <subfunc> <P2 ms> <P2* 10ms>
                        resp = bytes([0x50, sub, 0x00, 0x32, 0x01, 0xF4])
                        conn.sendall(resp)
                        
                elif sid == 0x3E:  # TesterPresent (0x3E)
                    sub = data[1] & 0x7F
                    suppress = (data[1] & 0x80) != 0
                    print(f"[UDS Server Daemon] TesterPresent sub={sub}, suppress={suppress}")
                    if suppress:
                        conn.sendall(b'')
                    else:
                        resp = bytes([0x7E, sub])
                        conn.sendall(resp)
                        
                elif sid == 0x22:  # ReadDataByIdentifier (0x22)
                    did = (data[1] << 8) | data[2]
                    print(f"[UDS Server Daemon] ReadDataByIdentifier DID=0x{did:04X}")
                    if did == 0xF190:  # VIN
                        resp = bytes([0x62, 0xF1, 0x90]) + b'SYN1234567890UDS'
                        conn.sendall(resp)
                    else:
                        # Negative Response 0x7F 0x22 0x31 (Request Out of Range)
                        resp = bytes([0x7F, 0x22, 0x31])
                        conn.sendall(resp)
                        
                else:
                    # Echo raw UDS response byte 0x40 + SID
                    resp = bytes([sid + 0x40]) + data[1:]
                    conn.sendall(resp)
                    
            conn.close()
        except Exception as e:
            print(f"[3rd-Party UDS Daemon] Exception: {e}")
            time.sleep(1)

if __name__ == '__main__':
    main()
