#include "devtree.h"
#include "string.h"
#include "heap.h"
#include "vga.h"

static dev_node_t* dev_list = NULL;

static dev_node_t* create_dev(const char* name, uint32_t type, uint32_t major, uint32_t minor) {
    dev_node_t* d = (dev_node_t*)kmalloc(sizeof(dev_node_t));
    if (!d) return NULL;
    uint32_t i;
    for (i = 0; name[i] && i < 31; i++) d->name[i] = name[i];
    d->name[i] = '\0';
    d->type = type;
    d->major = major;
    d->minor = minor;
    d->next = dev_list;
    dev_list = d;
    return d;
}

void devtree_init(void) {
    dev_list = NULL;
    create_dev("tty0",     DEV_TTY,    4, 0);
    create_dev("tty1",     DEV_TTY,    4, 1);
    create_dev("null",     DEV_NULL,   1, 3);
    create_dev("zero",     DEV_ZERO,   1, 5);
    create_dev("random",   DEV_RANDOM, 1, 8);
    create_dev("keyboard", DEV_KBD,    13, 0);
    create_dev("vga",      DEV_VGA,    29, 0);
    create_dev("sda",      DEV_SDA,    8, 0);
    create_dev("sda1",     DEV_SDA,    8, 1);
    create_dev("sda2",     DEV_SDA,    8, 2);
    create_dev("eth0",     DEV_NET,   10, 0);
    create_dev("wlan0",    DEV_NET,   11, 0);
}

dev_node_t* devtree_find(const char* name) {
    dev_node_t* d = dev_list;
    while (d) {
        if (strcmp(d->name, name) == 0) return d;
        d = d->next;
    }
    return NULL;
}

void devtree_list(void) {
    vga_puts("Character devices:\n");
    vga_puts("  4,0   tty0\n");
    vga_puts("  4,1   tty1\n");
    vga_puts("  1,3   null\n");
    vga_puts("  1,5   zero\n");
    vga_puts("  1,8   random\n");
    vga_puts("Block devices:\n");
    vga_puts("  8,0   sda\n");
    vga_puts("  8,1   sda1\n");
    vga_puts("  8,2   sda2\n");
    vga_puts("Network devices:\n");
    vga_puts("  10,0  eth0\n");
    vga_puts("  11,0  wlan0\n");
    vga_puts("Input devices:\n");
    vga_puts("  13,0  keyboard\n");
    vga_puts("  29,0  vga\n");
}
