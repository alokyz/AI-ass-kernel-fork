#include "types.h"
#include "multiboot.h"
#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "vga.h"
#include "keyboard.h"
#include "vfs.h"
#include "timer.h"
#include "pmm.h"
#include "heap.h"
#include "process.h"
#include "shell.h"
#include "ata.h"
#include "devtree.h"
#include "security.h"
#include "scheduler.h"
#include "signal.h"
#include "net.h"
#include "init.h"
#include "usb.h"

void kernel_main(uint32_t magic, uint32_t param) {
    UNUSED(param);
    gdt_init();
    idt_init();
    heap_init();
    timer_init(100);
    keyboard_init();
    vfs_init();
    process_init();
    scheduler_init();
    signal_init();
    net_init();

    if (magic == 0x2BADB002 && param) {
        multiboot_info_t* mboot = (multiboot_info_t*)param;
        if (mboot->flags & 0x40) {
            pmm_init(mboot->mmap_addr, mboot->mmap_length);
        }
    }

    ata_init();
    devtree_init();
    security_init();
    usb_init();

    vga_init();
    vga_clear();

    init_run();
    vga_puts("\n");
    shell_init();
    shell_run();
}
