#include "desktop.h"
#include "graphics.h"
#include "mouse.h"
#include "window.h"
#include "keyboard.h"
#include "timer.h"
#include "string.h"
#include "heap.h"
#include "vfs.h"
#include "shell.h"
#include "pmm.h"

static int prev_buttons;
static desktop_icon_t icons[MAX_ICONS];
static int icon_count;
static int show_start_menu;
static terminal_t terminals[MAX_TERMINALS];
static terminal_t* active_terminal;
static int app_file_mgr_win;
static int app_browser_win;
static int app_viewer_win;
static int app_editor_win;
static int app_fetch_win;
static int app_mobile_win;
static int app_store_win;

static void term_puts(terminal_t* t, const char* s) {
    while (*s) {
        if (t->buf_len < TERM_BUF_SIZE - 1) t->buffer[t->buf_len++] = *s;
        s++;
    }
    t->buffer[t->buf_len] = '\0';
}

static void term_print_prompt(terminal_t* t) {
    term_puts(t, "root@orangex ~ % ");
}

static int term_tokenize(char* line, char** tok, int max) {
    int n = 0, in = 0;
    while (*line && n < max) {
        if (*line == ' ' || *line == '\t') { *line = '\0'; in = 0; }
        else { if (!in) { tok[n++] = line; in = 1; } }
        line++;
    }
    return n;
}

static void term_execute(terminal_t* t, const char* cmd) {
    char buf[TERM_LINE_SIZE];
    int i;
    for (i = 0; cmd[i] && i < TERM_LINE_SIZE - 1; i++) buf[i] = cmd[i];
    buf[i] = '\0';
    char* args[32];
    int argc = term_tokenize(buf, args, 32);
    if (argc == 0) { term_print_prompt(t); return; }
    if (strcmp(args[0], "clear") == 0) { t->buf_len = 0; t->buffer[0] = '\0'; }
    else if (strcmp(args[0], "help") == 0) {
        term_puts(t, "Commands: ls, cd, cat, echo, pwd, whoami, uname\n");
        term_puts(t, "  mkdir, touch, ps, kill, mem, uptime, clear, help\n");
    }
    else if (strcmp(args[0], "echo") == 0) {
        for (i = 1; i < argc; i++) { if (i > 1) term_puts(t, " "); term_puts(t, args[i]); }
        term_puts(t, "\n");
    }
    else if (strcmp(args[0], "whoami") == 0) term_puts(t, "root\n");
    else if (strcmp(args[0], "uname") == 0) term_puts(t, "O&angeX_Kernel 0.1.0 Darwin-compatible x86_32\n");
    else if (strcmp(args[0], "pwd") == 0) term_puts(t, "/\n");
    else if (strcmp(args[0], "ls") == 0) {
        vfs_node_t* dir = vfs_get_root();
        if (argc > 1) dir = vfs_lookup(args[1]);
        if (dir && (dir->type & VFS_DIR)) {
            vfs_node_t* c;
            for (c = dir->children; c; c = c->next) {
                term_puts(t, c->name);
                if (c->type & VFS_DIR) term_puts(t, "/");
                term_puts(t, "  ");
            }
            term_puts(t, "\n");
        } else term_puts(t, "ls: not found\n");
    }
    else if (strcmp(args[0], "cat") == 0) {
        if (argc < 2) term_puts(t, "cat: missing file\n");
        else { vfs_node_t* n = vfs_lookup(args[1]); if (n && n->content) term_puts(t, n->content); else term_puts(t, "cat: not found\n"); }
    }
    else if (strcmp(args[0], "mkdir") == 0) { if (argc > 1) vfs_mkdir(args[1]); }
    else if (strcmp(args[0], "touch") == 0) { if (argc > 1) vfs_touch(args[1]); }
    else if (strcmp(args[0], "ps") == 0) {
        term_puts(t, "  PID  STATE      NAME\n");
        term_puts(t, "    1  running    desktop\n");
        term_puts(t, "    2  running    shell\n");
    }
    else if (strcmp(args[0], "uptime") == 0) {
        uint32_t t2 = timer_ticks() / 100;
        char tmp[16]; int p = 0;
        tmp[p++] = '0' + t2 / 3600; tmp[p++] = ':';
        uint32_t m2 = (t2 / 60) % 60;
        tmp[p++] = '0' + m2 / 10; tmp[p++] = '0' + m2 % 10;
        tmp[p] = '\0';
        term_puts(t, "up "); term_puts(t, tmp); term_puts(t, "\n");
    }
    else if (strcmp(args[0], "mem") == 0) {
        uint32_t total = pmm_get_total() * 4;
        uint32_t free2 = pmm_get_free() * 4;
        term_puts(t, "Memory: total=");
        char tmp[16]; int p = 0;
        uint32_t v = total; char rev[16]; int k = 0;
        if (v == 0) { rev[k++] = '0'; } else { while (v > 0) { rev[k++] = '0' + (v % 10); v /= 10; } }
        while (k > 0) tmp[p++] = rev[--k];
        tmp[p] = '\0';
        term_puts(t, tmp); term_puts(t, "KB free=");
        v = free2; k = 0;
        if (v == 0) { rev[k++] = '0'; } else { while (v > 0) { rev[k++] = '0' + (v % 10); v /= 10; } }
        p = 0;
        while (k > 0) tmp[p++] = rev[--k];
        tmp[p] = '\0';
        term_puts(t, tmp); term_puts(t, "KB\n");
    }
    else { term_puts(t, "command not found: "); term_puts(t, args[0]); term_puts(t, "\n"); }
    term_print_prompt(t);
}

