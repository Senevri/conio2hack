#include <stdio.h>
#include <stdlib.h>
#include <conio2.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
/* tui.h */

typedef struct {
    int x, y;
} tui_point;

typedef struct {
    int width, height;
} tui_size;

typedef struct {
    unsigned char r, g, b;
} tui_color;

typedef struct {
    tui_color fg;
    tui_color bg;
    int bold;
    int underline;
} tui_style;

void tui_init(void);
void tui_shutdown(void);

void tui_clear(void);
void tui_goto(int x, int y);
void tui_put(int x, int y, wchar_t ch);
void tui_write(int x, int y, const wchar_t *text);

void tui_set_style(tui_style style);
void tui_reset_style(void);

tui_size tui_get_size(void);

void tui_draw_box(int x, int y, int width, int height);
void tui_draw_hline(int x, int y, int length);
void tui_draw_vline(int x, int y, int length);

void tui_fill_rect(int x, int y, int width, int height, wchar_t ch, tui_style style);

struct tui_cell {
    wchar_t ch;
    tui_style style;
};

struct tui_cell *screen;
struct tui_cell *previous_screen;