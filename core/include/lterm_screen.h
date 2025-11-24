#pragma once

#include <stddef.h>
#include <stdint.h>

#include "lterm_csi_param.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LTERM_CELL_FLAG_BOLD (1u << 0)
#define LTERM_CELL_FLAG_UNDERLINE (1u << 1)
#define LTERM_CELL_FLAG_INVERSE (1u << 2)

typedef struct {
    uint32_t codepoint;
    uint8_t fg;
    uint8_t bg;
    uint8_t flags;
} lterm_cell;

typedef struct {
    lterm_cell *data;
    size_t length;
    size_t capacity;
} lterm_scrollback;

typedef struct {
    size_t rows;
    size_t cols;
    lterm_cell *cells;
} lterm_screen_grid;

typedef struct {
    lterm_screen_grid grid;
    size_t cursor_row;
    size_t cursor_col;
    lterm_scrollback scrollback;
    uint8_t current_fg;
    uint8_t current_bg;
    uint16_t current_flags;
} lterm_screen;

void lterm_screen_init(lterm_screen *screen, size_t rows, size_t cols);
void lterm_screen_free(lterm_screen *screen);
void lterm_screen_clear(lterm_screen *screen);
void lterm_screen_put_text(lterm_screen *screen, const char *text);
void lterm_screen_move_cursor(lterm_screen *screen, int drow, int dcol);
void lterm_screen_set_cursor(lterm_screen *screen, size_t row, size_t col);
void lterm_screen_carriage_return(lterm_screen *screen);
void lterm_screen_line_feed(lterm_screen *screen);
void lterm_screen_set_size(lterm_screen *screen, size_t rows, size_t cols);
void lterm_screen_clear_line(lterm_screen *screen, int mode);
void lterm_screen_clear_screen(lterm_screen *screen, int mode);
void lterm_screen_reset_attributes(lterm_screen *screen);
void lterm_screen_apply_sgr(lterm_screen *screen, const lterm_csi_param *param);
const lterm_scrollback *lterm_screen_scrollback(const lterm_screen *screen);

#ifdef __cplusplus
}
#endif