terminal_t* terminal_create(int win_id) {
    int i;
    for (i = 0; i < MAX_TERMINALS; i++) {
        if (!terminals[i].active) {
            terminals[i].active = 1;
            terminals[i].window_id = win_id;
            terminals[i].buf_len = 0;
            terminals[i].buffer[0] = '\0';
            terminals[i].line_len = 0;
            terminals[i].line_pos = 0;
            terminals[i].line[0] = '\0';
            active_terminal = &terminals[i];
            term_print_prompt(&terminals[i]);
            return &terminals[i];
        }
    }
    return NULL;
}

void terminal_keypress(terminal_t* t, char c) {
    if (!t || !t->active) return;
    if (c == '\n') {
        t->buffer[t->buf_len++] = '\n';
        t->buffer[t->buf_len] = '\0';
        t->line[t->line_len] = '\0';
        term_execute(t, t->line);
        t->line_len = 0;
        t->line_pos = 0;
        t->line[0] = '\0';
    } else if (c == '\b') {
        if (t->line_len > 0) {
            int i;
            for (i = t->line_pos - 1; i < t->line_len - 1; i++) t->line[i] = t->line[i + 1];
            t->line_len--;
            t->line_pos--;
            t->line[t->line_len] = '\0';
        }
    } else {
        if (t->line_len < TERM_LINE_SIZE - 1) {
            int i;
            for (i = t->line_len; i > t->line_pos; i--) t->line[i] = t->line[i - 1];
            t->line[t->line_pos] = c;
            t->line_len++;
            t->line_pos++;
            t->line[t->line_len] = '\0';
        }
    }
}

void terminal_draw(terminal_t* t, int wx, int wy, int ww, int wh) {
    if (!t || !t->active) return;
    int iy = wy + WIN_TITLE_H;
    int ih = wh - WIN_TITLE_H - 4;
    int tw = (ww - 16) / 8;
    int th = ih / 10;

    gfx_fill_rect(wx + 1, iy + 1, ww - 2, ih - 2, 0x0D1117);

    char dbuf[TERM_BUF_SIZE + TERM_LINE_SIZE + 64];
    int dl = 0, i;
    for (i = 0; i < t->buf_len && dl < TERM_BUF_SIZE - 1; i++) dbuf[dl++] = t->buffer[i];
    for (i = 0; i < t->line_len && dl < TERM_BUF_SIZE - 1; i++) dbuf[dl++] = t->line[i];
    dbuf[dl] = '\0';

    int lc = 0, start = 0;
    for (i = 0; i < dl; i++) if (dbuf[i] == '\n') lc++;
    int skip = lc - th + 2;
    if (skip < 0) skip = 0;
    for (i = 0; i < dl && skip > 0; i++) if (dbuf[i] == '\n') { start = i + 1; skip--; }

    int cx = 0, cy = 0;
    for (i = start; i < dl; i++) {
        if (dbuf[i] == '\n') { cx = 0; cy++; if (cy >= th) break; }
        else if (cx < tw) {
            uint32_t cc = 0xCDD6F4;
            if (dbuf[i] == '$' || dbuf[i] == '%') cc = 0xA6E3A1;
            gfx_draw_char(wx + 8 + cx * 8, iy + 4 + cy * 10, dbuf[i], cc, 0x0D1117);
            cx++;
        }
    }

    if (t == active_terminal) {
        static int cv = 1; cv = !cv;
        if (cv) gfx_fill_rect(wx + 8 + t->line_pos * 8, iy + 4 + cy * 10, 8, 10, 0xCDD6F4);
    }
}

