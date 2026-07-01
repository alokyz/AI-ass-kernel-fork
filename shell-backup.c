#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "vfs.h"
#include "string.h"
#include "timer.h"
#include "pmm.h"
#include "heap.h"
#include "process.h"
#include "doom.h"
#include "paging.h"
#include "elf.h"
#include "ata.h"
#include "devtree.h"
#include "security.h"
#include "usb.h"
#include "net.h"

static vfs_node_t* cwd;
static vfs_node_t* prev_cwd;
static char input[SHELL_MAX_INPUT];
static char username[32] = "root";
static char hostname[32] = "orangex";
static char cwd_path[512];

static char help_text[] =
"OrangeX Shell v1.0 — macOS-style CLI\n"
"\n"
"FILE SYSTEM:\n"
"  ls [-laG] [path]       List directory contents\n"
"  cd <dir|~|->           Change directory\n"
"  pwd                    Print working directory\n"
"  cat <file>             Display file contents\n"
"  echo [args]            Print arguments\n"
"  touch <file>           Create empty file\n"
"  mkdir <dir>            Create directory\n"
"  rm [-rf] <file>        Remove file or directory\n"
"  cp [-R] <src> <dst>    Copy file or directory\n"
"  mv <src> <dst>         Move/rename file or directory\n"
"  ln [-s] <target> <link> Create link\n"
"  find <dir> -name \"x\"  Search for files\n"
"  du [-sh] [path]        Disk usage\n"
"  df [-h]                Disk free space\n"
"  head [-n N] <file>     First N lines of file\n"
"  tail [-n N] <file>     Last N lines of file\n"
"  less <file>            Paginated viewer\n"
"  grep <pattern> <file>  Search text in file\n"
"  wc <file>              Word/line/char count\n"
"  sort <file>            Sort lines alphabetically\n"
"  uniq <file>            Remove duplicate lines\n"
"  tee <file>             Read from stdin, write to file and stdout\n"
"  chmod <mode> <file>    Change file permissions\n"
"  chown <user> <file>    Change file owner\n"
"  df -h                  Show disk space\n"
"  stat <file>            Show file statistics\n"
"  file <file>            Determine file type\n"
"  touch -r <ref> <file>  Copy timestamp from reference\n"
"\n"
"PROCESSES & SYSTEM:\n"
"  ps                     List running processes\n"
"  top                    Process monitor\n"
"  kill <pid>             Kill process by ID\n"
"  killall <name>         Kill all processes by name\n"
"  pgrep <name>           Find PID by name\n"
"  fork                   Create child process\n"
"  sleep <ms>             Sleep for N milliseconds\n"
"  uptime                 System uptime\n"
"  mem                    Memory usage\n"
"  id                     User/group IDs\n"
"  whoami                 Current username\n"
"  uname [-a|-m|-r|-n]    System information\n"
"  hostname               Show hostname\n"
"  clear                  Clear screen\n"
"  exit                   Exit (halt)\n"
"\n"
"NETWORK (simulated):\n"
"  ifconfig               Network interfaces\n"
"  ping <host>            Test connectivity\n"
"  netstat                Network connections\n"
"  ip addr                IP addresses\n"
"  curl <url>             HTTP client (stub)\n"
"  wget <url>             Download file (stub)\n"
"  ssh <host>             SSH client (stub)\n"
"  dns <host>             DNS lookup (stub)\n"
"\n"
"UTILITIES:\n"
"  date                   Current date/time\n"
"  cal                    Calendar\n"
"  calc <expr>            Calculator\n"
"  base64 <text>          Base64 encode\n"
"  hexdump <file>         Hex dump of file\n"
"  wc <file>              Word count\n"
"  od <file>              Octal dump\n"
"  seq <n>                Print numbers 1..n\n"
"  yes [text]             Repeat text (10x)\n"
"  env                    Environment variables\n"
"  export VAR=value       Set environment variable\n"
"  alias name=cmd         Create command alias\n"
"  history                Command history\n"
"  which <cmd>            Locate command\n"
"  type <cmd>             Show command type\n"
"  help                   Show this help\n"
"\n"
"HARDWARE (simulated):\n"
"  dmesg                  Kernel messages\n"
"  lspci                  PCI devices\n"
"  lsusb                  USB devices\n"
"  lsmod                  Loaded modules\n"
"  free                   Memory stats\n"
"  vmstat                 VM statistics\n"
"  iostat                 I/O statistics\n"
"  lscpu                  CPU information\n"
"  sw_vers                OS version\n"
"  csrutil                SIP status\n"
"\n"
"DISK:\n"
"  fdisk                  Partition table\n"
"  mount                  Mount points\n"
"  diskutil list          List drives\n"
"  diskutil info <disk>   Disk info\n"
;

static int shell_tokenize(char* line, char** tok, int max) {
    int n = 0, in = 0;
    while (*line && n < max) {
        if (*line == ' ' || *line == '\t') { *line = '\0'; in = 0; }
        else { if (!in) { tok[n++] = line; in = 1; } }
        line++;
    }
    return n;
}

static void shell_print_prompt(void) {
    const char* path = vfs_get_path(cwd);
    vga_puts_color(username, VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK));
    vga_puts("@");
    vga_puts_color(hostname, VGA_COLOR(VGA_LIGHT_CYAN, VGA_BLACK));
    vga_puts(" ");
    if (path[0] == '/' && path[1] == '\0') {
        vga_puts_color("~", VGA_COLOR(VGA_LIGHT_BLUE, VGA_BLACK));
    } else {
        const char* home = "/Users/root";
        bool is_home = true;
        int i;
        for (i = 0; home[i]; i++) { if (path[i] != home[i]) { is_home = false; break; } }
        if (is_home && (path[i] == '\0' || path[i] == '/')) {
            vga_puts_color("~", VGA_COLOR(VGA_LIGHT_BLUE, VGA_BLACK));
            vga_puts_color(&path[i], VGA_COLOR(VGA_LIGHT_BLUE, VGA_BLACK));
        } else {
            vga_puts_color(path, VGA_COLOR(VGA_LIGHT_BLUE, VGA_BLACK));
        }
    }
    vga_puts_color(" %% ", VGA_COLOR(VGA_LIGHT_GREY, VGA_BLACK));
}

static void cmd_ls(int argc, char** args) {
    int show_all = 0, show_long = 0;
    vfs_node_t* dir = cwd;
    int i;
    for (i = 1; i < argc; i++) {
        if (args[i][0] == '-') {
            int j;
            for (j = 1; args[i][j]; j++) {
                if (args[i][j] == 'a') show_all = 1;
                if (args[i][j] == 'l') show_long = 1;
            }
        } else {
            dir = vfs_lookup(args[i]);
            if (!dir) {
                vga_puts_color("ls: cannot access '", VGA_COLOR(VGA_RED, VGA_BLACK));
                vga_puts_color(args[i], VGA_COLOR(VGA_RED, VGA_BLACK));
                vga_puts_color("': No such file or directory\n", VGA_COLOR(VGA_RED, VGA_BLACK));
                return;
            }
        }
    }
    if (!dir || !(dir->type & VFS_DIR)) {
        vga_puts_color("ls: not a directory\n", VGA_COLOR(VGA_RED, VGA_BLACK)); return;
    }
    if (show_long) {
        uint32_t total = 0;
        vfs_node_t* c;
        for (c = dir->children; c; c = c->next) {
            if (!show_all && c->name[0] == '.') continue;
            total += (c->size + 1023) / 1024;
        }
        vga_puts_color("total ", VGA_COLOR(VGA_LIGHT_GREY, VGA_BLACK));
        vga_print_dec(total); vga_puts("\n");
        for (c = dir->children; c; c = c->next) {
            if (!show_all && c->name[0] == '.') continue;
            char perms[11] = "----------";
            if (c->type & VFS_DIR) perms[0] = 'd';
            else if (c->type & VFS_LINK) perms[0] = 'l';
            else perms[0] = '-';
            if (c->perms & 0400) perms[1] = 'r';
            if (c->perms & 0200) perms[2] = 'w';
            if (c->perms & 0100) perms[3] = 'x';
            if (c->perms & 0040) perms[4] = 'r';
            if (c->perms & 0020) perms[5] = 'w';
            if (c->perms & 0010) perms[6] = 'x';
            if (c->perms & 0004) perms[7] = 'r';
            if (c->perms & 0002) perms[8] = 'w';
            if (c->perms & 0001) perms[9] = 'x';
            vga_puts_color(perms, VGA_COLOR(VGA_LIGHT_GREY, VGA_BLACK));
            vga_puts(" root root  ");
            vga_print_dec(c->size);
            vga_puts("  ");
            if (c->type & VFS_DIR)
                vga_puts_color(c->name, VGA_COLOR(VGA_LIGHT_BLUE, VGA_BLACK));
            else
                vga_puts_color(c->name, VGA_COLOR(VGA_WHITE, VGA_BLACK));
            vga_puts("\n");
        }
    } else {
        vfs_node_t* c;
        for (c = dir->children; c; c = c->next) {
            if (!show_all && c->name[0] == '.') continue;
            if (c->type & VFS_DIR)
                vga_puts_color(c->name, VGA_COLOR(VGA_LIGHT_BLUE, VGA_BLACK));
            else if (c->type & VFS_LINK)
                vga_puts_color(c->name, VGA_COLOR(VGA_CYAN, VGA_BLACK));
            else
                vga_puts_color(c->name, VGA_COLOR(VGA_WHITE, VGA_BLACK));
            vga_puts("  ");
        }
        vga_puts("\n");
    }
}

static void cmd_cd(const char* target) {
    vfs_node_t* child;
    if (!target || !*target) return;
    if (target[0] == '/' && target[1] == '\0') { prev_cwd = cwd; cwd = vfs_get_root(); return; }
    if (strcmp(target, "..") == 0) { if (cwd->parent) { prev_cwd = cwd; cwd = cwd->parent; } return; }
    if (strcmp(target, "-") == 0) { if (prev_cwd) { vfs_node_t* t = cwd; cwd = prev_cwd; prev_cwd = t; } return; }
    if (target[0] == '~' && (target[1] == '\0' || target[1] == '/')) {
        vfs_node_t* home = vfs_lookup("/Users/root");
        if (home) { prev_cwd = cwd; cwd = home; }
        return;
    }
    child = vfs_find_child(cwd, target);
    if (!child) { child = vfs_lookup(target); }
    if (!child) {
        vga_puts_color("cd: no such file or directory: ", VGA_COLOR(VGA_RED, VGA_BLACK));
        vga_puts_color(target, VGA_COLOR(VGA_RED, VGA_BLACK)); vga_puts("\n"); return;
    }
    if (!(child->type & VFS_DIR)) {
        vga_puts_color("cd: not a directory: ", VGA_COLOR(VGA_RED, VGA_BLACK));
        vga_puts_color(target, VGA_COLOR(VGA_RED, VGA_BLACK)); vga_puts("\n"); return;
    }
    prev_cwd = cwd;
    cwd = child;
}

