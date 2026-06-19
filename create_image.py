#!/usr/bin/env python3
"""Create a bootable floppy image: bootloader (512B) + flat kernel binary."""
import os, sys

D = os.path.dirname(os.path.abspath(__file__))
BOOT = os.path.join(D, "boot16.bin")
KERN = os.path.join(D, "orangex_kernel.bin")
OUT  = os.path.join(D, "orangex_boot.img")
SZ   = 1474560

def main():
    for p, label in [(BOOT, "boot16.bin"), (KERN, "orangex_kernel.bin")]:
        if not os.path.isfile(p):
            print(f"ERROR: {label} not found. Run: make all")
            sys.exit(1)

    boot = open(BOOT, "rb").read()
    kern = open(KERN, "rb").read()

    # Verify it's NOT an ELF (ELF magic = 0x7F 'E' 'L' 'F')
    if len(kern) >= 4 and kern[0] == 0x7F and kern[1] == ord('E'):
        print("ERROR: orangex_kernel.bin is still an ELF file!")
        print("       Run: i686-elf-objcopy -O binary orangex_kernel.bin orangex_kernel.flat")
        print("       Then update create_image.py to use the .flat file.")
        sys.exit(1)

    if len(boot) != 512:
        print(f"ERROR: Boot sector must be exactly 512 bytes (got {len(boot)})")
        sys.exit(1)

    if len(kern) > SZ - 512:
        print(f"ERROR: Kernel too large ({len(kern)} bytes, max {SZ - 512})")
        sys.exit(1)

    img = bytearray(SZ)
    img[:512] = boot
    img[512:512+len(kern)] = kern

    open(OUT, "wb").write(img)
    print(f"OK: {OUT} ({len(boot)} + {len(kern)} = {len(boot)+len(kern)} bytes)")

if __name__ == "__main__":
    main()
