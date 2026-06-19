#include "window.h"
#include "graphics.h"
#include "heap.h"
#include "string.h"

static window_t windows[MAX_WINDOWS];
static int next_id;
static int top_window;

void wm_init(void) {
    int i;
    for (i = 0; i < MAX_WINDOWS; i++) windows[i].id = 0;
    next_id = 1;
    top_window = -1;
}

int wm_create(const char* title, int x, int y, int w, int h, uint32_t bg) {
    int i;
    for (i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].id == 0) {
            windows[i].id = next_id++;
            int j;
            for (j = 0; title[j] && j < 63; j++) windows[i].title[j] = title[j];
            windows[i].title[j] = '\0';
            windows[i].x = x; windows[i].y = y;
            windows[i].w = w; windows[i].h = h;
            windows[i].visible = 1;
            windows[i].focused = 0;
            windows[i].dragging = 0;
            windows[i].bg_color = bg;
            windows[i].title_color = COLOR_TITLE;
            windows[i].content = (uint32_t*)kmalloc(w * h * 4);
            if (windows[i].content) {
                int px, py;
                for (py = 0; py < h; py++)
                    for (px = 0; px < w; px++)
                        windows[i].content[py * w + px] = bg;
            }
            top_window = i;
            wm_set_focus(windows[i].id);
            return windows[i].id;
        }
    }
    return -1;
}

void wm_destroy(int id) {
    int i;
    for (i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].id == id) {
            if (windows[i].content) kfree(windows[i].content);
            windows[i].id = 0;
            windows[i].visible = 0;
            if (top_window == i) top_window = -1;
            return;
        }
    }
}

void wm_set_focus(int id) {
    int i;
    for (i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].id == id) {
            windows[i].focused = 1;
            windows[i].title_color = COLOR_TITLE;
            top_window = i;
        } else if (windows[i].id != 0) {
            windows[i].focused = 0;
            windows[i].title_color = COLOR_DKGRAY;
        }
    }
}

window_t* wm_get_window(int id) {
    int i;
    for (i = 0; i < MAX_WINDOWS; i++)
        if (windows[i].id == id) return &windows[i];
    return NULL;
}

int wm_get_count(void) {
    int i, count = 0;
    for (i = 0; i < MAX_WINDOWS; i++)
        if (windows[i].id != 0) count++;
    return count;
}

static void draw_title_bar(window_t* win) {
    gfx_fill_rect(win->x, win->y, win->w, WIN_TITLE_H, win->title_color);
    gfx_draw_string(win->x + 8, win->y + 8, win->title, COLOR_WHITE, 0xFFFFFFFF);

    int bx = win->x + win->w - 20;
    gfx_fill_rect(bx, win->y + 4, 16, 16, COLOR_RED);
    gfx_draw_string(bx + 4, win->y + 6, "X", COLOR_WHITE, 0xFFFFFFFF);

    if (win->focused) {
        gfx_fill_rect(win->x + win->w - 40, win->y + 4, 16, 16, COLOR_GREEN);
        gfx_draw_string(win->x + win->w - 38, win->y + 6, "_", COLOR_WHITE, 0xFFFFFFFF);
    }
}

void wm_draw_window(int id) {
    window_t* win = wm_get_window(id);
    if (!win || !win->visible) return;

    gfx_fill_rect(win->x, win->y, win->w, win->h, COLOR_DKGRAY);
    draw_title_bar(win);

    if (win->content) {
        int px, py;
        for (py = 0; py < win->h - WIN_TITLE_H; py++) {
            for (px = 0; px < win->w; px++) {
                uint32_t c = win->content[(py + WIN_TITLE_H) * win->w + px];
                gfx_put_pixel(win->x + px, win->y + WIN_TITLE_H + py, c);
            }
        }
    } else {
        gfx_fill_rect(win->x + 1, win->y + WIN_TITLE_H + 1,
                       win->w - 2, win->h - WIN_TITLE_H - 2, win->bg_color);
    }

    gfx_draw_rect_thick(win->x, win->y, win->w, win->h, COLOR_DKGRAY, 1);
}

void wm_draw_all(void) {
    int i;
    for (i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].id != 0 && windows[i].visible)
            wm_draw_window(windows[i].id);
    }
}

static int hit_test(int mx, int my, window_t* win) {
    return (mx >= win->x && mx < win->x + win->w &&
            my >= win->y && my < win->y + win->h);
}

void wm_handle_mouse(int mx, int my, int buttons, int prev_buttons) {
    int i;
    int clicked = (buttons & 1) && !(prev_buttons & 1);

    if (clicked) {
        for (i = MAX_WINDOWS - 1; i >= 0; i--) {
            if (windows[i].id != 0 && windows[i].visible && hit_test(mx, my, &windows[i])) {
                wm_set_focus(windows[i].id);

                int bx = windows[i].x + windows[i].w - 20;
                if (mx >= bx && mx < bx + 16 && my >= windows[i].y && my < windows[i].y + 16) {
                    wm_destroy(windows[i].id);
                    return;
                }

                if (my < windows[i].y + WIN_TITLE_H) {
                    windows[i].dragging = 1;
                    windows[i].drag_off_x = mx - windows[i].x;
                    windows[i].drag_off_y = my - windows[i].y;
                }
                break;
            }
        }
    }

    if (!(buttons & 1)) {
        for (i = 0; i < MAX_WINDOWS; i++)
            windows[i].dragging = 0;
    }

    if (buttons & 1) {
        for (i = 0; i < MAX_WINDOWS; i++) {
            if (windows[i].id != 0 && windows[i].dragging) {
                windows[i].x = mx - windows[i].drag_off_x;
                windows[i].y = my - windows[i].drag_off_y;
                if (windows[i].x < 0) windows[i].x = 0;
                if (windows[i].y < 0) windows[i].y = 0;
            }
        }
    }
}