static void draw_window_content(int win_id) {
    window_t* win = wm_get_window(win_id);
    if (!win || !win->content) return;
    int px, py;

    if (win_id == app_file_mgr_win) {
        for (py = 0; py < win->h; py++)
            for (px = 0; px < win->w; px++)
                win->content[py * win->w + px] = 0x1E1E2E;
        gfx_fill_rect(win->x, win->y + WIN_TITLE_H, win->w, 28, 0x313244);
        gfx_draw_string(win->x + 8, win->y + WIN_TITLE_H + 8, "/ (Root)", 0xCDD6F4, 0x313244);
        vfs_node_t* root = vfs_get_root();
        vfs_node_t* c;
        int iy = WIN_TITLE_H + 40;
        for (c = root->children; c; c = c->next) {
            if (c->type & VFS_DIR) {
                gfx_fill_rect(win->x + 8, win->y + iy, 20, 16, 0x89B4FA);
                gfx_draw_string(win->x + 32, win->y + iy + 4, c->name, 0xCDD6F4, 0x1E1E2E);
            } else {
                gfx_fill_rect(win->x + 8, win->y + iy, 16, 16, 0xFAB387);
                gfx_draw_string(win->x + 30, win->y + iy + 4, c->name, 0xCDD6F4, 0x1E1E2E);
            }
            iy += 24;
            if (iy > win->h - 10) break;
        }
    }
    else if (win_id == app_browser_win) {
        for (py = 0; py < win->h; py++)
            for (px = 0; px < win->w; px++)
                win->content[py * win->w + px] = 0xFFFFFF;
        gfx_fill_rect(win->x, win->y + WIN_TITLE_H, win->w, 30, 0xF5F5F5);
        gfx_draw_rect(win->x + 4, win->y + WIN_TITLE_H + 4, win->w - 8, 22, 0xDDDDDD);
        gfx_draw_string(win->x + 10, win->y + WIN_TITLE_H + 8, "https://orangex.dev", 0x888888, 0xFFFFFF);
        gfx_draw_string(win->x + 20, win->y + WIN_TITLE_H + 50, "Welcome to OrangeX Browser", 0x333333, 0xFFFFFF);
        gfx_draw_string(win->x + 20, win->y + WIN_TITLE_H + 70, "A simple web browser for", 0x666666, 0xFFFFFF);
        gfx_draw_string(win->x + 20, win->y + WIN_TITLE_H + 85, "OrangeX Kernel OS", 0x666666, 0xFFFFFF);
        gfx_draw_string(win->x + 20, win->y + WIN_TITLE_H + 115, "No network available.", 0xFF4444, 0xFFFFFF);
        gfx_draw_string(win->x + 20, win->y + WIN_TITLE_H + 130, "This is a local-only browser.", 0x888888, 0xFFFFFF);
    }
    else if (win_id == app_viewer_win) {
        for (py = 0; py < win->h; py++)
            for (px = 0; px < win->w; px++)
                win->content[py * win->w + px] = 0x2B2B2B;
        int cx = win->w / 2, cy = (win->h - WIN_TITLE_H) / 2 + WIN_TITLE_H;
        int r;
        for (r = 60; r > 0; r--) {
            uint32_t color = (r % 3 == 0) ? 0xFF6B6B : (r % 3 == 1) ? 0x4ECDC4 : 0x45B7D1;
            int x, y;
            for (x = -r; x <= r; x++)
                for (y = -r; y <= r; y++)
                    if (x*x + y*y <= r*r && x*x + y*y > (r-1)*(r-1))
                        gfx_put_pixel(win->x + cx + x, win->y + cy + y, color);
        }
        gfx_draw_string(win->x + 10, win->y + WIN_TITLE_H + 5, "No images loaded", 0xCCCCCC, 0x2B2B2B);
    }
    else if (win_id == app_editor_win) {
        for (py = 0; py < win->h; py++)
            for (px = 0; px < win->w; px++)
                win->content[py * win->w + px] = 0x1E1E2E;
        gfx_fill_rect(win->x, win->y + WIN_TITLE_H, win->w, 24, 0x313244);
        gfx_draw_string(win->x + 8, win->y + WIN_TITLE_H + 6, "untitled.c", 0x7AA2F7, 0x313244);
        const char* code[] = {
            "#include <stdio.h>",
            "",
            "int main(void) {",
            "    printf(\"Hello, OrangeX!\\n\");",
            "    return 0;",
            "}",
            "",
            "// OrangeX Code Editor v0.1",
            "// Syntax highlighting: basic",
        };
        int ln;
        for (ln = 0; ln < 9; ln++) {
            char num[4];
            int n = ln + 1, k = 0, p = 0;
            char rev[4];
            if (n == 0) { rev[k++] = '0'; } else { while (n > 0) { rev[k++] = '0' + (n % 10); n /= 10; } }
            while (k > 0) num[p++] = rev[--k];
            num[p] = '\0';
            gfx_draw_string(win->x + 4, win->y + WIN_TITLE_H + 32 + ln * 14, num, 0x555555, 0x1E1E2E);
            uint32_t cc = 0xCDD6F4;
            const char* line = code[ln];
            if (line[0] == '#') cc = 0xBB9AF7;
            else if (line[0] == '/' && line[1] == '/') cc = 0x565F89;
            else if (line[0] == 'i' || line[0] == 'r') cc = 0x7AA2F7;
            gfx_draw_string(win->x + 30, win->y + WIN_TITLE_H + 32 + ln * 14, line, cc, 0x1E1E2E);
        }
    }
    else if (win_id == app_fetch_win) {
        for (py = 0; py < win->h; py++)
            for (px = 0; px < win->w; px++)
                win->content[py * win->w + px] = 0x1A1B26;
        int sx = 12, sy = WIN_TITLE_H + 12;
        gfx_draw_string(win->x + sx, win->y + sy, "     ___  ____   __  ____", 0xF7768E, 0x1A1B26);
        gfx_draw_string(win->x + sx, win->y + sy + 12, "    / __ \\/ __ | / / / __ \\", 0xF7768E, 0x1A1B26);
        gfx_draw_string(win->x + sx, win->y + sy + 24, "   / /_/ / /_/ |/ / / / / /", 0xF7768E, 0x1A1B26);
        gfx_draw_string(win->x + sx, win->y + sy + 36, "  / ____/ _, _/ / / /_/ /", 0xF7768E, 0x1A1B26);
        gfx_draw_string(win->x + sx, win->y + sy + 48, " /_/   /_/ |_/____/____/", 0xF7768E, 0x1A1B26);
        int dy = sy + 68;
        gfx_draw_string(win->x + sx, win->y + dy, "OS:       OrangeX Kernel 0.1.0", 0x9ECE6A, 0x1A1B26); dy += 14;
        gfx_draw_string(win->x + sx, win->y + dy, "Kernel:   0.1.0-OrangEx", 0x7DCFFF, 0x1A1B26); dy += 14;
        gfx_draw_string(win->x + sx, win->y + dy, "Shell:    orangex-sh 1.0", 0x7DCFFF, 0x1A1B26); dy += 14;
        gfx_draw_string(win->x + sx, win->y + dy, "Terminal: VGA Text / Framebuffer", 0x7DCFFF, 0x1A1B26); dy += 14;
        gfx_draw_string(win->x + sx, win->y + dy, "CPU:      Intel 486 (i686)", 0xE0AF68, 0x1A1B26); dy += 14;
        gfx_draw_string(win->x + sx, win->y + dy, "Memory:   128MB RAM", 0xE0AF68, 0x1A1B26); dy += 14;
        gfx_draw_string(win->x + sx, win->y + dy, "Display:  1024x768x32", 0xBB9AF7, 0x1A1B26); dy += 14;
        gfx_draw_string(win->x + sx, win->y + dy, "Uptime:   running...", 0x73DACA, 0x1A1B26); dy += 14;
        gfx_draw_string(win->x + sx, win->y + dy, "Packages: 14 (kernel modules)", 0xF7768E, 0x1A1B26); dy += 14;
        gfx_draw_string(win->x + sx, win->y + dy, "Compiler: i686-elf-gcc 15.2", 0xFF9E64, 0x1A1B26); dy += 14;
    }
    else if (win_id == app_mobile_win) {
        for (py = 0; py < win->h; py++)
            for (px = 0; px < win->w; px++)
                win->content[py * win->w + px] = 0x000000;
        gfx_fill_rect(win->x + 4, win->y + WIN_TITLE_H + 4, win->w - 8, 20, 0x1A1B26);
        uint32_t t2 = timer_ticks() / 100;
        uint32_t h2 = (t2 / 3600) % 24;
        uint32_t m2 = (t2 / 60) % 60;
        char clk[6]; clk[0] = '0' + h2 / 10; clk[1] = '0' + h2 % 10;
        clk[2] = ':'; clk[3] = '0' + m2 / 10; clk[4] = '0' + m2 % 10; clk[5] = '\0';
        gfx_draw_string(win->x + win->w / 2 - 20, win->y + WIN_TITLE_H + 8, clk, 0xFFFFFF, 0x1A1B26);

        int bx = 8, by = WIN_TITLE_H + 36;
        int bw = (win->w - 24) / 3, bh = 36;
        const char* apps[] = {"Call", "Msg", "Cam"};
        uint32_t ac[] = {0x4CAF50, 0x2196F3, 0xFF9800};
        int i;
        for (i = 0; i < 3; i++) {
            gfx_fill_rect(win->x + bx + i * (bw + 4), win->y + by, bw, bh, ac[i]);
            gfx_draw_string(win->x + bx + i * (bw + 4) + 8, win->y + by + 12, apps[i], 0xFFFFFF, ac[i]);
        }

        by += bh + 8;
        for (i = 0; i < 3; i++) {
            gfx_fill_rect(win->x + bx + i * (bw + 4), win->y + by, bw, bh, 0x555555);
            gfx_draw_string(win->x + bx + i * (bw + 4) + 4, win->y + by + 12, i == 0 ? "Sms" : i == 1 ? "Web" : "Set", 0xCCCCCC, 0x555555);
        }

        by += bh + 12;
        gfx_draw_rect(win->x + 10, win->y + by, win->w - 20, 40, 0x333333);
        gfx_draw_string(win->x + 16, win->y + by + 12, "OrangeX Mobile", 0xAAAAAA, 0x000000);
        gfx_draw_string(win->x + 16, win->y + by + 26, "v0.1 - Simulator", 0x666666, 0x000000);
    }
    else if (win_id == app_store_win) {
        for (py = 0; py < win->h; py++)
            for (px = 0; px < win->w; px++)
                win->content[py * win->w + px] = 0x1E1E2E;
        gfx_draw_string(win->x + 12, win->y + WIN_TITLE_H + 8, "OrangeX App Store", 0xF5C2E7, 0x1E1E2E);
        struct { const char* name; const char* desc; const char* ver; uint32_t color; } apps[] = {
            {"Python 3.12", "Python interpreter for OrangeX", "3.12.0", 0x4B8BBE},
            {"GCC Toolchain", "GNU Compiler Collection", "15.2.0", 0x9ECE6A},
            {"Nano Editor", "Simple text editor", "8.3", 0x7DCFFF},
            {"Git", "Version control system", "2.45", 0xF7768E},
            {"Node.js", "JavaScript runtime", "22.0", 0x9ECE6A},
            {"Curl", "URL transfer tool", "8.7", 0xE0AF68},
            {"FFmpeg", "Media framework", "7.0", 0xBB9AF7},
            {"Vim", "Advanced text editor", "9.1", 0x9ECE6A},
        };
        int iy = WIN_TITLE_H + 28;
        int i;
        for (i = 0; i < 8; i++) {
            gfx_fill_rect(win->x + 8, win->y + iy, 32, 32, apps[i].color);
            gfx_draw_string(win->x + 48, win->y + iy + 2, apps[i].name, 0xCDD6F4, 0x1E1E2E);
            gfx_draw_string(win->x + 48, win->y + iy + 16, apps[i].desc, 0x6C7086, 0x1E1E2E);
            gfx_fill_rect(win->x + win->w - 60, win->y + iy + 4, 48, 20, 0x9ECE6A);
            gfx_draw_string(win->x + win->w - 56, win->y + iy + 8, "Install", 0x1E1E2E, 0x9ECE6A);
            iy += 40;
            if (iy > win->h - 10) break;
        }
    }
}

