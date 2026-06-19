import re
with open('shell.c', 'r') as f:
    code = f.read()

code = code.replace('static const char* HELP_TEXT =', 'static char help_text[] =')
code = code.replace('static char cwd_path[512];\nstatic int shell_colors = 1;', 'static char cwd_path[512];')
code = code.replace('int start = 1, end = 10, step = 1;', 'int i, start = 1, end = 10, step = 1;')
code = code.replace('vga_puts("R  ", );', 'vga_puts("R  ");')
code = code.replace('vga_puts(HELP_TEXT)', 'vga_puts(help_text)')

# Fix argv -> args in shell_run
code = code.replace('if (argv[i][0]', 'if (args[i][0]')

# Remove unused functions
code = re.sub(r'static void cmd_lscpu_info\(void\) \{[^}]*\}', '', code, flags=re.DOTALL)
code = code.replace('static void cmd_ip(int argc, char** argv) {\n    if (argc > 1 && strcmp(argv[1], \"addr\") == 0) cmd_ifconfig();\n    else if (argc > 1 && strcmp(argv[1], \"link\") == 0) {\n        vga_puts(\"1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 state UNKNOWN\\n\");\n        vga_puts(\"    link/loopback 00:00:00:00:00:00\\n\");\n        vga_puts(\"2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500\\n\");\n        vga_puts(\"    link/ether 00:1a:2b:3c:4d:5e\\n\");\n    } else { cmd_ifconfig(); }\n}', '')
code = code.replace('static void cmd_mem(void) {\n    uint32_t total = pmm_get_total() * 4;\n    uint32_t free_m = pmm_get_free() * 4;\n    uint32_t used = total - free_m;\n    vga_puts(\"Mem:  \"); vga_print_dec(used); vga_puts(\"KB / \"); vga_print_dec(total); vga_puts(\"KB used  (\"); vga_print_dec(free_m); vga_puts(\"KB free)\\n\");\n    vga_puts(\"Heap: 16MB allocated at 0x01000000\\n\");\n}', '')

with open('shell.c', 'w') as f:
    f.write(code)
print('Fixed')
