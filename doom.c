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

// Basic static lookup functions for SIN/COS approximation since we lack math.h
static float local_sin(float rad) {
    // Maclaurin series approximation for sin
    float x = rad;
    // Keep angle within -PI to PI
    while (x > 3.14159f)  x -= 6.28318f;
    while (x < -3.14159f) x += 6.28318f;
    float x3 = x * x * x;
    float x5 = x3 * x * x;
    return x - (x3 / 6.0f) + (x5 / 120.0f);
}

static float local_cos(float rad) {
    return local_sin(rad + 1.57079f);
}

static int is_wall(float x, float y) {
    int mx = (int)x, my = (int)y;
    if (mx < 0 || mx >= MAP_W || my < 0 || my >= MAP_H) return 1;
    return map_data[my * MAP_W + mx] == '#';
}

static void render_frame(void) {
    // FIX 1: Reset VGA cursor position to (0,0) before redrawing to prevent scrolling artifacting
    vga_set_cursor(0, 0); 

    int col;
    for (col = 0; col < SCREEN_W; col++) {
        // Field of view spanning 60 degrees around the player angle
        float ra = pa - 0.5f + ((float)col / (float)SCREEN_W);
        
        // FIX 3: True directional components using sin/cos functions
        float ddx = local_cos(ra) * 0.02f;
        float ddy = local_sin(ra) * 0.02f;
        
        float eye_x = px, eye_y = py;
        float dist = 0;
        int hit = 0;

        int steps;
        for (steps = 0; steps < 1000; steps++) {
            eye_x += ddx;
            eye_y += ddy;
            dist += 0.02f;

            int ix = (int)eye_x;
            int iy = (int)eye_y;

            if (ix < 0 || ix >= MAP_W || iy < 0 || iy >= MAP_H) break;
            if (map_data[iy * MAP_W + ix] == '#') { 
                hit = 1; 
                break; 
            }
        }

        // Fish-eye correction math
        float corrected_dist = dist * local_cos(ra - pa);
        if (corrected_dist < 0.1f) corrected_dist = 0.1f;

        int wall_h = hit ? (int)(15.0f / corrected_dist) : 0;
        if (wall_h > SCREEN_H) wall_h = SCREEN_H;
        
        int ds = (SCREEN_H - wall_h) / 2;
        int de = ds + wall_h;

        int row;
        for (row = 0; row < SCREEN_H; row++) {
            char ch = ' ';
            uint8_t fg = VGA_DARK_GREY, bg = VGA_BLACK;
            
            if (row < ds) {
                ch = ' ';
                bg = VGA_BLUE; // Ceiling
            } else if (row < de) {
                ch = '#'; // Wall glyph
                if (dist < 3.0f) fg = VGA_WHITE;
                else if (dist < 6.0f) fg = VGA_LIGHT_GREY;
                else fg = VGA_DARK_GREY;
                bg = VGA_BLACK;
            } else {
                ch = '.'; // Floor
                fg = VGA_GREEN;
                bg = VGA_BLACK;
            }
            vga_put_char_color(ch, VGA_COLOR(fg, bg));
        }
    }
}

static void draw_hud(void) {
    vga_set_cursor(0, SCREEN_H);
    vga_puts_color("HP:100 AMMO:50   WASD:Move  A/D:Turn  Q:Quit       ",
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
            
            float sp = 0.15f; // Step speed
            float rn = 0.15f; // Turn angle delta

            // FIX 2: Angle-aware direction movement vectors
            if (c == 'w' || c == 'W') {
                float nx = px + local_cos(pa) * sp;
                float ny = py + local_sin(pa) * sp;
                if (!is_wall(nx, py)) px = nx;
                if (!is_wall(px, ny)) py = ny;
            }
            if (c == 's' || c == 'S') {
                float nx = px - local_cos(pa) * sp;
                float ny = py - local_sin(pa) * sp;
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