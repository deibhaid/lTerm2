#pragma once

#include <gtk/gtk.h>
#include "iterm_screen.h"

GtkWidget *terminal_view_new(void);
void terminal_view_append_text(GtkWidget *view, const char *text);
void terminal_view_set_screen(GtkWidget *view, const iterm_screen *screen);

