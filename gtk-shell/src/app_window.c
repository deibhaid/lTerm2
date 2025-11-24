#include "app_window.h"
#include "core_bridge.h"
#include "terminal_view.h"

static gboolean
terminal_key_pressed(GtkEventControllerKey *controller,
                     guint keyval,
                     guint keycode,
                     GdkModifierType state,
                     gpointer user_data)
{
    (void)controller;
    (void)keycode;
    CoreBridge *bridge = user_data;
    if (!bridge) {
        return GDK_EVENT_PROPAGATE;
    }
    if (core_bridge_handle_key(bridge, keyval, state)) {
        return GDK_EVENT_STOP;
    }
    return GDK_EVENT_PROPAGATE;
}

static GtkWidget *
create_placeholder(const char *title, GtkWidget **label_out)
{
    GtkWidget *frame = gtk_frame_new(NULL);

    GtkWidget *label = gtk_label_new(title);
    gtk_widget_add_css_class(label, "dim-label");
    if (label_out) {
        *label_out = label;
    }
    gtk_frame_set_child(GTK_FRAME(frame), label);
    return frame;
}

GtkWidget *
lterm_app_window_new(GtkApplication *app)
{
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_default_size(GTK_WINDOW(window), 1200, 720);
    gtk_window_set_title(GTK_WINDOW(window), "lTerm2 Preview");

    GtkWidget *header = gtk_header_bar_new();
    GtkWidget *header_title = gtk_label_new("lTerm2");
    gtk_widget_add_css_class(header_title, "title");
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(header), header_title);
    gtk_window_set_titlebar(GTK_WINDOW(window), header);

    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_top(content, 6);
    gtk_widget_set_margin_bottom(content, 6);
    gtk_widget_set_margin_start(content, 6);
    gtk_widget_set_margin_end(content, 6);
    gtk_window_set_child(GTK_WINDOW(window), content);

    GtkWidget *tab_strip = create_placeholder("Tab Strip Placeholder", NULL);
    gtk_box_append(GTK_BOX(content), tab_strip);

    GtkWidget *terminal = terminal_view_new();
    gtk_box_append(GTK_BOX(content), terminal);

    GtkWidget *status_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_box_append(GTK_BOX(content), status_box);

    GtkWidget *title_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(status_box), title_row);
    GtkWidget *title_caption = gtk_label_new("Window Title:");
    gtk_widget_add_css_class(title_caption, "dim-label");
    gtk_box_append(GTK_BOX(title_row), title_caption);
    GtkWidget *title_value = gtk_label_new("(Untitled)");
    gtk_box_append(GTK_BOX(title_row), title_value);

    GtkWidget *clipboard_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(status_box), clipboard_row);
    GtkWidget *clipboard_caption = gtk_label_new("Clipboard:");
    gtk_widget_add_css_class(clipboard_caption, "dim-label");
    gtk_box_append(GTK_BOX(clipboard_row), clipboard_caption);
    GtkWidget *clipboard_value = gtk_label_new("(Idle)");
    gtk_box_append(GTK_BOX(clipboard_row), clipboard_value);

    GtkWidget *tmux_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(status_box), tmux_row);
    GtkWidget *tmux_caption = gtk_label_new("Tmux Payload:");
    gtk_widget_add_css_class(tmux_caption, "dim-label");
    gtk_box_append(GTK_BOX(tmux_row), tmux_caption);
    GtkWidget *tmux_value = gtk_label_new("(No activity)");
    gtk_box_append(GTK_BOX(tmux_row), tmux_value);

    CoreBridge *bridge = core_bridge_new(GTK_WINDOW(window),
                                         terminal,
                                         title_value,
                                         clipboard_value,
                                         tmux_value);
    if (!core_bridge_start_shell(bridge, NULL)) {
        core_bridge_feed_demo(bridge);
    }
    GtkEventController *key_controller = gtk_event_controller_key_new();
    g_signal_connect(key_controller, "key-pressed", G_CALLBACK(terminal_key_pressed), bridge);
    gtk_widget_add_controller(terminal, key_controller);
    gtk_widget_grab_focus(terminal);
    g_object_set_data_full(G_OBJECT(window), "core-bridge", bridge, (GDestroyNotify)core_bridge_free);

    return window;
}