void desktop_init(void) {
    icon_count = 0;
    show_start_menu = 0;
    prev_buttons = 0;
    active_terminal = NULL;
    app_file_mgr_win = 0;
    app_browser_win = 0;
    app_viewer_win = 0;
    app_editor_win = 0;
    app_fetch_win = 0;
    app_mobile_win = 0;
    app_store_win = 0;
    int i;
    for (i = 0; i < MAX_TERMINALS; i++) terminals[i].active = 0;

    strcpy(icons[0].name, "Terminal");  icons[0].x = 20;  icons[0].y = 20;  icons[0].color = COLOR_GREEN;  icon_count++;
    strcpy(icons[1].name, "Files");     icons[1].x = 20;  icons[1].y = 80;  icons[1].color = COLOR_BLUE;   icon_count++;
    strcpy(icons[2].name, "Browser");   icons[2].x = 20;  icons[2].y = 140; icons[2].color = COLOR_CYAN;   icon_count++;
    strcpy(icons[3].name, "Editor");    icons[3].x = 20;  icons[3].y = 200; icons[3].color = COLOR_BTN;    icon_count++;
    strcpy(icons[4].name, "Viewer");    icons[4].x = 20;  icons[4].y = 260; icons[4].color = COLOR_ORANGE; icon_count++;
    strcpy(icons[5].name, "Fastfetch"); icons[5].x = 90;  icons[5].y = 20;  icons[5].color = COLOR_YELLOW; icon_count++;
    strcpy(icons[6].name, "Mobile");    icons[6].x = 90;  icons[6].y = 80;  icons[6].color = 0x4CAF50;     icon_count++;
    strcpy(icons[7].name, "AppStore");  icons[7].x = 90;  icons[7].y = 140; icons[7].color = 0xF7768E;     icon_count++;

    wm_init();
}