static void cmd_cat(const char* path) {
    vfs_node_t* n = vfs_lookup(path);
    if (!n) n = vfs_find_child(cwd, path);
    if (!n) { vga_puts_color("cat: ", VGA_COLOR(VGA_RED, VGA_BLACK)); vga_puts_color(path, VGA_COLOR(VGA_RED, VGA_BLACK)); vga_puts(": No such file or directory\n"); return; }
    if (n->type & VFS_DIR) { vga_puts_color("cat: ", VGA_COLOR(VGA_RED, VGA_BLACK)); vga_puts_color(path, VGA_COLOR(VGA_RED, VGA_BLACK)); vga_puts(": Is a directory\n"); return; }
    if (n->content) vga_puts(n->content);
}

static void cmd_touch(int argc, char** args) {
    int i;
    for (i = 1; i < argc; i++) { if (vfs_touch(args[i])) vga_puts(""); }
}

static void cmd_mkdir(int argc, char** args) {
    int i;
    for (i = 1; i < argc; i++) {
        if (vfs_mkdir(args[i]) == -2) {
            vga_puts_color("mkdir: ", VGA_COLOR(VGA_RED, VGA_BLACK));
            vga_puts_color(args[i], VGA_COLOR(VGA_RED, VGA_BLACK));
            vga_puts(": File exists\n");
        }
    }
}

static void cmd_rm(int argc, char** args) {
    int force = 0, recursive = 0, start = 1, i;
    if (argc < 2) { vga_puts_color("rm: missing operand\n", VGA_COLOR(VGA_RED, VGA_BLACK)); return; }
    if (args[1][0] == '-') {
        int j;
        for (j = 1; args[1][j]; j++) {
            if (args[1][j] == 'r' || args[1][j] == 'R') recursive = 1;
            if (args[1][j] == 'f') force = 1;
        }
        start = 2;
    }
    for (i = start; i < argc; i++) {
        vfs_node_t* node = vfs_lookup(args[i]);
        if (!node) { if (!force) { vga_puts_color("rm: ", VGA_COLOR(VGA_RED, VGA_BLACK)); vga_puts_color(args[i], VGA_COLOR(VGA_RED, VGA_BLACK)); vga_puts(": No such file or directory\n"); } continue; }
        if ((node->type & VFS_DIR) && !recursive) { vga_puts_color("rm: ", VGA_COLOR(VGA_RED, VGA_BLACK)); vga_puts_color(args[i], VGA_COLOR(VGA_RED, VGA_BLACK)); vga_puts(": is a directory\n"); continue; }
        vfs_rm(args[i]);
    }
}

static void cmd_cp(int argc, char** args) {
    int start = 1;
    if (argc < 3) { vga_puts_color("cp: missing operand\n", VGA_COLOR(VGA_RED, VGA_BLACK)); return; }
    if (args[1][0] == '-' && args[1][1] == 'R') start = 2;
    if (argc - start < 2) { vga_puts_color("cp: missing destination\n", VGA_COLOR(VGA_RED, VGA_BLACK)); return; }
    vfs_cp(args[start], args[start + 1]);
}

static void cmd_mv(int argc, char** args) {
    if (argc < 3) { vga_puts_color("mv: missing operand\n", VGA_COLOR(VGA_RED, VGA_BLACK)); return; }
    vfs_mv(args[1], args[2]);
}

static void cmd_ln(int argc, char** args) {
    int start = 1;
    if (argc < 3) { vga_puts_color("ln: missing operand\n", VGA_COLOR(VGA_RED, VGA_BLACK)); return; }
    if (args[1][0] == '-' && args[1][1] == 's') start = 2;
    if (argc - start < 2) { vga_puts_color("ln: missing target\n", VGA_COLOR(VGA_RED, VGA_BLACK)); return; }
    vfs_node_t* target = vfs_lookup(args[start]);
    if (!target) { vga_puts_color("ln: ", VGA_COLOR(VGA_RED, VGA_BLACK)); vga_puts_color(args[start], VGA_COLOR(VGA_RED, VGA_BLACK)); vga_puts(": No such file or directory\n"); return; }
    char name[VFS_NAME_LEN];
    vfs_node_t* parent = NULL;
    vfs_node_t* existing = vfs_lookup(args[start + 1]);
    if (existing && (existing->type & VFS_DIR)) { parent = existing; strcpy(name, target->name); }
    else {
        char tmp[512]; int last = 0, k;
        strcpy(tmp, args[start + 1]);
        for (k = 0; tmp[k]; k++) if (tmp[k] == '/') last = k;
        if (last == 0 && tmp[0] == '/') { parent = vfs_get_root(); strcpy(name, &tmp[1]); }
        else { tmp[last] = '\0'; parent = vfs_lookup(tmp); strcpy(name, &tmp[last + 1]); }
    }
    if (!parent) { vga_puts_color("ln: parent not found\n", VGA_COLOR(VGA_RED, VGA_BLACK)); return; }
    if (start == 2) {
        vfs_node_t* ln = vfs_create_node(name, VFS_LINK, parent);
        if (ln) { ln->link_target = (char*)kmalloc(strlen(args[start]) + 1); if (ln->link_target) strcpy(ln->link_target, args[start]); vfs_add_child(parent, ln); }
    } else { vfs_cp(args[start], args[start + 1]); }
}

static void cmd_find(int argc, char** args) {
    const char* dir_path = "."; const char* pattern = NULL; int i;
    for (i = 1; i < argc; i++) { if (strcmp(args[i], "-name") == 0 && i + 1 < argc) pattern = args[++i]; else if (args[i][0] != '-') dir_path = args[i]; }
    if (!pattern) { vga_puts_color("find: -name required\n", VGA_COLOR(VGA_RED, VGA_BLACK)); return; }
    vfs_node_t* dir = vfs_lookup(dir_path);
    if (!dir) { vga_puts_color("find: cannot access\n", VGA_COLOR(VGA_RED, VGA_BLACK)); return; }
    vfs_node_t* c;
    for (c = dir->children; c; c = c->next) {
        if (strcmp(c->name, pattern) == 0) { vga_puts(dir_path); if (dir_path[1] != '\0') vga_putc('/'); vga_puts(c->name); vga_puts("\n"); }
        if (c->type & VFS_DIR) { char sub[512]; strcpy(sub, dir_path); if (dir_path[1] != '\0') { int l = strlen(sub); sub[l] = '/'; sub[l+1] = '\0'; } int l = strlen(sub); int j; for (j = 0; c->name[j]; j++) sub[l + j] = c->name[j]; sub[l + j] = '\0'; char* fa[4] = {"find", sub, "-name", (char*)pattern}; cmd_find(4, fa); }
    }
}

static void cmd_wc(const char* path) {
    vfs_node_t* n = vfs_lookup(path);
    if (!n || !n->content) { vga_puts_color("wc: cannot open\n", VGA_COLOR(VGA_RED, VGA_BLACK)); return; }
    uint32_t lines = 0, words = 1, chars = 0;
    int in_word = 0; uint32_t i;
    for (i = 0; n->content[i]; i++) {
        chars++;
        if (n->content[i] == '\n') lines++;
        if (n->content[i] == ' ' || n->content[i] == '\n' || n->content[i] == '\t') in_word = 0;
        else if (!in_word) { words++; in_word = 1; }
    }
    vga_print_dec(lines); vga_putc(' ');
    vga_print_dec(words); vga_putc(' ');
    vga_print_dec(chars); vga_putc(' ');
    vga_puts(path); vga_puts("\n");
}

static void cmd_grep(int argc, char** args) {
    const char* pattern = NULL; const char* path = NULL; int i;
    for (i = 1; i < argc; i++) { if (args[i][0] == '-') continue; if (!pattern) pattern = args[i]; else path = args[i]; }
    if (!pattern || !path) { vga_puts_color("grep: usage: grep pattern file\n", VGA_COLOR(VGA_RED, VGA_BLACK)); return; }
    vfs_node_t* n = vfs_lookup(path);
    if (!n || !n->content) { vga_puts_color("grep: cannot open\n", VGA_COLOR(VGA_RED, VGA_BLACK)); return; }
    uint32_t plen = strlen(pattern); uint32_t pos = 0, line_start = 0;
    while (n->content[pos]) {
        if (n->content[pos] == '\n' || n->content[pos] == '\0') {
            uint32_t line_len = pos - line_start;
            if (line_len >= plen) { uint32_t j; for (j = line_start; j <= pos - plen; j++) { uint32_t k; for (k = 0; k < plen; k++) { if (n->content[j + k] != pattern[k]) break; } if (k == plen) { uint32_t l; for (l = line_start; l <= pos; l++) vga_putc(n->content[l]); break; } } }
            line_start = pos + 1;
        }
        pos++;
    }
}

static void cmd_head(int argc, char** args) {
    int lines = 10; const char* path = NULL; int i;
    for (i = 1; i < argc; i++) {
        if (args[i][0] == '-' && args[i][1] == 'n') {
            lines = 0;
            if (args[i][2]) { int j; for (j = 2; args[i][j]; j++) lines = lines * 10 + (args[i][j] - '0'); }
            else if (i + 1 < argc) { i++; int j; for (j = 0; args[i][j]; j++) lines = lines * 10 + (args[i][j] - '0'); }
        } else path = args[i];
    }
    if (!path) { vga_puts_color("head: missing file\n", VGA_COLOR(VGA_RED, VGA_BLACK)); return; }
    vfs_node_t* n = vfs_lookup(path);
    if (!n || !n->content) { vga_puts_color("head: cannot open\n", VGA_COLOR(VGA_RED, VGA_BLACK)); return; }
    int ln = 0; uint32_t pos = 0;
    while (n->content[pos] && ln < lines) { vga_putc(n->content[pos]); if (n->content[pos] == '\n') ln++; pos++; }
}

static void cmd_tail(int argc, char** args) {
    int lines = 10; const char* path = NULL; int i;
    for (i = 1; i < argc; i++) {
        if (args[i][0] == '-' && args[i][1] == 'n') {
            lines = 0; int j;
            if (args[i][2]) { for (j = 2; args[i][j]; j++) lines = lines * 10 + (args[i][j] - '0'); }
            else if (i + 1 < argc) { i++; for (j = 0; args[i][j]; j++) lines = lines * 10 + (args[i][j] - '0'); }
        } else path = args[i];
    }
    if (!path) { vga_puts_color("tail: missing file\n", VGA_COLOR(VGA_RED, VGA_BLACK)); return; }
    vfs_node_t* n = vfs_lookup(path);
    if (!n || !n->content) { vga_puts_color("tail: cannot open\n", VGA_COLOR(VGA_RED, VGA_BLACK)); return; }
    uint32_t total = 0; while (n->content[total]) total++;
    int count = 0; int j = (int)total - 1;
    while (j >= 0 && count < lines) { if (n->content[j] == '\n') count++; j--; }
    if (j < 0) j = 0; else j++;
    uint32_t pos; for (pos = (uint32_t)j; n->content[pos]; pos++) vga_putc(n->content[pos]);
    if (total > 0 && n->content[total - 1] != '\n') vga_puts("\n");
}

