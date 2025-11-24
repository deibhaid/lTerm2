#include "iterm_app.h"

#include "app_window.h"

void
iterm_app_activate(GtkApplication *app, gpointer user_data)
{
    (void)user_data;

    GtkWidget *window = iterm_app_window_new(app);
    gtk_widget_show(window);
}