static int action_terminal(void) {
    int id = wm_create("Terminal", 120, 60, 520, 380, COLOR_BG);
    if (id > 0) terminal_create(id);
    return id;
}

static int action_files(void) {
    int id = wm_create("File Manager", 150, 80, 420, 340, 0x1E1E2E);
    if (id > 0) app_file_mgr_win = id;
    return id;
}

static int action_browser(void) {
    int id = wm_create("Browser", 180, 70, 480, 360, 0xFFFFFF);
    if (id > 0) app_browser_win = id;
    return id;
}

static int action_editor(void) {
    int id = wm_create("Code Editor", 200, 60, 460, 380, 0x1E1E2E);
    if (id > 0) app_editor_win = id;
    return id;
}

static int action_viewer(void) {
    int id = wm_create("Image Viewer", 220, 80, 400, 320, 0x2B2B2B);
    if (id > 0) app_viewer_win = id;
    return id;
}

static int action_fetch(void) {
    int id = wm_create("Fastfetch", 250, 50, 360, 340, 0x1A1B26);
    if (id > 0) app_fetch_win = id;
    return id;
}

static int action_mobile(void) {
    int id = wm_create("Mobile Simulator", 350, 60, 200, 360, 0x000000);
    if (id > 0) app_mobile_win = id;
    return id;
}

