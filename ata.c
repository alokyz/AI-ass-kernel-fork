#include "ata.h"
#include "port.h"
#include "heap.h"

static int ata_detected = 0;
static uint16_t ata_base = 0x1F0;

static void ata_wait_ready(void) {
    uint32_t i;
    for (i = 0; i < 100000; i++) { if (!(inb(ata_base + 7) & 0x80)) break; }
}

static void ata_wait_drq(void) {
    uint32_t i;
    for (i = 0; i < 100000; i++) { if (inb(ata_base + 7) & 0x08) break; }
}

int ata_detect(void) {
    outb(ata_base + 6, 0xA0);
    outb(ata_base + 7, 0xEC);
    ata_wait_ready();

    uint8_t status = inb(ata_base + 7);
    if (status == 0) { ata_detected = 0; return 0; }

    ata_wait_drq();
    uint16_t ident[256];
    uint32_t i;
    for (i = 0; i < 256; i++) ident[i] = inw(ata_base);

    if (ident[0] != 0 || ident[1] != 0) { ata_detected = 1; return 1; }
    return 0;
}

void ata_init(void) { ata_detected = 0; ata_detect(); }

uint8_t* ata_read_sectors(uint32_t lba, uint32_t count) {
    if (!ata_detected) return NULL;
    uint8_t* buf = (uint8_t*)kmalloc(count * 512);
    if (!buf) return NULL;

    outb(ata_base + 6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ata_base + 2, (uint8_t)count);
    outb(ata_base + 3, (uint8_t)(lba & 0xFF));
    outb(ata_base + 4, (uint8_t)((lba >> 8) & 0xFF));
    outb(ata_base + 5, (uint8_t)((lba >> 16) & 0xFF));
    outb(ata_base + 7, 0x20);

    uint32_t s;
    for (s = 0; s < count; s++) {
        ata_wait_drq();
        uint16_t* ptr = (uint16_t*)(buf + s * 512);
        uint32_t w;
        for (w = 0; w < 256; w++) ptr[w] = inw(ata_base);
    }
    return buf;
}

int ata_write_sectors(uint32_t lba, uint32_t count, uint8_t* data) {
    if (!ata_detected || !data) return -1;
    outb(ata_base + 6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ata_base + 2, (uint8_t)count);
    outb(ata_base + 3, (uint8_t)(lba & 0xFF));
    outb(ata_base + 4, (uint8_t)((lba >> 8) & 0xFF));
    outb(ata_base + 5, (uint8_t)((lba >> 16) & 0xFF));
    outb(ata_base + 7, 0x30);

    uint32_t s;
    for (s = 0; s < count; s++) {
        ata_wait_drq();
        uint16_t* ptr = (uint16_t*)(data + s * 512);
        uint32_t w;
        for (w = 0; w < 256; w++) outw(ata_base, ptr[w]);
    }
    ata_wait_ready();
    return 0;
}
