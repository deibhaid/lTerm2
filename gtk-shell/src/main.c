#include <gtk/gtk.h>

#include "iterm_app.h"

int
main(int argc, char *argv[])
{
    GtkApplication *app = gtk_application_new("com.lterm2.GtkApp", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(iterm_app_activate), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}