static int action_store(void) {
    int id = wm_create("App Store", 200, 60, 420, 380, 0x1E1E2E);
    if (id > 0) app_store_win = id;
    return id;
}

static void draw_taskbar(void) {
    int sw = gfx_width();
    int sh = gfx_height();
    gfx_fill_rect(0, sh - TASKBAR_H, sw, TASKBAR_H, COLOR_TASKBAR);
    gfx_draw_line(0, sh - TASKBAR_H, sw, sh - TASKBAR_H, COLOR_DKGRAY);
    gfx_fill_rect(4, sh - TASKBAR_H + 4, 60, TASKBAR_H - 8, COLOR_TITLE);
    gfx_draw_string(10, sh - TASKBAR_H + 10, "Start", COLOR_WHITE, 0xFFFFFFFF);

    int i, tx = 72;
    for (i = 0; i < MAX_WINDOWS; i++) {
        window_t* win = wm_get_window(i + 1);
        if (win && win->id != 0) {
            uint32_t bg = win->focused ? COLOR_DKGRAY : 0x333344;
            int tw = strlen(win->title) * 8 + 16;
            if (tx + tw > sw - 70) break;
            gfx_fill_rect(tx, sh - TASKBAR_H + 4, tw, TASKBAR_H - 8, bg);
            gfx_draw_string(tx + 8, sh - TASKBAR_H + 10, win->title, COLOR_WHITE, 0xFFFFFFFF);
            tx += tw + 4;
        }
    }

    uint32_t t = timer_ticks() / 100;
    uint32_t h = (t / 3600) % 24, m = (t / 60) % 60;
    char clk[8]; int p = 0;
    if (h < 10) clk[p++] = '0';
    char rv[4]; int k = 0, v = h;
    if (v == 0) { rv[k++] = '0'; } else { while (v > 0) { rv[k++] = '0' + (v % 10); v /= 10; } }
    int j; for (j = k-1; j >= 0; j--) clk[p++] = rv[j];
    clk[p++] = ':';
    if (m < 10) clk[p++] = '0';
    k = 0; v = m;
    if (v == 0) { rv[k++] = '0'; } else { while (v > 0) { rv[k++] = '0' + (v % 10); v /= 10; } }
    for (j = k-1; j >= 0; j--) clk[p++] = rv[j];
    clk[p] = '\0';
    gfx_draw_string(sw - 60, sh - TASKBAR_H + 10, clk, COLOR_WHITE, 0xFFFFFFFF);
}

