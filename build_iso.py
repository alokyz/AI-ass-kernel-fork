#!/usr/bin/env python3
"""
OrangeX Kernel - ISO Builder for Windows
Creates a bootable ISO (or floppy image fallback) without grub-mkrescue.

Usage:
    python build_iso.py              # tries xorriso, falls back to floppy
    python build_iso.py --xorriso /path/to/xorriso.exe
    python build_iso.py --floppy     # force raw floppy image
"""

import os
import sys
import struct
import shutil
import subprocess
import argparse

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
KERNEL_BIN = os.path.join(SCRIPT_DIR, "orangex_kernel.bin")
ISO_OUT    = os.path.join(SCRIPT_DIR, "orangex_kernel.iso")
FLOPPY_OUT = os.path.join(SCRIPT_DIR, "orangex_kernel.flp")
ISO_DIR    = os.path.join(SCRIPT_DIR, "iso")

GRUB_CFG = (
    'set timeout=0\n'
    'set default=0\n'
    '\n'
    'menuentry "OrangeX Kernel" {\n'
    '    multiboot /boot/orangex_kernel.bin\n'
    '    boot\n'
    '}\n'
)


def find_tool(names):
    for name in names:
        path = shutil.which(name)
        if path:
            return path
    return None


def build_iso_xorriso(xorriso_path, kernel_path, output_path):
    """Create a bootable Multiboot ISO using xorriso + GRUB i386-pc core.img."""
    boot_dir = os.path.join(ISO_DIR, "boot")
    grub_dir = os.path.join(boot_dir, "grub")
    os.makedirs(grub_dir, exist_ok=True)

    shutil.copy2(kernel_path, os.path.join(boot_dir, "orangex_kernel.bin"))
    with open(os.path.join(grub_dir, "grub.cfg"), "w") as f:
        f.write(GRUB_CFG)

    core_img = os.path.join(grub_dir, "core.img")
    bios_boot = os.path.join(grub_dir, "bios_boot.img")

    have_grub_files = os.path.isfile(core_img) and os.path.isfile(bios_boot)

    if not have_grub_files:
        print("[INFO] GRUB boot files not found in iso/boot/grub/")
        print("       Creating non-bootable data ISO (use with -kernel in QEMU)")
        print("       For full bootable ISO, place core.img and bios_boot.img in:")
        print(f"       {grub_dir}")

    if have_grub_files:
        cmd = [
            xorriso_path,
            "-as", "mkisofs",
            "-b", "boot/grub/bios_boot.img",
            "-no-emul-boot",
            "-boot-load-size", "4",
            "-boot-info-table",
            "--grub2-boot-info",
            "-o", output_path,
            ISO_DIR
        ]
    else:
        cmd = [
            xorriso_path,
            "-as", "mkisofs",
            "-o", output_path,
            ISO_DIR
        ]

    print(f"[*] Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"[ERROR] xorriso failed:\n{result.stderr}")
        return False
    print(f"[OK] ISO created: {output_path}")
    return True


def build_floppy_image(kernel_path, output_path):
    """Create a 1.44MB floppy image with Multiboot kernel (no GRUB needed)."""
    SECTOR_SIZE = 512
    FLOPPY_SIZE = 1474560

    with open(kernel_path, "rb") as f:
        kernel_data = f.read()

    kernel_sectors = (len(kernel_data) + SECTOR_SIZE - 1) // SECTOR_SIZE
    if kernel_sectors > 2870:
        print("[ERROR] Kernel too large for floppy image")
        return False

    image = bytearray(FLOPPY_SIZE)

    BPB = bytearray(62)
    BPB[0:3] = b'\xEB\x3C\x90'
    BPB[3:11] = b'MKFS    '
    BPB[11:13] = struct.pack('<H', SECTOR_SIZE)
    BPB[13] = 1
    BPB[14:16] = struct.pack('<H', 1)
    BPB[16] = 2
    BPB[17:19] = struct.pack('<H', 224)
    BPB[19:21] = struct.pack('<H', 2880)
    BPB[21] = 0xF0
    BPB[22:24] = struct.pack('<H', 9)
    BPB[24:26] = struct.pack('<H', 18)
    BPB[26] = 2
    BPB[28:30] = struct.pack('<H', 0)

    image[0:62] = BPB

    boot_sig = bytearray([0x55, 0xAA])
    image[510:512] = boot_sig

    root_dir_entries = 224
    fat_start = 1
    fat_sectors = 9
    root_start = fat_start + fat_sectors
    root_sectors = (root_dir_entries * 32 + SECTOR_SIZE - 1) // SECTOR_SIZE
    data_start = root_start + root_sectors

    fat = bytearray(fat_sectors * SECTOR_SIZE)
    fat[0] = 0xF0
    fat[1] = 0xFF

    next_free = 2
    kernel_clusters = kernel_sectors

    alloc_chain = []
    for i in range(kernel_clusters):
        alloc_chain.append(next_free)
        if i < kernel_clusters - 1:
            fat[next_free * 3 // 2] = (next_free + 1) & 0xFF
            if next_free % 2 == 0:
                fat[next_free * 3 // 2] |= 0x00
            else:
                fat[next_free * 3 // 2 + 1] = 0x00
        next_free += 1

    for i in range(len(alloc_chain)):
        idx = alloc_chain[i]
        next_idx = alloc_chain[i + 1] if i + 1 < len(alloc_chain) else 0xFFF
        byte_pos = idx * 3 // 2
        if idx % 2 == 0:
            fat[byte_pos] = next_idx & 0xFF
            fat[byte_pos + 1] = (fat[byte_pos + 1] & 0xF0) | ((next_idx >> 8) & 0x0F)
        else:
            fat[byte_pos] = (fat[byte_pos] & 0x0F) | ((next_idx & 0x0F) << 4)
            fat[byte_pos + 1] = (next_idx >> 4) & 0xFF

    fat_offset = fat_start * SECTOR_SIZE
    image[fat_offset:fat_offset + len(fat)] = fat

    root_offset = root_start * SECTOR_SIZE
    dir_entry = bytearray(32)
    dir_entry[0:8] = b'ORANGEX '
    dir_entry[8:11] = b'BIN'
    dir_entry[11] = 0x20
    dir_entry[26:28] = struct.pack('<H', alloc_chain[0])
    dir_entry[28:32] = struct.pack('<I', len(kernel_data))
    image[root_offset:root_offset + 32] = dir_entry

    for i, cluster in enumerate(alloc_chain):
        data_offset = (data_start + cluster - 2) * SECTOR_SIZE
        start = i * SECTOR_SIZE
        end = min(start + SECTOR_SIZE, len(kernel_data))
        chunk = kernel_data[start:end]
        image[data_offset:data_offset + len(chunk)] = chunk

    with open(output_path, "wb") as f:
        f.write(image)

    print(f"[OK] Floppy image created: {output_path}")
    print(f"     Run with: qemu-system-i386 -fda {output_path}")
    return True


def build_iso_simple(kernel_path, output_path):
    """Create a simple ISO9660 image (data only, boot with -kernel)."""
    with open(kernel_path, "rb") as f:
        kernel_data = f.read()

    boot_dir = os.path.join(ISO_DIR, "boot")
    os.makedirs(boot_dir, exist_ok=True)
    shutil.copy2(kernel_path, os.path.join(boot_dir, "orangex_kernel.bin"))
    with open(os.path.join(boot_dir, "grub.cfg"), "w") as f:
        f.write(GRUB_CFG)

    xorriso = find_tool(["xorriso", "xorriso.exe"])
    if xorriso:
        cmd = [
            xorriso, "-as", "mkisofs",
            "-V", "ORANGEX",
            "-o", output_path,
            ISO_DIR
        ]
        print(f"[*] Running: {' '.join(cmd)}")
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode == 0:
            print(f"[OK] ISO created: {output_path}")
            print(f"     Run with: qemu-system-i386 -cdrom {output_path} -kernel {kernel_path}")
            return True
        print(f"[WARN] xorriso failed: {result.stderr.strip()}")

    print("[INFO] No ISO tools found. Using floppy image fallback.")
    return False


def main():
    parser = argparse.ArgumentParser(description="OrangeX Kernel ISO Builder")
    parser.add_argument("--xorriso", type=str, default=None, help="Path to xorriso executable")
    parser.add_argument("--floppy", action="store_true", help="Force floppy image creation")
    args = parser.parse_args()

    if not os.path.isfile(KERNEL_BIN):
        print(f"[ERROR] Kernel not found: {KERNEL_BIN}")
        print("        Run 'make' first to build the kernel.")
        sys.exit(1)

    print(f"[*] Kernel: {KERNEL_BIN} ({os.path.getsize(KERNEL_BIN)} bytes)")

    if args.floppy:
        build_floppy_image(KERNEL_BIN, FLOPPY_OUT)
        shutil.rmtree(ISO_DIR, ignore_errors=True)
        return

    xorriso = args.xorriso or find_tool(["xorriso", "xorriso.exe", "xorriso.exe"])

    if xorriso:
        print(f"[*] Found xorriso: {xorriso}")
        if build_iso_xorriso(xorriso, KERNEL_BIN, ISO_OUT):
            shutil.rmtree(ISO_DIR, ignore_errors=True)
            return
        print("[WARN] xorriso ISO build failed, falling back to floppy")

    if not build_iso_simple(KERNEL_BIN, ISO_OUT):
        build_floppy_image(KERNEL_BIN, FLOPPY_OUT)

    shutil.rmtree(ISO_DIR, ignore_errors=True)


if __name__ == "__main__":
    main()