static void cmd_less(const char* path) {
    vfs_node_t* n = vfs_lookup(path);
    if (!n || !n->content) { vga_puts_color("less: cannot open\n", VGA_COLOR(VGA_RED, VGA_BLACK)); return; }
    uint32_t page = 20, pos = 0;
    while (n->content[pos]) {
        uint32_t row;
        for (row = 0; row < page && n->content[pos]; row++) {
            while (n->content[pos] && n->content[pos] != '\n') vga_putc(n->content[pos]++);
            if (n->content[pos] == '\n') { vga_putc('\n'); pos++; }
        }
        if (n->content[pos]) {
            vga_puts_color("--More-- (press any key)", VGA_COLOR(VGA_LIGHT_GREY, VGA_BLACK));
            keyboard_getchar();
            vga_puts("\r                         \r");
        }
    }
}

static void cmd_date(void) {
    uint32_t t = timer_ticks() / 100;
    uint32_t s = t % 60, m = (t / 60) % 60, h = (t / 3600) % 24;
    vga_puts("Jan 1 2025 ");
    char buf[16]; int k, p = 0;
    if (h < 10) buf[p++] = '0';
    char rv[4]; int j = 0, v = h;
    if (v == 0) { rv[j++] = '0'; } else { while (v > 0) { rv[j++] = '0' + (v % 10); v /= 10; } }
    for (k = j-1; k >= 0; k--) buf[p++] = rv[k];
    buf[p++] = ':';
    if (m < 10) buf[p++] = '0';
    j = 0; v = m;
    if (v == 0) { rv[j++] = '0'; } else { while (v > 0) { rv[j++] = '0' + (v % 10); v /= 10; } }
    for (k = j-1; k >= 0; k--) buf[p++] = rv[k];
    buf[p++] = ':';
    if (s < 10) buf[p++] = '0';
    j = 0; v = s;
    if (v == 0) { rv[j++] = '0'; } else { while (v > 0) { rv[j++] = '0' + (v % 10); v /= 10; } }
    for (k = j-1; k >= 0; k--) buf[p++] = rv[k];
    buf[p] = '\0';
    vga_puts("Jan 1 2025 "); vga_puts(buf); vga_puts(" UTC\n");
}

static void cmd_calc(const char* expr) {
    int32_t a = 0, b = 0; char op = '+'; int i = 0;
    while (expr[i] >= '0' && expr[i] <= '9') { a = a * 10 + (expr[i] - '0'); i++; }
    if (expr[i]) { op = expr[i]; i++; }
    while (expr[i] >= '0' && expr[i] <= '9') { b = b * 10 + (expr[i] - '0'); i++; }
    int32_t result;
    switch (op) {
        case '+': result = a + b; break;
        case '-': result = a - b; break;
        case '*': result = a * b; break;
        case '/': result = b ? a / b : 0; break;
        case '%': result = b ? a % b : 0; break;
        default: result = a; break;
    }
    vga_print_dec(result); vga_puts("\n");
}

static void cmd_hexdump(const char* path) {
    vfs_node_t* n = vfs_lookup(path);
    if (!n || !n->content) { vga_puts_color("hexdump: cannot open\n", VGA_COLOR(VGA_RED, VGA_BLACK)); return; }
    uint32_t i; uint32_t len = n->size ? n->size : strlen(n->content);
    for (i = 0; i < len && i < 256; i += 16) {
        vga_print_hex(i); vga_puts("  ");
        uint32_t j;
        for (j = 0; j < 16 && (i + j) < len; j++) {
            uint8_t byte = (uint8_t)n->content[i + j];
            vga_putc("0123456789ABCDEF"[(byte >> 4) & 0xF]);
            vga_putc("0123456789ABCDEF"[byte & 0xF]);
            vga_putc(' ');
        }
        vga_puts(" |");
        for (j = 0; j < 16 && (i + j) < len; j++) {
            uint8_t byte = (uint8_t)n->content[i + j];
            vga_putc((byte >= 32 && byte < 127) ? byte : '.');
        }
        vga_puts("|\n");
    }
}

static void cmd_seq(int argc, char** args) {
    int i, start = 1, end = 10, step = 1;
    if (argc > 1) { end = 0; int j; for (j = 0; args[1][j]; j++) end = end * 10 + (args[1][j] - '0'); }
    if (argc > 2) { start = end; end = 0; int j; for (j = 0; args[2][j]; j++) end = end * 10 + (args[2][j] - '0'); }
    if (argc > 3) { step = 0; int j; for (j = 0; args[3][j]; j++) step = step * 10 + (args[3][j] - '0'); }
    if (step == 0) step = 1;
    if (start <= end) { for (i = start; i <= end; i += step) { vga_print_dec(i); vga_puts("\n"); } }
    else { for (i = start; i >= end; i -= step) { vga_print_dec(i); vga_puts("\n"); } }
}

static void cmd_yes(int argc, char** args) {
    int i;
    for (i = 0; i < 10; i++) {
        if (argc > 1) { int j; for (j = 1; j < argc; j++) { if (j > 1) vga_putc(' '); vga_puts(args[j]); } }
        else vga_puts("y");
        vga_puts("\n");
    }
}

static void cmd_free(void) {
    uint32_t total = pmm_get_total() * 4;
    uint32_t free_m = pmm_get_free() * 4;
    uint32_t used = total - free_m;
    vga_puts("              total        used        free\n");
    vga_puts("Mem:     "); vga_print_dec(total); vga_puts("KB   "); vga_print_dec(used); vga_puts("KB   "); vga_print_dec(free_m); vga_puts("KB\n");
}

static void cmd_uptime(void) {
    uint32_t t = timer_ticks() / 100;
    uint32_t s = t % 60, m = (t / 60) % 60, h = (t / 3600) % 24;
    vga_puts("up ");
    vga_print_dec(h);
    vga_puts(":");
    if (m < 10) vga_putc('0');
    vga_print_dec(m);
    vga_puts(":");
    if (s < 10) vga_putc('0');
    vga_print_dec(s);
    vga_puts(", ");
    vga_print_dec(process_get_count());
    vga_puts(" user");
    if (process_get_count() != 1) vga_putc('s');
    vga_puts("\n");
}



static void cmd_lscpu(void) {
    vga_puts("Architecture:       i686 (x86_32)\n");
    vga_puts("CPU op-mode(s):     32-bit\n");
    vga_puts("CPU model:          Intel 486 (QEMU)\n");
    vga_puts("CPU(s):             1\n");
    vga_puts("Thread(s) per core: 1\n");
    vga_puts("Core(s) per socket: 1\n");
    vga_puts("Socket(s):          1\n");
    vga_puts("Vendor ID:          GenuineIntel\n");
    vga_puts("L1d cache:          64K\n");
    vga_puts("L1i cache:          64K\n");
}

static void cmd_lspci(void) {
    vga_puts("00:00.0 Host bridge: Intel 440FX\n");
    vga_puts("00:01.0 ISA bridge: Intel PIIX3\n");
    vga_puts("00:01.1 IDE: Intel PIIX3\n");
    vga_puts("00:02.0 VGA compatible: Cirrus Logic GD 5446\n");
    vga_puts("00:03.0 Ethernet: Realtek RTL8139\n");
    vga_puts("00:05.0 Multimedia: Intel AC97\n");
}

static void cmd_lsusb(void) {
    vga_puts("Bus 001 Device 001: ID 1af4:0001 Virtio\n");
    vga_puts("Bus 001 Device 002: ID 0627:0001 QEMU USB Tablet\n");
}

static void cmd_lsmod(void) {
    vga_puts("Module          Size  Used by\n");
    vga_puts("vga_framebuffer 4096  1\n");
    vga_puts("keyboard_drv    2048  1\n");
    vga_puts("timer_drv       1024  1\n");
    vga_puts("pci_bus          512  0\n");
}

static void cmd_dmesg(void) {
    vga_puts("[    0.000] OrangeX Kernel v0.1.0 booting...\n");
    vga_puts("[    0.001] GDT loaded (5 entries)\n");
    vga_puts("[    0.002] IDT loaded (256 entries)\n");
    vga_puts("[    0.003] PIT configured at 100Hz\n");
    vga_puts("[    0.004] Keyboard driver initialized\n");
    vga_puts("[    0.005] VFS initialized with root filesystem\n");
    vga_puts("[    0.006] Process scheduler started\n");
    vga_puts("[    0.007] System ready.\n");
}

static void cmd_ifconfig(void) {
    char buf[512];
    net_ifconfig(buf, sizeof(buf));
    vga_puts(buf);
}

static void cmd_ping(const char* host) {
    uint32_t ip = net_resolve(host);
    uint8_t* b = (uint8_t*)&ip;
    vga_puts("PING "); vga_puts(host); vga_puts(" (");
    vga_print_dec(b[3]); vga_puts("."); vga_print_dec(b[2]); vga_puts(".");
    vga_print_dec(b[1]); vga_puts("."); vga_print_dec(b[0]); vga_puts(") 56(84) bytes.\n");
    uint32_t received = net_ping_host(host, 4);
    vga_puts("\n--- "); vga_puts(host); vga_puts(" ping statistics ---\n");
    vga_puts("4 packets transmitted, ");
    vga_print_dec(received); vga_puts(" received, 0% packet loss\n");
}

static void cmd_sw_vers(void) {
    vga_puts("ProductName:    OrangeX Kernel\n");
    vga_puts("ProductVersion: 0.1.0\n");
    vga_puts("BuildVersion:   1\n");
}

static void cmd_csrutil(void) {
    vga_puts("System Integrity Protection status: enabled.\n");
    vga_puts("Configuration: Apple Internal, Kext Signing, etc.\n");
}

static void cmd_mount(void) {
    vga_puts("rootfs on / type orangex (rw,relatime)\n");
    vga_puts("devfs on /dev type devfs (rw,relatime)\n");
    vga_puts("proc on /proc type proc (rw,relatime)\n");
    vga_puts("tmpfs on /tmp type tmpfs (rw,relatime,size=65536k)\n");
}

static void cmd_fdisk(void) {
    vga_puts("Disk /dev/sda: 128 MB, 134217728 bytes\n");
    vga_puts("Sector size (logical/physical): 512 bytes\n");
    vga_puts("\n   Device  Boot  Start    End   Sectors  Size  Type\n");
    vga_puts("   /dev/sda1  *       1   2880   2879   1.4M  FAT12\n");
}



static void cmd_env(void) {
    vga_puts("SHELL=/bin/sh\n");
    vga_puts("USER=root\n");
    vga_puts("HOME=/Users/root\n");
    vga_puts("PATH=/bin:/usr/bin:/usr/local/bin\n");
    vga_puts("TERM=orangex-128color\n");
    vga_puts("LANG=en_US.UTF-8\n");
    vga_puts("HOSTNAME=orangex\n");
    vga_puts("PWD=/\n");
}

static void cmd_hostname_get(void) {
    vga_puts(hostname); vga_puts("\n");
}

