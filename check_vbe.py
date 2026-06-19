#!/usr/bin/env python3
"""Read VBE data from the floppy image to see what BIOS reported."""
import struct, sys

with open("orangex_boot.img", "rb") as f:
    data = f.read()

# Our struct is at offset 0x500 in physical memory
# But in the floppy image, the kernel starts at sector 1 (offset 512)
# and is loaded to 0x10000. The struct at 0x500 is NOT in the image.
# Let's check what's at offset 512-512+0x500=... no, 0x500 is below the kernel.

# The VBE mode info from BIOS is at 0x8000 in memory.
# The clean struct is at 0x500 in memory.
# Neither is in the floppy image.

# Let's check the first bytes of the kernel to see if it starts correctly.
kernel = data[512:]
print(f"Kernel size: {len(kernel)} bytes")
print(f"First 32 bytes: {kernel[:32].hex()}")
print(f"Bytes 0x100-0x110 (kernel_main offset): {kernel[0x100:0x110].hex()}")

# Search for the VBE valid magic in the kernel
for i in range(0, min(len(kernel), 65000), 4):
    v = struct.unpack_from('<I', kernel, i)[0]
    if v == 0x4F5258:
        print(f"Found VBE magic at kernel offset 0x{i:X} (mem addr 0x{0x10000+i:X})")
        # Check surrounding values
        fb = struct.unpack_from('<I', kernel, i-20)[0]
        pt = struct.unpack_from('<I', kernel, i-16)[0]
        wd = struct.unpack_from('<I', kernel, i-12)[0]
        ht = struct.unpack_from('<I', kernel, i-8)[0]
        bp = struct.unpack_from('<I', kernel, i-4)[0]
        print(f"  fb=0x{fb:08X} pitch={pt} width={wd} height={ht} bpp={bp}")
        break
