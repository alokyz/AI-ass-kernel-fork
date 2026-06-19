#ifndef ORANGEX_WINDOW_H
#define ORANGEX_WINDOW_H

#include "types.h"

#define MAX_WINDOWS 32
#define WIN_TITLE_H 24

typedef struct {
    int  id;
    char title[64];
    int  x, y, w, h;
    int  visible;
    int  focused;
    int  dragging;
    int  drag_off_x, drag_off_y;
    uint32_t bg_color;
    uint32_t title_color;
    uint32_t* content;
} window_t;

void wm_init(void);
int  wm_create(const char* title, int x, int y, int w, int h, uint32_t bg);
void wm_destroy(int id);
void wm_draw_window(int id);
void wm_draw_all(void);
void wm_handle_mouse(int mx, int my, int buttons, int prev_buttons);
void wm_set_focus(int id);
window_t* wm_get_window(int id);
int  wm_get_count(void);

#endif
