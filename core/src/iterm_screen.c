#include "iterm_screen.h"

#include <stdlib.h>
#include <string.h>

void
iterm_screen_init(iterm_screen *screen, size_t rows, size_t cols)
{
    if (!screen) {
        return;
    }
    screen->grid.rows = rows;
    screen->grid.cols = cols;
    screen->grid.cells = calloc(rows * cols, sizeof(iterm_cell));
    screen->cursor_row = 0;
    screen->cursor_col = 0;
    screen->scrollback.data = NULL;
    screen->scrollback.length = 0;
    screen->scrollback.capacity = 0;
    iterm_screen_reset_attributes(screen);
}

void
iterm_screen_set_size(iterm_screen *screen, size_t rows, size_t cols)
{
    if (!screen) {
        return;
    }
    free(screen->grid.cells);
    screen->grid.rows = rows;
    screen->grid.cols = cols;
    screen->grid.cells = calloc(rows * cols, sizeof(iterm_cell));
    screen->cursor_row = 0;
    screen->cursor_col = 0;
    iterm_screen_reset_attributes(screen);
}

void
iterm_screen_free(iterm_screen *screen)
{
    if (!screen) {
        return;
    }
    free(screen->grid.cells);
    screen->grid.cells = NULL;
    free(screen->scrollback.data);
    screen->scrollback.data = NULL;
    screen->scrollback.length = screen->scrollback.capacity = 0;
}

void
iterm_screen_clear(iterm_screen *screen)
{
    if (!screen || !screen->grid.cells) {
        return;
    }
    memset(screen->grid.cells, 0, screen->grid.rows * screen->grid.cols * sizeof(iterm_cell));
    screen->cursor_row = 0;
    screen->cursor_col = 0;
    iterm_screen_reset_attributes(screen);
}

void
iterm_screen_put_text(iterm_screen *screen, const char *text)
{
    if (!screen || !screen->grid.cells || !text) {
        return;
    }
    while (*text) {
        if (*text == '\n') {
            screen->cursor_row++;
            screen->cursor_col = 0;
            text++;
            continue;
        }
        if (screen->cursor_row >= screen->grid.rows) {
            size_t row_size = screen->grid.cols * sizeof(iterm_cell);
            if (screen->scrollback.length + screen->grid.cols > screen->scrollback.capacity) {
                size_t new_capacity = screen->scrollback.capacity ? screen->scrollback.capacity * 2 : screen->grid.cols * 32;
                screen->scrollback.data = realloc(screen->scrollback.data, new_capacity * sizeof(iterm_cell));
                screen->scrollback.capacity = new_capacity;
            }
            memcpy(screen->scrollback.data + screen->scrollback.length,
                   screen->grid.cells,
                   row_size);
            screen->scrollback.length += screen->grid.cols;
            memmove(screen->grid.cells,
                    screen->grid.cells + screen->grid.cols,
                    (screen->grid.rows - 1) * row_size);
            memset(screen->grid.cells + (screen->grid.rows - 1) * screen->grid.cols,
                   0,
                   row_size);
            screen->cursor_row = screen->grid.rows - 1;
        }
        if (screen->cursor_col >= screen->grid.cols) {
            screen->cursor_row++;
            screen->cursor_col = 0;
            continue;
        }
        iterm_cell *cell = &screen->grid.cells[screen->cursor_row * screen->grid.cols + screen->cursor_col];
        uint8_t fg = screen->current_fg;
        uint8_t bg = screen->current_bg;
        uint16_t flags = screen->current_flags;
        if (flags & ITERM_CELL_FLAG_INVERSE) {
            uint8_t tmp = fg;
            fg = bg;
            bg = tmp;
        }
        cell->codepoint = (uint8_t)*text;
        cell->fg = fg;
        cell->bg = bg;
        cell->flags = (uint8_t)(flags & 0xFF);
        screen->cursor_col++;
        text++;
    }
}

void
iterm_screen_move_cursor(iterm_screen *screen, int drow, int dcol)
{
    if (!screen) {
        return;
    }
    int new_row = (int)screen->cursor_row + drow;
    int new_col = (int)screen->cursor_col + dcol;
    if (new_col < 0) {
        new_col = 0;
    }
    if (new_row < 0) {
        new_row = 0;
    }
    if ((size_t)new_row >= screen->grid.rows) {
        new_row = screen->grid.rows - 1;
    }
    if ((size_t)new_col >= screen->grid.cols) {
        new_col = screen->grid.cols - 1;
    }
    screen->cursor_row = (size_t)new_row;
    screen->cursor_col = (size_t)new_col;
}

void
iterm_screen_set_cursor(iterm_screen *screen, size_t row, size_t col)
{
    if (!screen) {
        return;
    }
    if (row >= screen->grid.rows) {
        row = screen->grid.rows - 1;
    }
    if (col >= screen->grid.cols) {
        col = screen->grid.cols - 1;
    }
    screen->cursor_row = row;
    screen->cursor_col = col;
}

