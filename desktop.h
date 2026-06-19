#ifndef ORANGEX_DESKTOP_H
#define ORANGEX_DESKTOP_H

#include "types.h"

#define TASKBAR_H 32
#define ICON_SIZE 48
#define MAX_ICONS 16
#define TERM_BUF_SIZE 4096
#define TERM_LINE_SIZE 256
#define TERM_MAX_LINES 200

typedef struct {
    char name[32];
    int x, y;
    uint32_t color;
    int (*action)(void);
} desktop_icon_t;

typedef struct {
    char    buffer[TERM_BUF_SIZE];
    int     buf_len;
    char    line[TERM_LINE_SIZE];
    int     line_len;
    int     line_pos;
    int     window_id;
    int     active;
    int     scroll_offset;
    int     text_rows;
    int     text_cols;
} terminal_t;

#define MAX_TERMINALS 4

void desktop_init(void);
void desktop_draw(void);
void desktop_update(void);
terminal_t* terminal_create(int win_id);
void terminal_keypress(terminal_t* t, char c);
void terminal_draw(terminal_t* t, int win_x, int win_y, int win_w, int win_h);

#endif
