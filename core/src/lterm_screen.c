#include "lterm_screen.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static void ensure_scrollback_capacity(lterm_screen *screen, size_t additional_cells);
static void scroll_up(lterm_screen *screen);
static void ensure_cursor_row(lterm_screen *screen);
static void write_codepoint(lterm_screen *screen, uint32_t codepoint);

static void ensure_scrollback_capacity(lterm_screen *screen, size_t additional_cells)
{
    if (!screen) {
        return;
    }
    size_t needed = screen->scrollback.length + additional_cells;
    if (needed <= screen->scrollback.capacity) {
        return;
    }
    size_t cols = screen->grid.cols ? screen->grid.cols : 1;
    size_t new_capacity = screen->scrollback.capacity ? screen->scrollback.capacity : cols * 32;
    while (new_capacity < needed) {
        new_capacity *= 2;
    }
    lterm_cell *new_data = realloc(screen->scrollback.data, new_capacity * sizeof(lterm_cell));
    if (!new_data) {
        return;
    }
    screen->scrollback.data = new_data;
    screen->scrollback.capacity = new_capacity;
}

static void scroll_up(lterm_screen *screen)
{
    if (!screen || !screen->grid.cells || screen->grid.rows == 0 || screen->grid.cols == 0) {
        return;
    }
    size_t cols = screen->grid.cols;
    size_t row_size = cols * sizeof(lterm_cell);
    ensure_scrollback_capacity(screen, cols);
    if (screen->scrollback.data && screen->scrollback.length + cols <= screen->scrollback.capacity) {
        memcpy(screen->scrollback.data + screen->scrollback.length,
               screen->grid.cells,
               row_size);
        screen->scrollback.length += cols;
    }
    if (screen->grid.rows > 1) {
        memmove(screen->grid.cells,
                screen->grid.cells + cols,
                (screen->grid.rows - 1) * row_size);
    }
    memset(screen->grid.cells + (screen->grid.rows - 1) * cols, 0, row_size);
}

static void ensure_cursor_row(lterm_screen *screen)
{
    if (!screen || !screen->grid.cells || screen->grid.rows == 0) {
        screen->cursor_row = 0;
        return;
    }
    while (screen->cursor_row >= screen->grid.rows) {
        scroll_up(screen);
        if (screen->grid.rows == 0) {
            break;
        }
        if (screen->cursor_row > 0) {
            screen->cursor_row--;
        } else {
            break;
        }
    }
    if (screen->cursor_row >= screen->grid.rows) {
        screen->cursor_row = screen->grid.rows - 1;
    }
}

static void write_codepoint(lterm_screen *screen, uint32_t codepoint)
{
    if (!screen || !screen->grid.cells || screen->grid.cols == 0) {
        return;
    }
    if (screen->cursor_col >= screen->grid.cols) {
        screen->cursor_col = 0;
        screen->cursor_row++;
    }
    ensure_cursor_row(screen);
    if (!screen->grid.cells) {
        return;
    }
    size_t index = screen->cursor_row * screen->grid.cols + screen->cursor_col;
    uint8_t fg = screen->current_fg;
    uint8_t bg = screen->current_bg;
    uint16_t flags = screen->current_flags;
    if (flags & LTERM_CELL_FLAG_INVERSE) {
        uint8_t tmp = fg;
        fg = bg;
        bg = tmp;
    }
    lterm_cell *cell = &screen->grid.cells[index];
    cell->codepoint = codepoint ? codepoint : ' ';
    cell->fg = fg;
    cell->bg = bg;
    cell->flags = (uint8_t)(flags & 0xFF);
    screen->cursor_col++;
    if (screen->cursor_col >= screen->grid.cols) {
        screen->cursor_col = 0;
        screen->cursor_row++;
    }
}

void
lterm_screen_init(lterm_screen *screen, size_t rows, size_t cols)
{
    if (!screen) {
        return;
    }
    screen->grid.rows = rows;
    screen->grid.cols = cols;
    screen->grid.cells = calloc(rows * cols, sizeof(lterm_cell));
    screen->cursor_row = 0;
    screen->cursor_col = 0;
    screen->scrollback.data = NULL;
    screen->scrollback.length = 0;
    screen->scrollback.capacity = 0;
    lterm_screen_reset_attributes(screen);
}

