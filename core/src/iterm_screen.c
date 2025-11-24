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
        cell->codepoint = (uint8_t)*text;
        cell->fg = 7;
        cell->bg = 0;
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

const iterm_scrollback *
iterm_screen_scrollback(const iterm_screen *screen)
{
    return screen ? &screen->scrollback : NULL;
}

