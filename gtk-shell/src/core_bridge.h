#pragma once

#include <gtk/gtk.h>

typedef struct _CoreBridge CoreBridge;

CoreBridge *core_bridge_new(GtkWindow *window,
                            GtkWidget *terminal_view,
                            GtkWidget *title_label,
                            GtkWidget *clipboard_label,
                            GtkWidget *tmux_label);
void core_bridge_free(CoreBridge *bridge);
void core_bridge_feed_demo(CoreBridge *bridge);
bool core_bridge_start_shell(CoreBridge *bridge, const char *shell_path);

