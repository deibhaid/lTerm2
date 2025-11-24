#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t codepoint;
    uint8_t fg;
    uint8_t bg;
    uint8_t flags;
} iterm_cell;

typedef struct {
    iterm_cell *data;
    size_t length;
    size_t capacity;
} iterm_scrollback;

typedef struct {
    size_t rows;
    size_t cols;
    iterm_cell *cells;
} iterm_screen_grid;

typedef struct {
    iterm_screen_grid grid;
    size_t cursor_row;
    size_t cursor_col;
    iterm_scrollback scrollback;
} iterm_screen;

void iterm_screen_init(iterm_screen *screen, size_t rows, size_t cols);
void iterm_screen_free(iterm_screen *screen);
void iterm_screen_clear(iterm_screen *screen);
void iterm_screen_put_text(iterm_screen *screen, const char *text);
void iterm_screen_move_cursor(iterm_screen *screen, int drow, int dcol);
void iterm_screen_set_cursor(iterm_screen *screen, size_t row, size_t col);
void iterm_screen_carriage_return(iterm_screen *screen);
void iterm_screen_line_feed(iterm_screen *screen);
void iterm_screen_set_size(iterm_screen *screen, size_t rows, size_t cols);
const iterm_scrollback *iterm_screen_scrollback(const iterm_screen *screen);

#ifdef __cplusplus
}
#endif