static void draw_start_menu(void) {
    int sh = gfx_height();
    int mx = 4, my = sh - TASKBAR_H - 200;
    int mw = 160, mh = 200;
    gfx_fill_rect(mx, my, mw, mh, COLOR_TASKBAR);
    gfx_draw_rect_thick(mx, my, mw, mh, COLOR_DKGRAY, 1);
    const char* items[] = {"Terminal", "Files", "Browser", "Editor", "Viewer", "Fastfetch", "Mobile", "App Store", "Shutdown"};
    uint32_t colors[] = {COLOR_GREEN, COLOR_BLUE, COLOR_CYAN, COLOR_BTN, COLOR_ORANGE, COLOR_YELLOW, 0x4CAF50, 0xF7768E, COLOR_RED};
    int i;
    for (i = 0; i < 9; i++) {
        int iy = my + 4 + i * 22;
        gfx_fill_rect(mx + 2, iy, mw - 4, 20, colors[i]);
        gfx_draw_string(mx + 10, iy + 6, items[i], COLOR_WHITE, 0xFFFFFFFF);
    }
}

static void draw_cursor(int x, int y) {
    gfx_draw_line(x, y, x, y + 12, COLOR_WHITE);
    gfx_draw_line(x, y, x + 8, y + 6, COLOR_WHITE);
    gfx_draw_line(x + 8, y + 6, x + 4, y + 6, COLOR_WHITE);
    gfx_draw_line(x + 4, y + 6, x + 4, y + 10, COLOR_WHITE);
    gfx_draw_line(x + 4, y + 10, x + 2, y + 12, COLOR_WHITE);
}

static void draw_icon(desktop_icon_t* icon) {
    gfx_fill_rect(icon->x, icon->y, ICON_SIZE, ICON_SIZE, icon->color);
    gfx_draw_rect(icon->x, icon->y, ICON_SIZE, ICON_SIZE, COLOR_WHITE);
    int tx = icon->x + (ICON_SIZE - strlen(icon->name) * 8) / 2;
    if (tx < icon->x) tx = icon->x + 2;
    gfx_draw_string(tx, icon->y + ICON_SIZE + 4, icon->name, COLOR_WHITE, 0xFFFFFFFF);
}

static int is_terminal_window(int wid) {
    int i;
    for (i = 0; i < MAX_TERMINALS; i++)
        if (terminals[i].active && terminals[i].window_id == wid) return 1;
    return 0;
}

