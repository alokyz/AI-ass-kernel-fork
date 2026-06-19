#include "init.h"
#include "vga.h"
#include "timer.h"
#include "process.h"

static const char* boot_log[] = {
    "[  OK ] Started OrangeX Init System",
    "[  OK ] Mounted root filesystem",
    "[  OK ] Started device manager",
    "[  OK ] Started udev daemon",
    "[  OK ] Started network manager",
    "[  OK ] Started security daemon",
    "[  OK ] Started crond",
    "[  OK ] Started sshd",
    "[  OK ] Started dbus",
    "[  OK ] Reached target graphical.target",
    "[  OK ] Started display manager",
    "[  OK ] Started login manager",
};

void init_run(void) {
    uint32_t i;
    uint32_t num_entries = sizeof(boot_log) / sizeof(boot_log[0]);
    for (i = 0; i < num_entries; i++) {
        vga_puts_color(boot_log[i], VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK));
        vga_puts("\n");
        timer_sleep(50);
    }
    vga_puts("\n");
    vga_puts_color("Welcome to OrangeX 2.0.0\n", VGA_COLOR(VGA_LIGHT_CYAN, VGA_BLACK));
    vga_puts_color("orangex login: ", VGA_COLOR(VGA_WHITE, VGA_BLACK));
}