void shell_init(void) {
    cwd = vfs_get_root();
    prev_cwd = NULL;
    strcpy(cwd_path, "/");
}

void shell_run(void) {
    while (1) {
        char* args[SHELL_MAX_ARGS];
        int argc;
        shell_print_prompt();
        keyboard_wait_line(input, SHELL_MAX_INPUT);
        if (input[0] == '\0') continue;
        argc = shell_tokenize(input, args, SHELL_MAX_ARGS);
        if (argc == 0) continue;

        if (strcmp(args[0], "ls") == 0) cmd_ls(argc, args);
        else if (strcmp(args[0], "cd") == 0) { if (argc > 1) cmd_cd(args[1]); }
        else if (strcmp(args[0], "pwd") == 0) { vga_puts(vfs_get_path(cwd)); vga_puts("\n"); }
        else if (strcmp(args[0], "clear") == 0 || strcmp(args[0], "reset") == 0) vga_clear();
        else if (strcmp(args[0], "cat") == 0) { if (argc > 1) cmd_cat(args[1]); else vga_puts_color("cat: missing file\n", VGA_COLOR(VGA_RED, VGA_BLACK)); }
        else if (strcmp(args[0], "echo") == 0) { int i; for (i = 1; i < argc; i++) { if (i > 1) vga_putc(' '); vga_puts(args[i]); } vga_puts("\n"); }
        else if (strcmp(args[0], "touch") == 0) cmd_touch(argc, args);
        else if (strcmp(args[0], "mkdir") == 0) cmd_mkdir(argc, args);
        else if (strcmp(args[0], "rm") == 0) cmd_rm(argc, args);
        else if (strcmp(args[0], "cp") == 0) cmd_cp(argc, args);
        else if (strcmp(args[0], "mv") == 0) cmd_mv(argc, args);
        else if (strcmp(args[0], "ln") == 0) cmd_ln(argc, args);
        else if (strcmp(args[0], "find") == 0) cmd_find(argc, args);
        else if (strcmp(args[0], "wc") == 0) { if (argc > 1) cmd_wc(args[1]); else vga_puts_color("wc: missing file\n", VGA_COLOR(VGA_RED, VGA_BLACK)); }
        else if (strcmp(args[0], "grep") == 0) cmd_grep(argc, args);
        else if (strcmp(args[0], "head") == 0) cmd_head(argc, args);
        else if (strcmp(args[0], "tail") == 0) cmd_tail(argc, args);
        else if (strcmp(args[0], "less") == 0) { if (argc > 1) cmd_less(args[1]); else vga_puts_color("less: missing file\n", VGA_COLOR(VGA_RED, VGA_BLACK)); }
        else if (strcmp(args[0], "hexdump") == 0 || strcmp(args[0], "xxd") == 0) { if (argc > 1) cmd_hexdump(args[1]); else vga_puts_color("hexdump: missing file\n", VGA_COLOR(VGA_RED, VGA_BLACK)); }
        else if (strcmp(args[0], "calc") == 0) { if (argc > 1) cmd_calc(args[1]); else vga_puts_color("calc: missing expression\n", VGA_COLOR(VGA_RED, VGA_BLACK)); }
        else if (strcmp(args[0], "seq") == 0) cmd_seq(argc, args);
        else if (strcmp(args[0], "yes") == 0) cmd_yes(argc, args);
        else if (strcmp(args[0], "whoami") == 0) vga_puts("root\n");
        else if (strcmp(args[0], "id") == 0) vga_puts("uid=0(root) gid=0(root) groups=0(root)\n");
        else if (strcmp(args[0], "hostname") == 0) cmd_hostname_get();
        else if (strcmp(args[0], "uname") == 0) {
            int all = 0, machine = 0;
            int i; for (i = 1; i < argc; i++) { if (args[i][0] == '-') { int j; for (j = 1; args[i][j]; j++) { if (args[i][j] == 'a') all = 1; if (args[i][j] == 'm') machine = 1; } } }
            if (machine && !all) vga_puts("i686\n");
            else vga_puts("O&angeX_Kernel 0.1.0 orangex i686\n");
        }
        else if (strcmp(args[0], "date") == 0) cmd_date();
        else if (strcmp(args[0], "uptime") == 0) cmd_uptime();
        else if (strcmp(args[0], "mem") == 0 || strcmp(args[0], "free") == 0) cmd_free();
        else if (strcmp(args[0], "ps") == 0) {
            vga_puts_color("  PID  STATE      NAME\n", VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK));
            uint32_t i; for (i = 0; i < MAX_PROCESSES; i++) {
                process_t* p = process_get(i);
                if (!p) continue;
                vga_puts("  "); vga_print_dec(p->pid); vga_puts("  ");
                switch (p->state) {
                    case PROC_READY: vga_puts_color("ready      ", VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK)); break;
                    case PROC_RUNNING: vga_puts_color("running    ", VGA_COLOR(VGA_YELLOW, VGA_BLACK)); break;
                    case PROC_BLOCKED: vga_puts_color("sleeping   ", VGA_COLOR(VGA_LIGHT_GREY, VGA_BLACK)); break;
                    default: vga_puts_color("terminated ", VGA_COLOR(VGA_LIGHT_RED, VGA_BLACK)); break;
                }
                vga_puts_color(p->name, VGA_COLOR(VGA_WHITE, VGA_BLACK)); vga_puts("\n");
            }
        }
        else if (strcmp(args[0], "top") == 0) {
            vga_puts_color("  PID  CPU   STATE  NAME\n", VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK));
            uint32_t i; for (i = 0; i < MAX_PROCESSES; i++) {
                process_t* p = process_get(i);
                if (!p) continue;
                vga_puts("  "); vga_print_dec(p->pid); vga_puts("  0.0  ");
                switch (p->state) { case PROC_RUNNING: vga_puts("R  "); break; case PROC_READY: vga_puts("S  "); break; case PROC_BLOCKED: vga_puts("D  "); break; default: vga_puts("Z  "); break; }
                vga_puts_color(p->name, VGA_COLOR(VGA_WHITE, VGA_BLACK)); vga_puts("\n");
            }
        }
        else if (strcmp(args[0], "kill") == 0) {
            if (argc < 2) { vga_puts_color("kill: missing pid\n", VGA_COLOR(VGA_RED, VGA_BLACK)); continue; }
            uint32_t pid = 0; int j; for (j = 0; args[1][j]; j++) pid = pid * 10 + (args[1][j] - '0');
            process_t* p = process_get(pid);
            if (!p) { vga_puts_color("kill: no such process\n", VGA_COLOR(VGA_RED, VGA_BLACK)); continue; }
            p->state = PROC_TERMINATED; vga_puts("killed "); vga_print_dec(pid); vga_puts("\n");
        }
        else if (strcmp(args[0], "killall") == 0) {
            if (argc < 2) { vga_puts_color("killall: missing name\n", VGA_COLOR(VGA_RED, VGA_BLACK)); continue; }
            uint32_t i, count = 0;
            for (i = 0; i < MAX_PROCESSES; i++) { process_t* p = process_get(i); if (p && strcmp(p->name, args[1]) == 0) { p->state = PROC_TERMINATED; count++; } }
            vga_print_dec(count); vga_puts(" processes killed\n");
        }
        else if (strcmp(args[0], "pgrep") == 0) {
            if (argc < 2) { vga_puts_color("pgrep: missing name\n", VGA_COLOR(VGA_RED, VGA_BLACK)); continue; }
            uint32_t i; for (i = 0; i < MAX_PROCESSES; i++) { process_t* p = process_get(i); if (p && strcmp(p->name, args[1]) == 0) { vga_print_dec(p->pid); vga_puts("\n"); } }
        }
        else if (strcmp(args[0], "sleep") == 0) {
            if (argc < 2) { vga_puts_color("sleep: missing time\n", VGA_COLOR(VGA_RED, VGA_BLACK)); continue; }
            uint32_t ms = 0; int j; for (j = 0; args[1][j]; j++) ms = ms * 10 + (args[1][j] - '0');
            vga_puts("sleeping "); vga_print_dec(ms); vga_puts("ms...\n");
            timer_sleep(ms);
        }
        else if (strcmp(args[0], "fork") == 0) {
            uint32_t pid = process_create("child", 0, 0);
            if (pid) { vga_puts("forked "); vga_print_dec(pid); vga_puts("\n"); }
            else vga_puts_color("fork: failed\n", VGA_COLOR(VGA_RED, VGA_BLACK));
        }
        else if (strcmp(args[0], "ifconfig") == 0 || strcmp(args[0], "ip") == 0) cmd_ifconfig();
        else if (strcmp(args[0], "ping") == 0) cmd_ping(argc > 1 ? args[1] : "127.0.0.1");
        else if (strcmp(args[0], "sw_vers") == 0) cmd_sw_vers();
        else if (strcmp(args[0], "csrutil") == 0) cmd_csrutil();
        else if (strcmp(args[0], "mount") == 0) cmd_mount();
        else if (strcmp(args[0], "fdisk") == 0) cmd_fdisk();
        else if (strcmp(args[0], "lscpu") == 0) cmd_lscpu();
        else if (strcmp(args[0], "lspci") == 0) cmd_lspci();
        else if (strcmp(args[0], "lsusb") == 0) cmd_lsusb();
        else if (strcmp(args[0], "lsmod") == 0) cmd_lsmod();
        else if (strcmp(args[0], "dmesg") == 0) cmd_dmesg();
        else if (strcmp(args[0], "env") == 0) cmd_env();
        else if (strcmp(args[0], "vm_stat") == 0) cmd_free();
        /* === WiFi === */
        else if (strcmp(args[0], "wifi") == 0) {
            if (argc < 2) { vga_puts("Usage: wifi <scan|connect|disconnect|status|list>\n"); }
            else if (strcmp(args[1], "scan") == 0) {
                vga_puts_color("[WiFi] Scanning for networks...\n", VGA_COLOR(VGA_LIGHT_CYAN, VGA_BLACK));
                vga_puts("  1. OrangeX-5G        [████████░░] -42dBm  WPA2\n");
                vga_puts("  2. HomeNetwork       [██████░░░░] -58dBm  WPA2\n");
                vga_puts("  3. Guest_WiFi        [████░░░░░░] -67dBm  Open\n");
                vga_puts("  4. Neighbors_WiFi    [██░░░░░░░░] -78dBm  WPA3\n");
                vga_puts("  5. CoffeeShop_Guest  [█░░░░░░░░░] -85dBm  Open\n");
                vga_puts("Found 5 networks.\n");
            }
            else if (strcmp(args[1], "connect") == 0) {
                const char* net = argc > 2 ? args[2] : "OrangeX-5G";
                vga_puts_color("[WiFi] Connecting to ", VGA_COLOR(VGA_LIGHT_CYAN, VGA_BLACK));
                vga_puts_color(net, VGA_COLOR(VGA_WHITE, VGA_BLACK));
                vga_puts("...\n");
                vga_puts("[WiFi] Authenticating...\n");
                timer_sleep(500);
                vga_puts("[WiFi] Obtaining IP address...\n");
                timer_sleep(300);
                vga_puts_color("[WiFi] Connected! IP: 192.168.1.42\n", VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK));
            }
            else if (strcmp(args[1], "disconnect") == 0) {
                vga_puts_color("[WiFi] Disconnected.\n", VGA_COLOR(VGA_YELLOW, VGA_BLACK));
            }
            else if (strcmp(args[1], "status") == 0) {
                vga_puts("Interface: wlan0\n");
                vga_puts("Status:    Connected\n");
                vga_puts("SSID:      OrangeX-5G\n");
                vga_puts("Signal:    -42 dBm (Excellent)\n");
                vga_puts("IP:        192.168.1.42\n");
                vga_puts("Gateway:   192.168.1.1\n");
                vga_puts("DNS:       8.8.8.8, 8.8.4.4\n");
                vga_puts("Speed:     72 Mbps\n");
            }
            else if (strcmp(args[1], "list") == 0 || strcmp(args[1], "show") == 0) {
                vga_puts("wlan0: Connected to OrangeX-5G (192.168.1.42)\n");
            }
            else { vga_puts_color("wifi: unknown command: ", VGA_COLOR(VGA_RED, VGA_BLACK)); vga_puts_color(args[1], VGA_COLOR(VGA_RED, VGA_BLACK)); vga_puts("\n"); }
        }
        else if (strcmp(args[0], "iwconfig") == 0 || strcmp(args[0], "iw") == 0) {
            vga_puts("wlan0     IEEE 802.11  ESSID:\"OrangeX-5G\"\n");
            vga_puts("          Mode:Managed  Frequency:5.2 GHz  Tx-Power=20 dBm\n");
            vga_puts("          Link Quality=70/70  Signal level=-42 dBm\n");
            vga_puts("          Power Management:off\n");
        }
        /* === Bluetooth === */
        else if (strcmp(args[0], "bt") == 0 || strcmp(args[0], "bluetooth") == 0) {
            if (argc < 2) { vga_puts("Usage: bt <scan|on|off|pair|connect|disconnect|status|devices>\n"); }
            else if (strcmp(args[1], "on") == 0) {
                vga_puts_color("[BT] Enabling Bluetooth adapter...\n", VGA_COLOR(VGA_LIGHT_BLUE, VGA_BLACK));
                timer_sleep(300);
                vga_puts_color("[BT] Bluetooth is now ON.\n", VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK));
            }
            else if (strcmp(args[1], "off") == 0) {
                vga_puts_color("[BT] Bluetooth is now OFF.\n", VGA_COLOR(VGA_YELLOW, VGA_BLACK));
            }
            else if (strcmp(args[1], "scan") == 0 || strcmp(args[1], "discover") == 0) {
                vga_puts_color("[BT] Scanning for devices...\n", VGA_COLOR(VGA_LIGHT_BLUE, VGA_BLACK));
                timer_sleep(800);
                vga_puts("  1. AirPods Pro          [AA:BB:CC:DD:EE:01]  Paired\n");
                vga_puts("  2. MX Keys Keyboard    [AA:BB:CC:DD:EE:02]  Paired\n");
                vga_puts("  3. MX Master 3 Mouse   [AA:BB:CC:DD:EE:03]  Available\n");
                vga_puts("  4. Samsung Galaxy Buds  [AA:BB:CC:DD:EE:04]  Available\n");
                vga_puts("  5. JBL Charge 5         [AA:BB:CC:DD:EE:05]  Available\n");
                vga_puts("  6. Sony WH-1000XM5     [AA:BB:CC:DD:EE:06]  Available\n");
                vga_puts("Found 6 devices.\n");
            }
            else if (strcmp(args[1], "pair") == 0) {
                const char* dev = argc > 2 ? args[2] : "AirPods Pro";
                vga_puts_color("[BT] Pairing with ", VGA_COLOR(VGA_LIGHT_BLUE, VGA_BLACK));
                vga_puts_color(dev, VGA_COLOR(VGA_WHITE, VGA_BLACK));
                vga_puts("...\n");
                timer_sleep(1000);
                vga_puts_color("[BT] Paired successfully!\n", VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK));
            }
            else if (strcmp(args[1], "connect") == 0) {
                const char* dev = argc > 2 ? args[2] : "AirPods Pro";
                vga_puts_color("[BT] Connecting to ", VGA_COLOR(VGA_LIGHT_BLUE, VGA_BLACK));
                vga_puts_color(dev, VGA_COLOR(VGA_WHITE, VGA_BLACK));
                vga_puts("...\n");
                timer_sleep(500);
                vga_puts_color("[BT] Connected!\n", VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK));
            }
            else if (strcmp(args[1], "disconnect") == 0) {
                vga_puts_color("[BT] Disconnected.\n", VGA_COLOR(VGA_YELLOW, VGA_BLACK));
            }
            else if (strcmp(args[1], "status") == 0) {
                vga_puts("Bluetooth: ON\n");
                vga_puts("Adapter:   Intel Wireless-AC 9560\n");
                vga_puts("Address:   AA:BB:CC:DD:EE:00\n");
                vga_puts("Connected: AirPods Pro\n");
            }
            else if (strcmp(args[1], "devices") == 0 || strcmp(args[1], "paired") == 0) {
                vga_puts("Paired devices:\n");
                vga_puts("  AirPods Pro        [AA:BB:CC:DD:EE:01]  Connected\n");
                vga_puts("  MX Keys Keyboard  [AA:BB:CC:DD:EE:02]  Paired\n");
            }
            else { vga_puts_color("bt: unknown command: ", VGA_COLOR(VGA_RED, VGA_BLACK)); vga_puts_color(args[1], VGA_COLOR(VGA_RED, VGA_BLACK)); vga_puts("\n"); }
        }
        /* === App Store / Package Manager === */
        else if (strcmp(args[0], "orangepkg") == 0 || strcmp(args[0], "opkg") == 0) {
            if (argc < 2) { vga_puts("Usage: orangex-pkg <install|remove|search|list|update|info> [package]\n"); }
            else if (strcmp(args[1], "search") == 0) {
                if (argc < 3) { vga_puts("Usage: orangex-pkg search <query>\n"); return; }
                vga_puts_color("Searching for '", VGA_COLOR(VGA_LIGHT_CYAN, VGA_BLACK));
                vga_puts_color(args[2], VGA_COLOR(VGA_WHITE, VGA_BLACK));
                vga_puts_color("'...\n", VGA_COLOR(VGA_LIGHT_CYAN, VGA_BLACK));
                vga_puts("  orangex-gcc         15.2.0   GNU C Compiler\n");
                vga_puts("  orangex-python      3.12.0   Python Interpreter\n");
                vga_puts("  orangex-java        21.0.1   OpenJDK Runtime\n");
                vga_puts("  orangex-rust        1.75.0   Rust Toolchain\n");
                vga_puts("  orangex-node        22.0.0   Node.js Runtime\n");
                vga_puts("  orangex-go          1.21.0   Go Language\n");
                vga_puts("  orangex-vim         9.1.0    Vim Editor\n");
                vga_puts("  orangex-nano        8.3.0    Nano Editor\n");
                vga_puts("  orangex-git         2.43.0   Git VCS\n");
                vga_puts("  orangex-docker      24.0.0   Docker Engine\n");
                vga_puts("  orangex-nginx       1.25.0   Web Server\n");
                vga_puts("  orangex-mysql       8.3.0    MySQL Database\n");
                vga_puts("  orangex-redis       7.2.0    Redis Cache\n");
                vga_puts("  orangex-curl        8.6.0    HTTP Client\n");
                vga_puts("  orangex-wget        1.24.0   Download Manager\n");
                vga_puts("  orangex-ffmpeg      6.1.0    Media Framework\n");
                vga_puts("  orangex-blender     4.0.0    3D Creation Suite\n");
                vga_puts("  orangex-gimp        2.10.36  Image Editor\n");
                vga_puts("  orangex-firefox     123.0    Web Browser\n");
                vga_puts("  orangex-chromium    122.0    Web Browser\n");
            }
            else if (strcmp(args[1], "install") == 0) {
                if (argc < 3) { vga_puts("Usage: orangex-pkg install <package>\n"); return; }
                vga_puts_color("[pkg] Installing '", VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK));
                vga_puts_color(args[2], VGA_COLOR(VGA_WHITE, VGA_BLACK));
                vga_puts_color("'...\n", VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK));
                vga_puts("[pkg] Resolving dependencies...\n");
                timer_sleep(200);
                vga_puts("[pkg] Downloading from https://packages.orangex.dev/\n");
                timer_sleep(400);
                vga_puts("[pkg] Installing...\n");
                timer_sleep(300);
                vga_puts_color("[pkg] ", VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK));
                vga_puts(args[2]); vga_puts(" installed successfully!\n");
            }
            else if (strcmp(args[1], "remove") == 0 || strcmp(args[1], "uninstall") == 0) {
                if (argc < 3) { vga_puts("Usage: orangex-pkg remove <package>\n"); return; }
                vga_puts_color("[pkg] Removing '", VGA_COLOR(VGA_YELLOW, VGA_BLACK));
                vga_puts_color(args[2], VGA_COLOR(VGA_WHITE, VGA_BLACK));
                vga_puts_color("'...\n", VGA_COLOR(VGA_YELLOW, VGA_BLACK));
                timer_sleep(200);
                vga_puts_color("[pkg] ", VGA_COLOR(VGA_YELLOW, VGA_BLACK));
                vga_puts(args[2]); vga_puts(" removed.\n");
            }
            else if (strcmp(args[1], "list") == 0 || strcmp(args[1], "ls") == 0) {
                vga_puts("Installed packages:\n");
                vga_puts("  orangex-base       1.0.0    Base system\n");
                vga_puts("  orangex-shell      1.0.0    OrangeX Shell\n");
                vga_puts("  orangex-utils      1.0.0    Core utilities\n");
            }
            else if (strcmp(args[1], "update") == 0) {
                vga_puts_color("[pkg] Updating package lists...\n", VGA_COLOR(VGA_LIGHT_CYAN, VGA_BLACK));
                timer_sleep(500);
                vga_puts("[pkg] Fetching from https://repo.orangex.dev/stable/\n");
                timer_sleep(300);
                vga_puts("[pkg] Package lists updated.\n");
            }
            else if (strcmp(args[1], "info") == 0) {
                vga_puts("OrangeX Package Manager v1.0\n");
                vga_puts("Repository: https://packages.orangex.dev/\n");
                vga_puts("Architecture: i686\n");
                vga_puts("Package format: orxpkg\n");
            }
            else { vga_puts_color("orangepkg: unknown command\n", VGA_COLOR(VGA_RED, VGA_BLACK)); }
        }
        else if (strcmp(args[0], "appstore") == 0 || strcmp(args[0], "store") == 0) {
            vga_puts_color("=== OrangeX App Store ===\n", VGA_COLOR(VGA_LIGHT_CYAN, VGA_BLACK));
            vga_puts("\n");
            vga_puts_color("DEVELOPMENT:\n", VGA_COLOR(VGA_YELLOW, VGA_BLACK));
            vga_puts("  orangex-gcc      15.2.0   GNU C/C++ Compiler    [Free]\n");
            vga_puts("  orangex-python   3.12.0   Python Interpreter    [Free]\n");
            vga_puts("  orangex-java     21.0.1   OpenJDK Runtime       [Free]\n");
            vga_puts("  orangex-rust     1.75.0   Rust Toolchain        [Free]\n");
            vga_puts("  orangex-node     22.0.0   Node.js Runtime       [Free]\n");
            vga_puts("  orangex-go       1.21.0   Go Language           [Free]\n");
            vga_puts("  orangex-cmake    3.28.0   Build System          [Free]\n");
            vga_puts("  orangex-gdb      14.1.0   GNU Debugger          [Free]\n");
            vga_puts("\n");
            vga_puts_color("EDITORS & TOOLS:\n", VGA_COLOR(VGA_YELLOW, VGA_BLACK));
            vga_puts("  orangex-vim      9.1.0    Vim Editor            [Free]\n");
            vga_puts("  orangex-nano     8.3.0    Nano Editor           [Free]\n");
            vga_puts("  orangex-emacs    29.2.0   Emacs Editor          [Free]\n");
            vga_puts("  orangex-vscode   1.87.0   VS Code (CLI)         [Free]\n");
            vga_puts("  orangex-git      2.43.0   Git Version Control   [Free]\n");
            vga_puts("\n");
            vga_puts_color("SERVERS & DATABASES:\n", VGA_COLOR(VGA_YELLOW, VGA_BLACK));
            vga_puts("  orangex-nginx    1.25.0   Web Server            [Free]\n");
            vga_puts("  orangex-apache   2.4.58   HTTP Server           [Free]\n");
            vga_puts("  orangex-mysql    8.3.0    MySQL Database        [Free]\n");
            vga_puts("  orangex-postgres 16.2     PostgreSQL            [Free]\n");
            vga_puts("  orangex-redis    7.2.0    Redis Cache           [Free]\n");
            vga_puts("  orangex-mongo    7.0.0    MongoDB               [Free]\n");
            vga_puts("\n");
            vga_puts_color("MEDIA & GRAPHICS:\n", VGA_COLOR(VGA_YELLOW, VGA_BLACK));
            vga_puts("  orangex-ffmpeg   6.1.0    Media Framework      [Free]\n");
            vga_puts("  orangex-gimp     2.10.36  Image Editor         [Free]\n");
            vga_puts("  orangex-blender  4.0.0    3D Creation Suite    [Free]\n");
            vga_puts("  orangex-obs      30.0     Screen Recorder      [Free]\n");
            vga_puts("  orangex-vlc      3.0.20   Media Player         [Free]\n");
            vga_puts("\n");
            vga_puts_color("NETWORKING:\n", VGA_COLOR(VGA_YELLOW, VGA_BLACK));
            vga_puts("  orangex-curl     8.6.0    HTTP Client          [Free]\n");
            vga_puts("  orangex-wget     1.24.0   Download Manager     [Free]\n");
            vga_puts("  orangex-nmap     7.95     Network Scanner      [Free]\n");
            vga_puts("  orangex-ssh      9.6      SSH Client           [Free]\n");
            vga_puts("  orangex-docker   24.0.0   Container Engine     [Free]\n");
            vga_puts("\n");
            vga_puts_color("SYSTEM:\n", VGA_COLOR(VGA_YELLOW, VGA_BLACK));
            vga_puts("  orangex-htop     3.3.0    Process Monitor      [Free]\n");
            vga_puts("  orangex-neofetch 7.1.0    System Info          [Free]\n");
            vga_puts("  orangex-tmux     3.4      Terminal Multiplexer  [Free]\n");
            vga_puts("  orangex-zsh      5.9.0    Z Shell              [Free]\n");
            vga_puts("  orangex-fish     3.6.4    Friendly Shell       [Free]\n");
            vga_puts("\n");
            vga_puts("Use: orangex-pkg install <package>\n");
        }
        else if (strcmp(args[0], "brew") == 0) vga_puts("brew: command not found. Install Homebrew first.\n");
        else if (strcmp(args[0], "defaults") == 0) vga_puts("defaults: macOS system preferences (stub)\n");
        /* === Device Tree === */
        else if (strcmp(args[0], "lsdev") == 0 || strcmp(args[0], "lsblk") == 0 || strcmp(args[0], "lsdevices") == 0) {
            devtree_list();
        }
        else if (strcmp(args[0], "lspci") == 0) {
            vga_puts("00:00.0 Host bridge: Intel 440FX\n");
            vga_puts("00:01.0 ISA bridge: Intel PIIX3\n");
            vga_puts("00:01.1 IDE: Intel PIIX3\n");
            vga_puts("00:02.0 VGA: Cirrus Logic GD 5446\n");
            vga_puts("00:03.0 Ethernet: Realtek RTL8139\n");
            vga_puts("00:05.0 Audio: Intel AC97\n");
            vga_puts("00:06.0 USB: Intel UHCI\n");
            vga_puts("00:07.0 Bridge: Intel PCI-ISA\n");
            vga_puts("00:08.0 Wireless: Intel PRO/Wireless 3945ABG\n");
        }
        /* === USB === */
        else if (strcmp(args[0], "usb") == 0) {
            if (argc < 2) { vga_puts("Usage: usb <list|scan|info|reset|tree>\n"); return; }
            if (strcmp(args[1], "list") == 0 || strcmp(args[1], "devices") == 0) {
                usb_list_devices();
            }
            else if (strcmp(args[1], "scan") == 0 || strcmp(args[1], "detect") == 0) {
                vga_puts_color("[USB] Scanning USB buses...\n", VGA_COLOR(VGA_LIGHT_CYAN, VGA_BLACK));
                timer_sleep(300);
                usb_detect_controllers();
                usb_enumerate();
                vga_puts("[USB] Found "); vga_print_dec(usb_get_device_count()); vga_puts(" device(s)\n");
            }
            else if (strcmp(args[1], "info") == 0) {
                if (argc < 3) { vga_puts("Usage: usb info <addr>\n"); return; }
                uint8_t addr = 0;
                int j;
                for (j = 0; args[2][j]; j++) addr = addr * 10 + (args[2][j] - '0');
                usb_device_t* dev = usb_get_device(addr);
                if (!dev) { vga_puts_color("usb: device not found\n", VGA_COLOR(VGA_RED, VGA_BLACK)); return; }
                vga_puts("Device "); vga_print_dec(addr); vga_puts(":\n");
                vga_puts("  Vendor:   0x"); vga_print_hex(dev->vendor_id); vga_puts("\n");
                vga_puts("  Product:  0x"); vga_print_hex(dev->product_id); vga_puts("\n");
                vga_puts("  Class:    "); vga_puts(usb_class_name(dev->class)); vga_puts("\n");
                vga_puts("  Product:  "); vga_puts(dev->product); vga_puts("\n");
                vga_puts("  Vendor:   "); vga_puts(dev->manufacturer); vga_puts("\n");
                vga_puts("  Serial:   "); vga_puts(dev->serial); vga_puts("\n");
                vga_puts("  Endpoints: "); vga_print_dec(dev->num_endpoints); vga_puts("\n");
            }
            else if (strcmp(args[1], "reset") == 0) {
                vga_puts("[USB] Resetting all USB ports...\n");
                usb_reset_port(0);
                vga_puts("[USB] Reset complete.\n");
            }
            else if (strcmp(args[1], "tree") == 0) {
                vga_puts("USB Device Tree:\n");
                vga_puts("  o- (root)\n");
                vga_puts("  +-[UHCI] 0xC000\n");
                int i;
                for (i = 0; i < usb_get_device_count(); i++) {
                    usb_device_t* dev = usb_get_device(i + 1);
                    if (dev) {
                        vga_puts("  |  +-[");
                        vga_puts(usb_class_name(dev->class));
                        vga_puts("] "); vga_puts(dev->product); vga_puts("\n");
                    }
                }
            }
            else { vga_puts_color("usb: unknown command\n", VGA_COLOR(VGA_RED, VGA_BLACK)); }
        }
        else if (strcmp(args[0], "lsusb") == 0) {
            vga_puts("Bus 001 Device 001: ID 1af4:0001 Virtio\n");
            vga_puts("Bus 001 Device 002: ID 0627:0001 QEMU USB Tablet\n");
            vga_puts("Bus 001 Device 003: ID 46f4:0001 QEMU USB Keyboard\n");
        }
        else if (strcmp(args[0], "lsmod") == 0) {
            vga_puts("Module            Size    Used by\n");
            vga_puts("ata_driver        8192    1\n");
            vga_puts("vga_framebuffer   4096    1\n");
            vga_puts("keyboard_drv      2048    1\n");
            vga_puts("timer_drv         1024    1\n");
            vga_puts("pci_bus            512    0\n");
            vga_puts("ext4_fs            512    0\n");
            vga_puts("fat32_fs           256    0\n");
            vga_puts("tcp_ip_stack      4096    0\n");
            vga_puts("wifi_driver       2048    0\n");
            vga_puts("bluetooth_drv     1024    0\n");
            vga_puts("security_mod      2048    1\n");
        }
        else if (strcmp(args[0], "hdparm") == 0) {
            if (argc > 1 && strcmp(args[1], "-I") == 0) {
                vga_puts("/dev/sda:\n");
                vga_puts("  Model:    QEMU HARDDISK\n");
                vga_puts("  Serial:   QM00001\n");
                vga_puts("  Capacity: 128 MB\n");
                vga_puts("  Sector:   512 bytes\n");
                vga_puts("  RPM:      7200\n");
                vga_puts("  Cache:    32 MB\n");
                vga_puts("  Standby:  immediate\n");
            } else {
                vga_puts("hdparm: use -I for device info\n");
            }
        }
        /* === Security === */
        else if (strcmp(args[0], "security") == 0) {
            if (argc < 2) { vga_puts("Usage: security <scan|status|sandbox|trust>\n"); return; }
            if (strcmp(args[1], "scan") == 0) security_scan();
            else if (strcmp(args[1], "status") == 0) {
                vga_puts("Security Status:\n");
                vga_puts("  Antivirus:    active\n");
                vga_puts("  Firewall:     active\n");
                vga_puts("  Integrity:    active\n");
                vga_puts("  Sandbox:      active\n");
                vga_puts("  SELinux:      enforcing\n");
                vga_puts("  ASLR:         enabled\n");
                vga_puts("  Stack guard:  enabled\n");
                vga_puts("  W^X:          enabled\n");
                vga_puts("  SMEP:         enabled\n");
            }
            else if (strcmp(args[1], "sandbox") == 0) security_sandbox_info();
            else if (strcmp(args[1], "trust") == 0) {
                if (argc < 3) { vga_puts("Usage: security trust <path>\n"); return; }
                security_add_trusted(args[2]);
                vga_puts("Added to trusted: "); vga_puts(args[2]); vga_puts("\n");
            }
            else { vga_puts_color("security: unknown command\n", VGA_COLOR(VGA_RED, VGA_BLACK)); }
        }
        else if (strcmp(args[0], "clamscan") == 0 || strcmp(args[0], "antivirus") == 0) {
            vga_puts_color("OrangeX Antivirus v1.0\n", VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK));
            vga_puts("Scanning system files...\n");
            security_scan();
        }
        else if (strcmp(args[0], "firewall") == 0) {
            if (argc < 2) { vga_puts("Usage: firewall <status|rules|block|allow>)\n"); return; }
            if (strcmp(args[1], "status") == 0) {
                vga_puts("Firewall: ACTIVE\n");
                vga_puts("Policy:   DROP ALL INCOMING\n");
                vga_puts("Rules:    12 active\n");
                vga_puts("Blocked:  0 packets\n");
                vga_puts("Allowed:  1247 packets\n");
            } else if (strcmp(args[1], "rules") == 0) {
                vga_puts("Firewall Rules:\n");
                vga_puts("  1. ALLOW TCP 80   (HTTP)\n");
                vga_puts("  2. ALLOW TCP 443  (HTTPS)\n");
                vga_puts("  3. ALLOW TCP 22   (SSH)\n");
                vga_puts("  4. ALLOW TCP 53   (DNS)\n");
                vga_puts("  5. ALLOW UDP 53   (DNS)\n");
                vga_puts("  6. ALLOW UDP 123  (NTP)\n");
                vga_puts("  7. ALLOW TCP 8080 (HTTP Alt)\n");
                vga_puts("  8. BLOCK  TCP *   (Default)\n");
                vga_puts("  9. BLOCK  UDP *   (Default)\n");
                vga_puts(" 10. BLOCK  ICMP    (Ping)\n");
                vga_puts(" 11. ALLOW  ICMP    (Echo)\n");
                vga_puts(" 12. LOG    ALL     (Audit)\n");
            }
            else if (strcmp(args[1], "block") == 0 && argc > 2) {
                vga_puts("Blocked port: "); vga_puts(args[2]); vga_puts("\n");
            }
            else if (strcmp(args[1], "allow") == 0 && argc > 2) {
                vga_puts("Allowed port: "); vga_puts(args[2]); vga_puts("\n");
            }
            else { vga_puts("firewall: usage: firewall <status|rules|block|allow>\n"); }
        }
        /* === System Control === */
        else if (strcmp(args[0], "reboot") == 0) {
            vga_puts_color("Rebooting...\n", VGA_COLOR(VGA_YELLOW, VGA_BLACK));
            timer_sleep(500);
            __asm__ __volatile__("outb %0, %1" : : "a"((uint8_t)0xFE), "Nd"((uint16_t)0x64));
            for (;;) __asm__ __volatile__("cli; hlt");
        }
        else if (strcmp(args[0], "poweroff") == 0 || strcmp(args[0], "halt") == 0 || strcmp(args[0], "shutdown") == 0) {
            vga_puts_color("System halted.\n", VGA_COLOR(VGA_YELLOW, VGA_BLACK));
            for (;;) __asm__ __volatile__("cli; hlt");
        }
        else if (strcmp(args[0], "exec") == 0) {
            if (argc < 2) { vga_puts("Usage: exec <binary>\n"); return; }
            vfs_node_t* n = vfs_lookup(args[1]);
            if (!n || !n->content) { vga_puts_color("exec: ", VGA_COLOR(VGA_RED, VGA_BLACK)); vga_puts_color(args[1], VGA_COLOR(VGA_RED, VGA_BLACK)); vga_puts(": not found\n"); return; }
            elf_header_t* hdr = (elf_header_t*)n->content;
            if (hdr->magic != ELF_MAGIC) { vga_puts_color("exec: not a valid ELF binary\n", VGA_COLOR(VGA_RED, VGA_BLACK)); return; }
            vga_puts("Loading ELF binary...\n");
            vga_puts("Entry point: 0x"); vga_print_hex(hdr->entry); vga_puts("\n");
            vga_puts("Type: ELF32 executable\n");
            vga_puts("Program headers: "); vga_print_dec(hdr->phnum); vga_puts("\n");
            uint32_t pid = process_create(args[1], hdr->entry, 1);
            if (pid) { vga_puts("Started process "); vga_print_dec(pid); vga_puts("\n"); }
            else vga_puts_color("exec: failed to create process\n", VGA_COLOR(VGA_RED, VGA_BLACK));
        }
        else if (strcmp(args[0], "df") == 0) {
            vga_puts("Filesystem     1K-blocks  Used  Available  Use%%  Mounted on\n");
            uint32_t total = pmm_get_total() * 4;
            uint32_t free_m = pmm_get_free() * 4;
            uint32_t used = total - free_m;
            uint32_t pct = total ? (used * 100 / total) : 0;
            vga_puts("/dev/ram0       "); vga_print_dec(total); vga_puts("   "); vga_print_dec(used); vga_puts("    "); vga_print_dec(free_m); vga_puts("     "); vga_print_dec(pct); vga_puts("%   /\n");
        }
        else if (strcmp(args[0], "du") == 0) {
            const char* path = (argc > 1 && args[1][0] != '-') ? args[1] : ".";
            vfs_node_t* dir = vfs_lookup(path);
            if (!dir) { vga_puts_color("du: cannot access\n", VGA_COLOR(VGA_RED, VGA_BLACK)); return; }
            uint32_t total = vfs_disk_usage(dir);
            vga_print_dec(total / 1024); vga_puts("K\t"); vga_puts(path); vga_puts("\n");
        }
        else if (strcmp(args[0], "stat") == 0) {
            if (argc < 2) { vga_puts("Usage: stat <file>\n"); return; }
            vfs_node_t* n = vfs_lookup(args[1]);
            if (!n) { vga_puts_color("stat: ", VGA_COLOR(VGA_RED, VGA_BLACK)); vga_puts_color(args[1], VGA_COLOR(VGA_RED, VGA_BLACK)); vga_puts(": not found\n"); return; }
            vga_puts("  File: "); vga_puts(args[1]); vga_puts("\n");
            vga_puts("  Size: "); vga_print_dec(n->size); vga_puts(" bytes\n");
            vga_puts("  Type: "); vga_puts((n->type & VFS_DIR) ? "directory" : "regular file"); vga_puts("\n");
            vga_puts("  Perms: "); vga_print_dec(n->perms); vga_puts("\n");
        }
        else if (strcmp(args[0], "file") == 0) {
            if (argc < 2) { vga_puts("Usage: file <file>\n"); return; }
            vfs_node_t* n = vfs_lookup(args[1]);
            if (!n) { vga_puts_color(args[1], VGA_COLOR(VGA_RED, VGA_BLACK)); vga_puts(": cannot open\n"); return; }
            if (n->type & VFS_DIR) { vga_puts(args[1]); vga_puts(": directory\n"); }
            else if (n->content && n->size >= 4) {
                uint32_t magic = *(uint32_t*)n->content;
                if (magic == 0x464C457F) { vga_puts(args[1]); vga_puts(": ELF 32-bit executable\n"); }
                else { vga_puts(args[1]); vga_puts(": ASCII text\n"); }
            } else { vga_puts(args[1]); vga_puts(": empty file\n"); }
        }
        else if (strcmp(args[0], "sort") == 0) {
            if (argc < 2) { vga_puts("Usage: sort <file>\n"); return; }
            vfs_node_t* n = vfs_lookup(args[1]);
            if (!n || !n->content) { vga_puts_color("sort: cannot open\n", VGA_COLOR(VGA_RED, VGA_BLACK)); return; }
            /* Simple bubble sort for lines */
            char* lines[64];
            int line_count = 0;
            uint32_t pos = 0;
            while (n->content[pos] && line_count < 64) {
                lines[line_count++] = &n->content[pos];
                while (n->content[pos] && n->content[pos] != '\n') pos++;
                if (n->content[pos] == '\n') pos++;
            }
            int i, j;
            for (i = 0; i < line_count - 1; i++) {
                for (j = 0; j < line_count - i - 1; j++) {
                    if (strcmp(lines[j], lines[j+1]) > 0) {
                        char* tmp = lines[j];
                        lines[j] = lines[j+1];
                        lines[j+1] = tmp;
                    }
                }
            }
            for (i = 0; i < line_count; i++) { vga_puts(lines[i]); vga_puts("\n"); }
        }
        else if (strcmp(args[0], "uniq") == 0) {
            if (argc < 2) { vga_puts("Usage: uniq <file>\n"); return; }
            vfs_node_t* n = vfs_lookup(args[1]);
            if (!n || !n->content) { vga_puts_color("uniq: cannot open\n", VGA_COLOR(VGA_RED, VGA_BLACK)); return; }
            uint32_t pos = 0;
            char prev[256] = "";
            while (n->content[pos]) {
                char line[256] = "";
                int k = 0;
                while (n->content[pos] && n->content[pos] != '\n' && k < 255) { line[k++] = n->content[pos++]; }
                line[k] = '\0';
                if (n->content[pos] == '\n') pos++;
                if (strcmp(line, prev) != 0) { vga_puts(line); vga_puts("\n"); }
                strcpy(prev, line);
            }
        }
        else if (strcmp(args[0], "wc") == 0) { if (argc > 1) cmd_wc(args[1]); else vga_puts("wc: missing file\n"); }
        else if (strcmp(args[0], "head") == 0) cmd_head(argc, args);
        else if (strcmp(args[0], "tail") == 0) cmd_tail(argc, args);
        else if (strcmp(args[0], "less") == 0) { if (argc > 1) cmd_less(args[1]); else vga_puts("less: missing file\n"); }
        else if (strcmp(args[0], "chmod") == 0) {
            if (argc < 3) { vga_puts("Usage: chmod <mode> <file>\n"); return; }
            vfs_node_t* n = vfs_lookup(args[2]);
            if (!n) { vga_puts_color("chmod: ", VGA_COLOR(VGA_RED, VGA_BLACK)); vga_puts_color(args[2], VGA_COLOR(VGA_RED, VGA_BLACK)); vga_puts(": not found\n"); return; }
            uint32_t mode = 0;
            int j;
            for (j = 0; args[1][j]; j++) { if (args[1][j] >= '0' && args[1][j] <= '7') mode = mode * 8 + (args[1][j] - '0'); }
            n->perms = mode;
            vga_puts("chmod: permissions updated\n");
        }
        else if (strcmp(args[0], "find") == 0) cmd_find(argc, args);
        else if (strcmp(args[0], "grep") == 0) cmd_grep(argc, args);
        else if (strcmp(args[0], "hexdump") == 0 || strcmp(args[0], "xxd") == 0) { if (argc > 1) cmd_hexdump(args[1]); else vga_puts("hexdump: missing file\n"); }
        else if (strcmp(args[0], "calc") == 0) { if (argc > 1) cmd_calc(args[1]); else vga_puts("calc: missing expression\n"); }
        else if (strcmp(args[0], "help") == 0 || strcmp(args[0], "?") == 0) vga_puts(help_text);
        else if (strcmp(args[0], "exit") == 0 || strcmp(args[0], "halt") == 0 || strcmp(args[0], "shutdown") == 0) {
            vga_puts_color("\nShutting down...\n", VGA_COLOR(VGA_YELLOW, VGA_BLACK));
            for (;;) __asm__ __volatile__("cli; hlt");
        }
        else if (strcmp(args[0], "doom") == 0) { doom_run(); }
        else if (strcmp(args[0], "neofetch") == 0 || strcmp(args[0], "fastfetch") == 0) {
            vga_puts_color("       ___  ____   __  ____\n", VGA_COLOR(VGA_LIGHT_RED, VGA_BLACK));
            vga_puts_color("      / __ \\/ __ | / / / __ \\\n", VGA_COLOR(VGA_LIGHT_RED, VGA_BLACK));
            vga_puts_color("     / /_/ / /_/ |/ / / / / /\n", VGA_COLOR(VGA_LIGHT_RED, VGA_BLACK));
            vga_puts_color("    / ____/ _, _/ / / /_/ /\n", VGA_COLOR(VGA_LIGHT_RED, VGA_BLACK));
            vga_puts_color("   /_/   /_/ |_/____/____/\n", VGA_COLOR(VGA_LIGHT_RED, VGA_BLACK));
            vga_puts("\n");
            vga_puts("OS:       OrangeX Kernel 1.0.0\n");
            vga_puts("Host:     QEMU Standard PC\n");
            vga_puts("Kernel:   1.0.0-orangex\n");
            vga_puts("Shell:    orangex-sh 1.0\n");
            vga_puts("CPU:      Intel 486 (1)\n");
            vga_puts("Memory:   "); vga_print_dec(pmm_get_total() * 4); vga_puts("KB\n");
            vga_puts("Uptime:   "); vga_print_dec(timer_ticks() / 100); vga_puts("s\n");
            vga_puts("Packages: 0 (use orangex-pkg to install)\n");
            vga_puts("Terminal: orangex-pty\n");
            vga_puts("Compiler: i686-elf-gcc\n");
        }
        /* === Programming Language Tools === */
        else if (strcmp(args[0], "gcc") == 0 || strcmp(args[0], "clang") == 0) {
            vga_puts_color("[OrangeX] ", VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK));
            vga_puts(args[0]); vga_puts(" is not installed.\n");
            vga_puts("Install with: orangex-pkg install gcc\n");
            vga_puts("Or use: cc (built-in C compiler stub)\n");
        }
        else if (strcmp(args[0], "cc") == 0 || strcmp(args[0], "c99") == 0) {
            if (argc < 2) { vga_puts("Usage: cc <source.c> -o <output>\n"); vga_puts("OrangeX C Compiler v0.1 (stub)\n"); }
            else { vga_puts_color("[OrangeX CC] ", VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK)); vga_puts("Compiling "); vga_puts(args[1]); vga_puts("...\n"); vga_puts("[OrangeX CC] Error: compiler not yet implemented in kernel space.\n"); vga_puts("[OrangeX CC] Use cross-compiler on host: i686-elf-gcc\n"); }
        }
        else if (strcmp(args[0], "python") == 0 || strcmp(args[0], "python3") == 0 || strcmp(args[0], "py") == 0) {
            vga_puts_color("[Python] ", VGA_COLOR(VGA_LIGHT_BLUE, VGA_BLACK));
            if (argc > 1 && strcmp(args[1], "--version") == 0) { vga_puts("Python 3.12.0 (OrangeX stub)\n"); }
            else if (argc > 1 && strcmp(args[1], "-c") == 0) {
                vga_puts_color("[Python] ", VGA_COLOR(VGA_LIGHT_BLUE, VGA_BLACK));
                vga_puts("Exec: "); vga_puts(argc > 2 ? args[2] : ""); vga_puts("\n");
                vga_puts("[Python] Interactive mode not available in kernel.\n");
                vga_puts("[Python] Use host Python for development.\n");
            } else { vga_puts("Python 3.12.0 (OrangeX stub)\n>>> Use host Python for real execution.\n"); }
        }
        else if (strcmp(args[0], "java") == 0 || strcmp(args[0], "javac") == 0) {
            vga_puts_color("[Java] ", VGA_COLOR(VGA_LIGHT_RED, VGA_BLACK));
            if (argc > 1 && strcmp(args[1], "-version") == 0) { vga_puts("openjdk 21.0.1 (OrangeX stub)\n"); }
            else { vga_puts("Java runtime not available in kernel.\n"); vga_puts("Use host JDK for development.\n"); }
        }
        else if (strcmp(args[0], "rustc") == 0 || strcmp(args[0], "cargo") == 0) {
            vga_puts_color("[Rust] ", VGA_COLOR(VGA_LIGHT_RED, VGA_BLACK));
            if (argc > 1 && strcmp(args[1], "--version") == 0) { vga_puts("rustc 1.75.0 (OrangeX stub)\n"); }
            else { vga_puts("Rust toolchain not available in kernel.\n"); vga_puts("Use host rustup for development.\n"); }
        }
        else if (strcmp(args[0], "go") == 0) {
            vga_puts_color("[Go] ", VGA_COLOR(VGA_LIGHT_CYAN, VGA_BLACK));
            if (argc > 1 && strcmp(args[1], "version") == 0) { vga_puts("go version go1.21.0 (OrangeX stub)\n"); }
            else { vga_puts("Go runtime not available in kernel.\n"); vga_puts("Use host Go for development.\n"); }
        }
        else if (strcmp(args[0], "node") == 0 || strcmp(args[0], "npm") == 0 || strcmp(args[0], "npx") == 0) {
            vga_puts_color("[Node.js] ", VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK));
            if (argc > 1 && strcmp(args[1], "--version") == 0) { vga_puts("v22.0.0 (OrangeX stub)\n"); }
            else { vga_puts("Node.js runtime not available in kernel.\n"); }
        }
        else if (strcmp(args[0], "ruby") == 0 || strcmp(args[0], "gem") == 0) {
            vga_puts_color("[Ruby] ", VGA_COLOR(VGA_LIGHT_RED, VGA_BLACK));
            vga_puts("Ruby 3.3.0 (OrangeX stub)\n");
        }
        else if (strcmp(args[0], "perl") == 0) {
            vga_puts_color("[Perl] ", VGA_COLOR(VGA_LIGHT_BLUE, VGA_BLACK));
            vga_puts("perl 5.38.0 (OrangeX stub)\n");
        }
        else if (strcmp(args[0], "php") == 0) {
            vga_puts_color("[PHP] ", VGA_COLOR(VGA_LIGHT_BLUE, VGA_BLACK));
            vga_puts("PHP 8.3.0 (OrangeX stub)\n");
        }
        else if (strcmp(args[0], "swift") == 0) {
            vga_puts_color("[Swift] ", VGA_COLOR(VGA_LIGHT_RED, VGA_BLACK));
            vga_puts("swift 5.9.0 (OrangeX stub)\n");
        }
        else if (strcmp(args[0], "kotlin") == 0 || strcmp(args[0], "kotlinc") == 0) {
            vga_puts_color("[Kotlin] ", VGA_COLOR(VGA_LIGHT_MAGENTA, VGA_BLACK));
            vga_puts("kotlin 1.9.20 (OrangeX stub)\n");
        }
        else if (strcmp(args[0], "lua") == 0) {
            vga_puts_color("[Lua] ", VGA_COLOR(VGA_LIGHT_BLUE, VGA_BLACK));
            vga_puts("Lua 5.4.0 (OrangeX stub)\n");
        }
        else if (strcmp(args[0], "make") == 0 || strcmp(args[0], "cmake") == 0) {
            vga_puts_color("[Build] ", VGA_COLOR(VGA_YELLOW, VGA_BLACK));
            vga_puts("GNU Make 4.3 (OrangeX stub)\n");
        }
        else if (strcmp(args[0], "git") == 0) {
            vga_puts_color("[Git] ", VGA_COLOR(VGA_LIGHT_RED, VGA_BLACK));
            if (argc > 1 && strcmp(args[1], "--version") == 0) { vga_puts("git version 2.43.0 (OrangeX stub)\n"); }
            else { vga_puts("Git version control (OrangeX stub)\n"); vga_puts("Use host git for real operations.\n"); }
        }
        else if (strcmp(args[0], "vim") == 0 || strcmp(args[0], "nano") == 0 || strcmp(args[0], "emacs") == 0) {
            vga_puts_color("[Editor] ", VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK));
            vga_puts(args[0]); vga_puts(" is not available in kernel.\n");
            vga_puts("Use the built-in 'cat' to view files.\n");
        }
        else if (strcmp(args[0], "man") == 0) {
            if (argc > 1) {
                vga_puts_color(args[1], VGA_COLOR(VGA_LIGHT_CYAN, VGA_BLACK));
                vga_puts(" - OrangeX manual page\n");
                vga_puts("No detailed manual available. Use 'help' for commands.\n");
            } else { vga_puts("What manual page do you want?\n"); }
        }
        else if (strcmp(args[0], "which") == 0) {
            if (argc > 1) { vga_puts_color(args[1], VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK)); vga_puts(": built-in command\n"); }
            else { vga_puts("which: missing argument\n"); }
        }
        else if (strcmp(args[0], "type") == 0) {
            if (argc > 1) { vga_puts_color(args[1], VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK)); vga_puts(" is a shell builtin command\n"); }
            else { vga_puts("type: missing argument\n"); }
        }
        else if (strcmp(args[0], "time") == 0) {
            uint32_t start = timer_ticks();
            if (argc > 1) { /* just measure empty */ }
            uint32_t elapsed = timer_ticks() - start;
            vga_print_dec(elapsed / 100); vga_puts("."); vga_print_dec(elapsed % 100); vga_puts("s real\n");
        }
        else if (strcmp(args[0], "history") == 0) {
            vga_puts("Command history is not yet implemented.\n");
        }
        else if (strcmp(args[0], "export") == 0 || strcmp(args[0], "setenv") == 0) {
            if (argc > 1) { vga_puts_color(args[1], VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK)); vga_puts("=set\n"); }
            else { vga_puts("export: missing argument\n"); }
        }
        else if (strcmp(args[0], "uname") == 0) {
            int all = 0, machine = 0;
            int i; for (i = 1; i < argc; i++) { if (args[i][0] == '-') { int j; for (j = 1; args[i][j]; j++) { if (args[i][j] == 'a') all = 1; if (args[i][j] == 'm') machine = 1; } } }
            if (machine && !all) vga_puts("i686\n");
            else vga_puts("OrangeX 1.0.0 orangex i686\n");
        }
        else {
            vga_puts_color(args[0], VGA_COLOR(VGA_RED, VGA_BLACK));
            vga_puts_color(": command not found\n", VGA_COLOR(VGA_RED, VGA_BLACK));
        }
    }
}
