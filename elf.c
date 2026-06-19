#include "elf.h"
#include "string.h"
#include "heap.h"

uint32_t elf_load(uint8_t* data, uint32_t size) {
    elf_header_t* header = (elf_header_t*)data;

    if (header->magic != ELF_MAGIC) return 0;
    if (header->type != 2) return 0;

    elf_phdr_t* phdr = (elf_phdr_t*)(data + header->phoff);
    uint32_t i;
    for (i = 0; i < header->phnum; i++) {
        if (phdr[i].type == PT_LOAD) {
            uint32_t memsz = phdr[i].memsz;
            uint32_t filesz = phdr[i].filesz;
            uint32_t vaddr = phdr[i].vaddr;

            uint8_t* dest = (uint8_t*)vaddr;
            if (filesz > 0 && phdr[i].offset + filesz <= size) {
                memcpy(dest, data + phdr[i].offset, filesz);
            }
            if (memsz > filesz) {
                memset(dest + filesz, 0, memsz - filesz);
            }
        }
    }

    return header->entry;
}
