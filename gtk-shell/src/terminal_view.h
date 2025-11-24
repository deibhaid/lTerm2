#pragma once

#include <gtk/gtk.h>
#include "lterm_screen.h"

GtkWidget *terminal_view_new(void);
void terminal_view_append_text(GtkWidget *view, const char *text);
void terminal_view_set_screen(GtkWidget *view, const lterm_screen *screen);

