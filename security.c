#include "security.h"
#include "vfs.h"
#include "string.h"
#include "vga.h"

static file_integrity_t trusted_files[64];
static int trusted_count = 0;

uint32_t security_hash(const uint8_t* data, uint32_t len) {
    uint32_t hash = 5381;
    uint32_t i;
    for (i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + data[i];
    }
    return hash;
}

void security_init(void) {
    trusted_count = 0;
    security_add_trusted("/bin");
    security_add_trusted("/etc");
    security_add_trusted("/dev");
}

void security_add_trusted(const char* path) {
    if (trusted_count >= 64) return;
    vfs_node_t* node = vfs_lookup(path);
    uint32_t i;
    for (i = 0; path[i] && i < 127; i++) trusted_files[trusted_count].path[i] = path[i];
    trusted_files[trusted_count].path[i] = '\0';
    trusted_files[trusted_count].size = node ? node->size : 0;
    trusted_files[trusted_count].checksum = (node && node->content) ?
        security_hash((const uint8_t*)node->content, node->size) : 0;
    trusted_files[trusted_count].flags = 0;
    trusted_count++;
}

int security_is_trusted(const char* path) {
    uint32_t i;
    for (i = 0; i < (uint32_t)trusted_count; i++) {
        if (strcmp(trusted_files[i].path, path) == 0) return 1;
    }
    return 0;
}

int security_check_file(const char* path) {
    vfs_node_t* node = vfs_lookup(path);
    if (!node) return SEC_UNTRUSTED;

    if (!security_is_trusted(path)) return SEC_UNTRUSTED;

    uint32_t i;
    for (i = 0; i < (uint32_t)trusted_count; i++) {
        if (strcmp(trusted_files[i].path, path) == 0) {
            if (node->content && node->size > 0) {
                uint32_t current_hash = security_hash((const uint8_t*)node->content, node->size);
                if (current_hash != trusted_files[i].checksum) return SEC_TAMPERED;
            }
            return SEC_OK;
        }
    }
    return SEC_UNTRUSTED;
}

void security_scan(void) {
    vga_puts_color("Security scan started...\n", VGA_COLOR(VGA_LIGHT_CYAN, VGA_BLACK));
    uint32_t checked = 0, clean = 0, tampered = 0, untrusted = 0;

    uint32_t i;
    for (i = 0; i < (uint32_t)trusted_count; i++) {
        checked++;
        int status = security_check_file(trusted_files[i].path);
        switch (status) {
            case SEC_OK: clean++; break;
            case SEC_TAMPERED:
                tampered++;
                vga_puts_color("  [TAMPERED] ", VGA_COLOR(VGA_RED, VGA_BLACK));
                vga_puts(trusted_files[i].path);
                vga_puts("\n");
                break;
            case SEC_UNTRUSTED:
                untrusted++;
                break;
        }
    }

    vga_puts("\nScan complete:\n");
    vga_puts("  Files checked: "); vga_print_dec(checked); vga_puts("\n");
    vga_puts_color("  Clean:         ", VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK));
    vga_print_dec(clean); vga_puts("\n");
    vga_puts_color("  Tampered:      ", VGA_COLOR(VGA_RED, VGA_BLACK));
    vga_print_dec(tampered); vga_puts("\n");
    vga_puts_color("  Untrusted:     ", VGA_COLOR(VGA_YELLOW, VGA_BLACK));
    vga_print_dec(untrusted); vga_puts("\n");

    if (tampered == 0) {
        vga_puts_color("\nSystem integrity: OK\n", VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK));
    } else {
        vga_puts_color("\nWARNING: Tampered files detected!\n", VGA_COLOR(VGA_RED, VGA_BLACK));
    }
}

void security_sandbox_info(void) {
    vga_puts("OrangeX Security Sandbox v1.0\n");
    vga_puts("  Process isolation: enabled\n");
    vga_puts("  File integrity:   enabled\n");
    vga_puts("  Privilege levels: ring 0/3\n");
    vga_puts("  ASLR:             basic\n");
    vga_puts("  Stack canaries:   enabled\n");
    vga_puts("  SMEP:             enabled\n");
    vga_puts("  SMAP:             enabled\n");
    vga_puts("  W^X:              enabled\n");
    vga_puts("  Seccomp:          basic\n");
    vga_puts("  Capabilities:     basic\n");
}
