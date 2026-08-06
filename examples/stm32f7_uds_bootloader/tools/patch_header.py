#!/usr/bin/env python3
"""
Post-Build Header Patching Tool for STM32F767 UDS Bootloader Applications.

Calculates image size and CRC32 checksum, then patches the SYN_FBL_AppHeader at offset 0.
"""

import struct
import sys
import zlib

HEADER_MAGIC = 0x53594E31  # "SYN1"
HEADER_SIZE = 24  # sizeof(SYN_FBL_AppHeader)
STATE_VALID = 0x01
STATE_PENDING = 0x02


def patch_firmware_header(bin_path, version_major=1, version_minor=1, version_patch=0):
    with open(bin_path, "rb") as f:
        data = bytearray(f.read())

    if len(data) < HEADER_SIZE:
        print(f"Error: Binary {bin_path} too small ({len(data)} bytes).")
        sys.exit(1)

    payload = data[HEADER_SIZE:]
    image_size = len(payload)
    crc32_val = zlib.crc32(payload) & 0xFFFFFFFF

    # Pack SYN_FBL_AppHeader:
    # uint32 magic, uint16 major, uint16 minor, uint16 patch, uint16 reserved, uint32 size, uint32 crc, uint8 state, uint8 pad[3]
    header = struct.pack(
        "<IHHHHIIB3s",
        HEADER_MAGIC,
        version_major,
        version_minor,
        version_patch,
        0,
        image_size,
        crc32_val,
        STATE_VALID,
        b"\x00\x00\x00",
    )

    data[0:HEADER_SIZE] = header

    with open(bin_path, "wb") as f:
        f.write(data)

    print(f"[Post-Build Patch] Patched {bin_path}:")
    print(f"  Magic      : 0x{HEADER_MAGIC:08X} ('SYN1')")
    print(f"  Version    : V{version_major}.{version_minor}.{version_patch}")
    print(f"  Payload Size: {image_size} bytes")
    print(f"  CRC32      : 0x{crc32_val:08X}")
    print(f"  State      : 0x{STATE_VALID:02X} (Valid)")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 patch_header.py <app.bin> [major] [minor] [patch]")
        sys.exit(1)

    bin_file = sys.argv[1]
    maj = int(sys.argv[2]) if len(sys.argv) > 2 else 1
    min_ver = int(sys.argv[3]) if len(sys.argv) > 3 else 1
    pat = int(sys.argv[4]) if len(sys.argv) > 4 else 0

    patch_firmware_header(bin_file, maj, min_ver, pat)
