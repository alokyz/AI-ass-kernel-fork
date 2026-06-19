#ifndef ORANGEX_ELF_H
#define ORANGEX_ELF_H

#include "types.h"

#define EI_NIDENT 16

typedef struct {
    uint32_t magic;
    uint8_t  ident[EI_NIDENT - 4];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint32_t entry;
    uint32_t phoff;
    uint32_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
} __attribute__((packed)) elf_header_t;

typedef struct {
    uint32_t type;
    uint32_t offset;
    uint32_t vaddr;
    uint32_t paddr;
    uint32_t filesz;
    uint32_t memsz;
    uint32_t flags;
    uint32_t align;
} __attribute__((packed)) elf_phdr_t;

typedef struct {
    uint32_t name;
    uint32_t type;
    uint32_t flags;
    uint32_t addr;
    uint32_t offset;
    uint32_t size;
    uint32_t link;
    uint32_t info;
    uint32_t addralign;
    uint32_t entsize;
} __attribute__((packed)) elf_shdr_t;

#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4

#define SHT_NULL    0
#define SHT_PROGBITS 1
#define SHT_SYMTAB  2
#define SHT_STRTAB  3

#define ELF_MAGIC   0x464C457F

uint32_t elf_load(uint8_t* data, uint32_t size);

#endif
