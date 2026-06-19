#include "doom.h"
#include "vga.h"
#include "keyboard.h"
#include "timer.h"
#include "string.h"

#define MAP_W 16
#define MAP_H 16
#define SCREEN_W 80
#define SCREEN_H 24

static const char* map_data =
    "################"
    "#..............#"
    "#.##.###.##.##.#"
    "#.#..#.....#.#..#"
    "#.#.##.###.#.#.#"
    "#.....#.....#..#"
    "#.###.#.###.#.#.#"
    "#.#...#.#.#.#.#.#"
    "#.#.###.#.#.#.#.#"
    "#.#.....#...#...#"
    "#.#.###.#.###.###"
    "#.#...#.#.#.....#"
    "#.###.#.#.#.###.#"
    "#.....#...#.....#"
    "#.###.###.###.###"
    "################";

static float px = 1.5f, py = 1.5f, pa = 0.0f;

static int is_wall(float x, float y) {
    int mx = (int)x, my = (int)y;
    if (mx < 0 || mx >= MAP_W || my < 0 || my >= MAP_H) return 1;
    return map_data[my * MAP_W + mx] == '#';
}

static void render_frame(void) {
    int col;
    for (col = 0; col < SCREEN_W; col++) {
        float ra = pa - 1.5708f + (float)col * 3.14159f / (float)SCREEN_W;
        float ddx = 0, ddy = 0;
        float eye_x = px, eye_y = py;
        int ix, iy;

        /* Approximate direction components */
        int ri = (int)(ra * 1000);
        ri = ri % 6283;
        if (ri < 0) ri += 6283;
        if (ri < 1571) { ddx = 0.01f; ddy = (float)(ri) / 1571.0f * 0.01f; }
        else if (ri < 3142) { ddx = 0.01f; ddy = -(float)(ri - 1571) / 1571.0f * 0.01f; }
        else if (ri < 4712) { ddx = -0.01f; ddy = -(float)(ri - 3142) / 1571.0f * 0.01f; }
        else { ddx = -0.01f; ddy = (float)(ri - 4712) / 1571.0f * 0.01f; }

        float dist = 0;
        int hit = 0;
        int steps;
        for (steps = 0; steps < 2000; steps++) {
            eye_x += ddx;
            eye_y += ddy;
            dist += 0.01f;
            ix = (int)eye_x;
            iy = (int)eye_y;
            if (ix < 0 || ix >= MAP_W || iy < 0 || iy >= MAP_H) break;
            if (map_data[iy * MAP_W + ix] == '#') { hit = 1; break; }
        }

        int wall_h = hit ? (int)(20.0f / (dist + 0.01f)) : 0;
        if (wall_h > SCREEN_H) wall_h = SCREEN_H;
        int ds = (SCREEN_H - wall_h) / 2;
        int de = ds + wall_h;

        int row;
        for (row = 0; row < SCREEN_H; row++) {
            char ch = ' ';
            uint8_t fg = VGA_DARK_GREY, bg = VGA_BLACK;
            if (row < ds) {
                ch = ' ';
                bg = VGA_DARK_GREY;
            } else if (row < de) {
                ch = '|';
                if (dist < 2.0f) fg = VGA_WHITE;
                else if (dist < 5.0f) fg = VGA_LIGHT_GREY;
                else if (dist < 8.0f) fg = VGA_DARK_GREY;
                else fg = VGA_DARK_GREY;
                bg = VGA_BLACK;
            } else {
                ch = '-';
                fg = VGA_DARK_GREY;
                bg = VGA_BLACK;
            }
            vga_put_char_color(ch, VGA_COLOR(fg, bg));
        }
    }
}

static void draw_hud(void) {
    vga_puts_color("HP:100 AMMO:50 LVL:1  WASD:Move  Q:Quit\n",
                   VGA_COLOR(VGA_GREEN, VGA_BLACK));
}

void doom_run(void) {
    vga_clear();
    vga_puts_color("OrangeX DOOM v1.0\n", VGA_COLOR(VGA_RED, VGA_BLACK));
    vga_puts_color("WASD: Move  A/D: Turn  Q: Quit\n", VGA_COLOR(VGA_LIGHT_GREY, VGA_BLACK));
    vga_puts("Press any key to start...\n");
    keyboard_getchar();
    vga_clear();

    while (1) {
        render_frame();
        draw_hud();

        while (keyboard_has_data()) {
            char c = keyboard_try_getchar();
            if (!c) break;
            if (c == 'q' || c == 'Q') goto done;
            float sp = 0.2f;
            float rn = 0.12f;
            if (c == 'w' || c == 'W') {
                float nx = px + sp, ny = py + sp;
                if (!is_wall(nx, py)) px = nx;
                if (!is_wall(px, ny)) py = ny;
            }
            if (c == 's' || c == 'S') {
                float nx = px - sp, ny = py - sp;
                if (!is_wall(nx, py)) px = nx;
                if (!is_wall(px, ny)) py = ny;
            }
            if (c == 'a' || c == 'A') pa -= rn;
            if (c == 'd' || c == 'D') pa += rn;
        }
        timer_sleep(33);
    }
done:
    vga_clear();
    vga_puts_color("Thanks for playing OrangeX DOOM!\n", VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK));
}
