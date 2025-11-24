#pragma once

#include <stddef.h>
#include <stdint.h>

#include "iterm_csi_param.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ITERM_CELL_FLAG_BOLD (1u << 0)
#define ITERM_CELL_FLAG_UNDERLINE (1u << 1)
#define ITERM_CELL_FLAG_INVERSE (1u << 2)

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
    uint8_t current_fg;
    uint8_t current_bg;
    uint16_t current_flags;
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
void iterm_screen_clear_line(iterm_screen *screen, int mode);
void iterm_screen_clear_screen(iterm_screen *screen, int mode);
void iterm_screen_reset_attributes(iterm_screen *screen);
void iterm_screen_apply_sgr(iterm_screen *screen, const iterm_csi_param *param);
const iterm_scrollback *iterm_screen_scrollback(const iterm_screen *screen);

#ifdef __cplusplus
}
#endif