void
lterm_screen_set_size(lterm_screen *screen, size_t rows, size_t cols)
{
    if (!screen) {
        return;
    }
    if (rows == 0) {
        rows = 1;
    }
    if (cols == 0) {
        cols = 1;
    }
    if (rows == screen->grid.rows && cols == screen->grid.cols) {
        return;
    }
    lterm_cell *new_cells = calloc(rows * cols, sizeof(lterm_cell));
    if (!new_cells) {
        return;
    }
    if (screen->grid.cells) {
        size_t copy_rows = screen->grid.rows < rows ? screen->grid.rows : rows;
        size_t copy_cols = screen->grid.cols < cols ? screen->grid.cols : cols;
        for (size_t r = 0; r < copy_rows; ++r) {
            memcpy(new_cells + r * cols,
                   screen->grid.cells + r * screen->grid.cols,
                   copy_cols * sizeof(lterm_cell));
        }
        free(screen->grid.cells);
    }
    screen->grid.cells = new_cells;
    screen->grid.rows = rows;
    screen->grid.cols = cols;
    if (screen->cursor_row >= rows) {
        screen->cursor_row = rows - 1;
    }
    if (screen->cursor_col >= cols) {
        screen->cursor_col = cols - 1;
    }
}

void
lterm_screen_free(lterm_screen *screen)
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
lterm_screen_clear(lterm_screen *screen)
{
    if (!screen || !screen->grid.cells) {
        return;
    }
    memset(screen->grid.cells, 0, screen->grid.rows * screen->grid.cols * sizeof(lterm_cell));
    screen->cursor_row = 0;
    screen->cursor_col = 0;
    lterm_screen_reset_attributes(screen);
}

void
lterm_screen_put_text(lterm_screen *screen, const char *text)
{
    if (!screen || !screen->grid.cells || !text) {
        return;
    }
    while (*text) {
        unsigned char ch = (unsigned char)*text++;
        switch (ch) {
            case '\r':
                screen->cursor_col = 0;
                break;
            case '\n':
                lterm_screen_line_feed(screen);
                screen->cursor_col = 0;
                break;
            case '\t': {
                if (screen->grid.cols == 0) {
                    break;
                }
                const size_t tab = 8;
                size_t offset = screen->cursor_col % tab;
                size_t advance = tab - offset;
                if (advance == 0) {
                    advance = tab;
                }
                for (size_t i = 0; i < advance; ++i) {
                    write_codepoint(screen, ' ');
                }
                break;
            }
            default:
                if (ch < 0x20 && ch != 0x1B) {
                    break;
                }
                write_codepoint(screen, ch);
                break;
        }
    }
}

void
lterm_screen_move_cursor(lterm_screen *screen, int drow, int dcol)
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
lterm_screen_set_cursor(lterm_screen *screen, size_t row, size_t col)
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
lterm_screen_carriage_return(lterm_screen *screen)
{
    if (!screen) {
        return;
    }
    screen->cursor_col = 0;
}

void
lterm_screen_line_feed(lterm_screen *screen)
{
    if (!screen) {
        return;
    }
    screen->cursor_row++;
    if (screen->cursor_row >= screen->grid.rows) {
        scroll_up(screen);
        if (screen->grid.rows > 0) {
            screen->cursor_row = screen->grid.rows - 1;
        } else {
            screen->cursor_row = 0;
        }
    }
}

static void clear_cells(lterm_screen *screen, size_t start, size_t count)
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
    memset(screen->grid.cells + start, 0, count * sizeof(lterm_cell));
}

void lterm_screen_clear_line(lterm_screen *screen, int mode)
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

void lterm_screen_clear_screen(lterm_screen *screen, int mode)
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

void lterm_screen_reset_attributes(lterm_screen *screen)
{
    if (!screen) {
        return;
    }
    screen->current_fg = 7;
    screen->current_bg = 0;
    screen->current_flags = 0;
}

void lterm_screen_apply_sgr(lterm_screen *screen, const lterm_csi_param *param)
{
    if (!screen) {
        return;
    }
    if (!param || param->count == 0) {
        lterm_screen_reset_attributes(screen);
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
                lterm_screen_reset_attributes(screen);
                break;
            case 1:
                screen->current_flags |= LTERM_CELL_FLAG_BOLD;
                break;
            case 4:
                screen->current_flags |= LTERM_CELL_FLAG_UNDERLINE;
                break;
            case 7:
                screen->current_flags |= LTERM_CELL_FLAG_INVERSE;
                break;
            case 22:
                screen->current_flags &= ~LTERM_CELL_FLAG_BOLD;
                break;
            case 24:
                screen->current_flags &= ~LTERM_CELL_FLAG_UNDERLINE;
                break;
            case 27:
                screen->current_flags &= ~LTERM_CELL_FLAG_INVERSE;
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

const lterm_scrollback *
lterm_screen_scrollback(const lterm_screen *screen)
{
    return screen ? &screen->scrollback : NULL;
}

