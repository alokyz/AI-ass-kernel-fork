#include "keyboard.h"
#include "port.h"
#include "isr.h"
#include "vga.h"
#include "string.h"

static kfifo_t kfifo;
static int shift_down;
static int caps_lock;
static int esc_prefix;

static const char sc_normal[128] = {
    0,0,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
    'z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',
    0,0,0,0,0,0,0,0,0,0,0,0,
    '7','8','9','-','4','5','6','+','1','2','3','0','.',
    0,0,0,0,0
};

static const char sc_shift[128] = {
    0,0,'!','@','#','$','%','^','&','*','(',')','+','_','\b','\t',
    'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,
    'A','S','D','F','G','H','J','K','L',':','"','~',0,'|',
    'Z','X','C','V','B','N','M','<','>','?',0,'*',0,' ',
    0,0,0,0,0,0,0,0,0,0,0,0,
    '7','8','9','-','4','5','6','+','1','2','3','0','.',
    0,0,0,0,0
};

static bool kfifo_put(kfifo_t* f, char c) {
    uint32_t next = (f->head + 1) % KFIFO_SIZE;
    if (next == f->tail) return false;
    f->buffer[f->head] = c;
    f->head = next;
    return true;
}

static bool kfifo_get(kfifo_t* f, char* c) {
    if (f->head == f->tail) return false;
    *c = f->buffer[f->tail];
    f->tail = (f->tail + 1) % KFIFO_SIZE;
    return true;
}

static void kb_callback(registers_t* r) {
    uint8_t sc;
    char c;
    UNUSED(r);

    sc = inb(0x60);

    if (sc == 0xE0) { 
        esc_prefix = 1; 
        return; 
    }

    if (sc & 0x80) {
        uint8_t rel = sc & 0x7F;
        if (rel == 0x2A || rel == 0x36) shift_down = 0;
        return;
    }

    if (sc == 0x2A || sc == 0x36) { shift_down = 1; return; }
    if (sc == 0x3A) { caps_lock = !caps_lock; return; }
    
    if (sc >= 128) return;

    if (esc_prefix) {
        esc_prefix = 0;
        if (sc == 0x48)      kfifo_put(&kfifo, '\x1B');
        else if (sc == 0x50) kfifo_put(&kfifo, '\x1C');
        else if (sc == 0x4B) kfifo_put(&kfifo, '\x1D');
        else if (sc == 0x4D) kfifo_put(&kfifo, '\x1E');
        return;
    }

    c = shift_down ? sc_shift[sc] : sc_normal[sc];
    if (!c) return;

    if (c >= 'a' && c <= 'z' && caps_lock) c = c - 'a' + 'A';
    else if (c >= 'A' && c <= 'Z' && caps_lock) c = c - 'A' + 'a';

    kfifo_put(&kfifo, c);
}

void keyboard_init(void) {
    uint8_t mask;
    kfifo.head = 0;
    kfifo.tail = 0;
    shift_down = 0;
    caps_lock = 0;
    isr_register_callback(33, kb_callback);
    mask = inb(0x21);
    mask &= ~(1 << 1);
    outb(0x21, mask);
}

char keyboard_getchar(void) {
    char c = 0;
    while (!kfifo_get(&kfifo, &c))
        __asm__ __volatile__("hlt");
    return c;
}

int keyboard_has_data(void) {
    return kfifo.head != kfifo.tail;
}

char keyboard_try_getchar(void) {
    char c = 0;
    kfifo_get(&kfifo, &c);
    return c;
}

static char hist[50][256];
static int hist_count = 0;
static int hist_pos = -1;

void keyboard_wait_line(char* buf, uint32_t max_len) {
    static char hist[16][128];
    static int hist_count = 0;
    static int hist_pos = -1; // -1 means we are on the current fresh line
    static char current_backup[128] = ""; // Saves what you typed before hitting Up Arrow

    uint32_t i = 0;
    buf[0] = '\0';

    while (1) {
        char c = keyboard_getchar();

        if (c == '\n') {
            buf[i] = '\0';
            vga_putc('\n');

            // Save to history if the command isn't empty and is different from the last one
            if (i > 0) {
                if (hist_count == 0 || strcmp(buf, hist[(hist_count - 1) % 16]) != 0) {
                    strncpy(hist[hist_count % 16], buf, 127);
                    hist[hist_count % 16][127] = '\0';
                    hist_count++;
                }
            }
            hist_pos = -1; // Reset history navigation
            current_backup[0] = '\0';
            return;
        }

        if (c == '\b') {
            if (i > 0) {
                i--;
                buf[i] = '\0';
                vga_putc('\b');
            }
        } 
        else if (c == '\x1B') { /* Arrow Up - Go backwards in time */
            if (hist_count > 0) {
                // If we are starting to look at history, backup our current edits
                if (hist_pos == -1) {
                    buf[i] = '\0';
                    strncpy(current_backup, buf, 127);
                }

                int total_available = hist_count > 16 ? 16 : hist_count;
                if (hist_pos < total_available - 1) {
                    hist_pos++;
                    
                    // Clear current characters from display
                    while (i > 0) { i--; vga_putc('\b'); }
                    
                    // Fetch from history slot
                    int idx = (hist_count - 1 - hist_pos) % 16;
                    if (idx < 0) idx += 16;

                    // Copy history command into string buffer and print it
                    for (int j = 0; hist[idx][j] && i < max_len - 1; j++) {
                        buf[i++] = hist[idx][j];
                        vga_putc(hist[idx][j]);
                    }
                    buf[i] = '\0';
                }
            }
        } 
        else if (c == '\x1C') { /* Arrow Down - Go forwards toward modern day */
            if (hist_pos >= 0) {
                hist_pos--;
                
                // Clear current characters from display
                while (i > 0) { i--; vga_putc('\b'); }
                i = 0;

                if (hist_pos == -1) {
                    // Restore what the user was originally typing before navigating away
                    for (int j = 0; current_backup[j] && i < max_len - 1; j++) {
                        buf[i++] = current_backup[j];
                        vga_putc(current_backup[j]);
                    }
                } else {
                    // Move to the next recent item in history
                    int idx = (hist_count - 1 - hist_pos) % 16;
                    if (idx < 0) idx += 16;
                    for (int j = 0; hist[idx][j] && i < max_len - 1; j++) {
                        buf[i++] = hist[idx][j];
                        vga_putc(hist[idx][j]);
                    }
                }
                buf[i] = '\0';
            }
        } 
        else {
            // Standard text entry
            if (i < max_len - 1 && c >= 32 && c <= 126) {
                buf[i++] = c;
                buf[i] = '\0';
                vga_putc(c);
            }
        }
    }
}