static terminal_t* get_terminal_for_window(int wid) {
    int i;
    for (i = 0; i < MAX_TERMINALS; i++)
        if (terminals[i].active && terminals[i].window_id == wid) return &terminals[i];
    return NULL;
}

void desktop_draw(void) {
    gfx_clear(COLOR_BG);
    int i;
    for (i = 0; i < icon_count; i++) draw_icon(&icons[i]);

    for (i = 0; i < MAX_WINDOWS; i++) {
        window_t* win = wm_get_window(i + 1);
        if (win && win->id != 0 && win->visible) {
            if (win->id == app_file_mgr_win || win->id == app_browser_win ||
                win->id == app_viewer_win || win->id == app_editor_win ||
                win->id == app_fetch_win || win->id == app_mobile_win ||
                win->id == app_store_win) {
                draw_window_content(win->id);
            }
            wm_draw_window(win->id);
            if (is_terminal_window(win->id)) {
                terminal_t* t = get_terminal_for_window(win->id);
                if (t) terminal_draw(t, win->x, win->y, win->w, win->h);
            }
        }
    }

    draw_taskbar();
    if (show_start_menu) draw_start_menu();
    mouse_state_t* ms = mouse_get_state();
    draw_cursor(ms->x, ms->y);
    gfx_flip();
}

void desktop_handle_input(void) {
    mouse_state_t* ms = mouse_get_state();
    int sh = gfx_height();
    int mx = ms->x, my = ms->y;
    int buttons = ms->buttons;
    int clicked = (buttons & 1) && !(prev_buttons & 1);
    int i;

    wm_handle_mouse(mx, my, buttons, prev_buttons);

    if (clicked) {
        for (i = 0; i < MAX_WINDOWS; i++) {
            window_t* win = wm_get_window(i + 1);
            if (win && win->id != 0 && win->visible &&
                mx >= win->x && mx < win->x + win->w &&
                my >= win->y && my < win->y + win->h) {
                if (is_terminal_window(win->id))
                    active_terminal = get_terminal_for_window(win->id);
                else
                    active_terminal = NULL;
                break;
            }
        }
    }

    if (clicked) {
        if (my >= sh - TASKBAR_H) {
            if (mx >= 4 && mx <= 64) show_start_menu = !show_start_menu;
            else show_start_menu = 0;
        } else if (show_start_menu) {
            int menu_x = 4, menu_y = sh - TASKBAR_H - 200;
            if (mx >= menu_x && mx <= menu_x + 160 && my >= menu_y && my <= menu_y + 200) {
                int sel = (my - menu_y - 4) / 22;
                if (sel == 0) action_terminal();
                else if (sel == 1) action_files();
                else if (sel == 2) action_browser();
                else if (sel == 3) action_editor();
                else if (sel == 4) action_viewer();
                else if (sel == 5) action_fetch();
                else if (sel == 6) action_mobile();
                else if (sel == 7) action_store();
                else if (sel == 8) {
                    gfx_clear(COLOR_BLACK);
                    gfx_draw_string(300, 300, "System halted.", COLOR_WHITE, COLOR_BLACK);
                    gfx_flip();
                    for (;;) __asm__ __volatile__("cli; hlt");
                }
                show_start_menu = 0;
            } else show_start_menu = 0;
        } else {
            for (i = 0; i < icon_count; i++) {
                if (mx >= icons[i].x && mx < icons[i].x + ICON_SIZE &&
                    my >= icons[i].y && my < icons[i].y + ICON_SIZE) {
                    if (i == 0) action_terminal();
                    else if (i == 1) action_files();
                    else if (i == 2) action_browser();
                    else if (i == 3) action_editor();
                    else if (i == 4) action_viewer();
                    else if (i == 5) action_fetch();
                    else if (i == 6) action_mobile();
                    else if (i == 7) action_store();
                    break;
                }
            }
        }
    }
    prev_buttons = buttons;
}

void desktop_update(void) {
    while (keyboard_has_data()) {
        char c = keyboard_try_getchar();
        if (!c) break;
        if (active_terminal) terminal_keypress(active_terminal, c);
    }
    desktop_handle_input();
    desktop_draw();
}
