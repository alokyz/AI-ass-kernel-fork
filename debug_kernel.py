#!/usr/bin/env python3
"""
Try to find the VBE data by searching the boot image.
The bootloader stores VBE info at 0x500 in memory.
When loaded by QEMU, the boot sector is at 0x7C00.
The kernel is at 0x10000.
0x500 in memory corresponds to... nothing in the image (it's BIOS-written).
"""
import struct

# Read the kernel binary
with open("orangex_kernel.bin", "rb") as f:
    kern = f.read()

print(f"Kernel binary: {len(kern)} bytes")

# Find the gfx_init_vbe function by looking for the pattern
# The function reads from a pointer and checks for magic 0x4F5258
# Let's search for this constant in the binary
magic = struct.pack('<I', 0x004F5258)
pos = kern.find(magic)
if pos >= 0:
    print(f"Found VBE magic 0x4F5258 at offset 0x{pos:X}")
else:
    print("VBE magic not found in kernel binary")

# Also check what the entry point looks like
print(f"\nFirst 64 bytes of kernel binary:")
for i in range(0, min(64, len(kern)), 16):
    hex_str = kern[i:i+16].hex()
    ascii_str = ''.join(chr(b) if 32 <= b < 127 else '.' for b in kern[i:i+16])
    print(f"  {i:04X}: {hex_str}  {ascii_str}")
