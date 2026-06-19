#ifndef ORANGEX_MULTIBOOT_H
#define ORANGEX_MULTIBOOT_H

#include "types.h"

#define MULTIBOOT_MAGIC     0x1BADB002
#define MULTIBOOT_FLAGS     0x00000003
#define MULTIBOOT_CHECKSUM  -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

typedef struct {
    uint32_t magic;
    uint32_t flags;
    uint32_t checksum;
} __attribute__((packed)) multiboot_header_t;

typedef struct {
    uint32_t tabsize;
    uint32_t strsize;
    uint32_t addr;
    uint32_t reserved;
} __attribute__((packed)) multiboot_aout_t;

typedef struct {
    uint32_t num;
    uint32_t size;
    uint32_t addr;
    uint32_t shndx;
} __attribute__((packed)) multiboot_elf_t;

typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    union {
        multiboot_aout_t aout;
        multiboot_elf_t elf;
    } syms;
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
} __attribute__((packed)) multiboot_info_t;

typedef struct {
    uint32_t size;
    uint32_t base_low;
    uint32_t base_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
} __attribute__((packed)) multiboot_mmap_entry_t;

#define MULTIBOOT_FRAMEBUFFER  0x80000

#endif
