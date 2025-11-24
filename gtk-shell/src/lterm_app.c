#include "lterm_app.h"

#include "app_window.h"

void
lterm_app_activate(GtkApplication *app, gpointer user_data)
{
    (void)user_data;

    GtkWidget *window = lterm_app_window_new(app);
    gtk_widget_show(window);
}