void
iterm_screen_carriage_return(iterm_screen *screen)
{
    if (!screen) {
        return;
    }
    screen->cursor_col = 0;
}

void
iterm_screen_line_feed(iterm_screen *screen)
{
    if (!screen) {
        return;
    }
    screen->cursor_row++;
    if (screen->cursor_row >= screen->grid.rows) {
        size_t row_size = screen->grid.cols * sizeof(iterm_cell);
        memmove(screen->grid.cells,
                screen->grid.cells + screen->grid.cols,
                (screen->grid.rows - 1) * row_size);
        memset(screen->grid.cells + (screen->grid.rows - 1) * screen->grid.cols,
               0,
               row_size);
        screen->cursor_row = screen->grid.rows - 1;
    }
}

static void clear_cells(iterm_screen *screen, size_t start, size_t count)
{
    if (!screen || !screen->grid.cells || count == 0) {
        return;
    }
    size_t total = screen->grid.rows * screen->grid.cols;
    if (start >= total) {
        return;
    }
    if (start + count > total) {
        count = total - start;
    }
    memset(screen->grid.cells + start, 0, count * sizeof(iterm_cell));
}

void iterm_screen_clear_line(iterm_screen *screen, int mode)
{
    if (!screen || !screen->grid.cells) {
        return;
    }
    size_t row = screen->cursor_row;
    if (row >= screen->grid.rows) {
        row = screen->grid.rows ? screen->grid.rows - 1 : 0;
    }
    size_t cols = screen->grid.cols;
    size_t from = 0;
    size_t to = cols;
    if (mode == 0) {
        from = screen->cursor_col;
    } else if (mode == 1) {
        to = screen->cursor_col + 1;
    } else {
        from = 0;
        to = cols;
    }
    if (from > cols) {
        from = cols;
    }
    if (to > cols) {
        to = cols;
    }
    if (from >= to) {
        return;
    }
    size_t start = row * cols + from;
    clear_cells(screen, start, to - from);
}

void iterm_screen_clear_screen(iterm_screen *screen, int mode)
{
    if (!screen || !screen->grid.cells) {
        return;
    }
    size_t cols = screen->grid.cols;
    size_t total = screen->grid.rows * cols;
    size_t cursor = screen->cursor_row * cols + screen->cursor_col;
    if (cursor > total) {
        cursor = total;
    }
    size_t start = 0;
    size_t count = total;
    switch (mode) {
        case 0:
            start = cursor;
            count = (cursor < total) ? total - cursor : 0;
            break;
        case 1:
            start = 0;
            count = cursor < total ? cursor + 1 : total;
            break;
        case 2:
        default:
            start = 0;
            count = total;
            screen->cursor_row = 0;
            screen->cursor_col = 0;
            break;
    }
    clear_cells(screen, start, count);
}

void iterm_screen_reset_attributes(iterm_screen *screen)
{
    if (!screen) {
        return;
    }
    screen->current_fg = 7;
    screen->current_bg = 0;
    screen->current_flags = 0;
}

void iterm_screen_apply_sgr(iterm_screen *screen, const iterm_csi_param *param)
{
    if (!screen) {
        return;
    }
    if (!param || param->count == 0) {
        iterm_screen_reset_attributes(screen);
        return;
    }
    for (int i = 0; i < param->count; ++i) {
        int value = param->p[i];
        if (value == -1) {
            value = 0;
        }
        if (value >= 30 && value <= 37) {
            screen->current_fg = (uint8_t)(value - 30);
            continue;
        }
        if (value >= 40 && value <= 47) {
            screen->current_bg = (uint8_t)(value - 40);
            continue;
        }
        if (value >= 90 && value <= 97) {
            screen->current_fg = (uint8_t)((value - 90) + 8);
            continue;
        }
        if (value >= 100 && value <= 107) {
            screen->current_bg = (uint8_t)((value - 100) + 8);
            continue;
        }
        switch (value) {
            case 0:
                iterm_screen_reset_attributes(screen);
                break;
            case 1:
                screen->current_flags |= ITERM_CELL_FLAG_BOLD;
                break;
            case 4:
                screen->current_flags |= ITERM_CELL_FLAG_UNDERLINE;
                break;
            case 7:
                screen->current_flags |= ITERM_CELL_FLAG_INVERSE;
                break;
            case 22:
                screen->current_flags &= ~ITERM_CELL_FLAG_BOLD;
                break;
            case 24:
                screen->current_flags &= ~ITERM_CELL_FLAG_UNDERLINE;
                break;
            case 27:
                screen->current_flags &= ~ITERM_CELL_FLAG_INVERSE;
                break;
            case 39:
                screen->current_fg = 7;
                break;
            case 49:
                screen->current_bg = 0;
                break;
            default:
                break;
        }
    }
}

const iterm_scrollback *
iterm_screen_scrollback(const iterm_screen *screen)
{
    return screen ? &screen->scrollback : NULL;
}

