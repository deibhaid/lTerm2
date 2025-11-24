#pragma once

#include <gtk/gtk.h>
#include "lterm_screen.h"

typedef void (*terminal_view_resize_cb)(size_t cols, size_t rows, void *user_data);

GtkWidget *terminal_view_new(void);
void terminal_view_append_text(GtkWidget *view, const char *text);
void terminal_view_set_screen(GtkWidget *view, lterm_screen *screen);
void terminal_view_set_resize_callback(GtkWidget *view,
                                       terminal_view_resize_cb callback,
                                       void *user_data);
