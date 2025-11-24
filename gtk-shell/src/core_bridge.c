#include "core_bridge.h"

#include <stdint.h>
#include <string.h>

#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#include "lterm_screen.h"
#include "lterm_pty.h"
#include "terminal_view.h"
#include "lterm_parser.h"
#include "lterm_token.h"
#include "lterm_token_types.h"

struct _CoreBridge {
    lterm_parser *parser;
    lterm_screen screen;
    lterm_pty pty;
    GtkWindow *window;
    GtkWidget *terminal_view;
    GtkWidget *title_label;
    GtkWidget *clipboard_label;
    GtkWidget *tmux_label;
    GIOChannel *pty_channel;
    guint pty_watch_id;
    gboolean using_screen;
};

static void parser_callback(const lterm_token *token, void *user_data);

static bool
pty_write_all(CoreBridge *bridge, const uint8_t *data, size_t length)
{
    if (!bridge || !data || !length || !lterm_pty_is_active(&bridge->pty)) {
        return false;
    }
    ssize_t written = lterm_pty_write(&bridge->pty, data, length);
    return written == (ssize_t)length;
}

static bool
send_bytes_with_alt(CoreBridge *bridge, const uint8_t *data, size_t length, bool alt_prefix)
{
    if (!bridge || !data || !length) {
        return false;
    }
    if (alt_prefix) {
        const uint8_t esc = 0x1b;
        if (!pty_write_all(bridge, &esc, 1)) {
            return false;
        }
    }
    return pty_write_all(bridge, data, length);
}

static bool
send_sequence(CoreBridge *bridge, const char *sequence, bool alt_prefix)
{
    if (!sequence) {
        return false;
    }
    return send_bytes_with_alt(bridge, (const uint8_t *)sequence, strlen(sequence), alt_prefix);
}

static void
attach_screen(CoreBridge *bridge)
{
    if (!bridge || !bridge->terminal_view) {
        return;
    }
    if (!bridge->using_screen) {
        terminal_view_set_screen(bridge->terminal_view, &bridge->screen);
        bridge->using_screen = TRUE;
    }
    gtk_widget_queue_draw(bridge->terminal_view);
}

static void
stop_pty(CoreBridge *bridge)
{
    if (!bridge) {
        return;
    }
    if (bridge->pty_watch_id) {
        g_source_remove(bridge->pty_watch_id);
        bridge->pty_watch_id = 0;
    }
    if (bridge->pty_channel) {
        g_io_channel_unref(bridge->pty_channel);
        bridge->pty_channel = NULL;
    }
    lterm_pty_close(&bridge->pty);
}

static gboolean
pty_watch_cb(GIOChannel *source, GIOCondition condition, gpointer user_data)
{
    CoreBridge *bridge = user_data;
    if (!bridge) {
        return G_SOURCE_REMOVE;
    }

    if (condition & (G_IO_HUP | G_IO_ERR)) {
        stop_pty(bridge);
        return G_SOURCE_REMOVE;
    }

    uint8_t buffer[4096];
    ssize_t n = lterm_pty_read(&bridge->pty, buffer, sizeof(buffer));
    if (n > 0) {
        lterm_parser_feed(bridge->parser, buffer, (size_t)n, parser_callback, bridge);
        gtk_widget_queue_draw(bridge->terminal_view);
    }

    return TRUE;
}

static const char *
describe_token_type(lterm_token_type type)
{
    switch (type) {
        case LTERM_TOKEN_OSC:
        case LTERM_TOKEN_XTERM_WIN_TITLE:
        case LTERM_TOKEN_XTERM_ICON_TITLE:
        case LTERM_TOKEN_XTERM_WINICON_TITLE:
            return "OSC";
        case LTERM_TOKEN_CLIPBOARD_CONTROL:
            return "Clipboard";
        case LTERM_TOKEN_DCS:
            return "DCS";
        case LTERM_TOKEN_ANSI_RIS:
            return "ANSI RIS";
        default:
            return "Token";
    }
}

static void
handle_osc_title(CoreBridge *bridge, const lterm_token *token)
{
    if (!bridge || (!bridge->title_label && !bridge->window)) {
        return;
    }
    char payload[256] = {0};
    if (token->ascii.length > 0 && token->ascii.buffer) {
        size_t copy = token->ascii.length < sizeof(payload) - 1
                          ? token->ascii.length
                          : sizeof(payload) - 1;
        memcpy(payload, token->ascii.buffer, copy);
    }
    const char *text = payload[0] ? payload : "(Untitled)";
    if (bridge->title_label) {
        gtk_label_set_text(GTK_LABEL(bridge->title_label), text);
    }
    if (bridge->window) {
        gtk_window_set_title(bridge->window, payload[0] ? payload : "lTerm2");
    }
}

static void
handle_clipboard(CoreBridge *bridge, const lterm_token *token)
{
    if (!bridge) {
        return;
    }
    char payload[256] = {0};
    if (token->ascii.length > 0 && token->ascii.buffer) {
        size_t copy = token->ascii.length < sizeof(payload) - 1
                          ? token->ascii.length
                          : sizeof(payload) - 1;
        memcpy(payload, token->ascii.buffer, copy);
    }
    if (bridge->clipboard_label) {
        gtk_label_set_text(GTK_LABEL(bridge->clipboard_label),
                           payload[0] ? payload : "(Clipboard updated)");
    }
    if (bridge->window && payload[0]) {
        GdkDisplay *display = gtk_widget_get_display(GTK_WIDGET(bridge->window));
        if (display) {
            GdkClipboard *clipboard = gdk_display_get_clipboard(display);
            if (clipboard) {
                if (g_str_has_prefix(payload, "c;")) {
                    const char *b64 = payload + 2;
                    gsize out_len = 0;
                    g_autofree guchar *decoded = g_base64_decode(b64, &out_len);
                    if (decoded && out_len > 0) {
                        gchar *text = g_strndup((const gchar *)decoded, out_len);
                        gdk_clipboard_set_text(clipboard, text);
                        g_free(text);
                    }
                } else {
                    gdk_clipboard_set_text(clipboard, payload);
                }
            }
        }
    }
}

static void
handle_tmux(CoreBridge *bridge, const lterm_token *token)
{
    if (!bridge || !bridge->tmux_label) {
        return;
    }
    char payload[256] = {0};
    if (token->ascii.length > 0 && token->ascii.buffer) {
        size_t copy = token->ascii.length < sizeof(payload) - 1
                          ? token->ascii.length
                          : sizeof(payload) - 1;
        memcpy(payload, token->ascii.buffer, copy);
    }
    gtk_label_set_text(GTK_LABEL(bridge->tmux_label),
                       payload[0] ? payload : "(tmux passthrough)");
}

static void
handle_text(CoreBridge *bridge, const lterm_token *token)
{
    if (!bridge || !bridge->terminal_view) {
        return;
    }
    if (bridge->using_screen) {
        gtk_widget_queue_draw(bridge->terminal_view);
        return;
    }
    if (token->ascii.length == 0 || !token->ascii.buffer) {
        return;
    }
    gchar *text = g_strndup((const gchar *)token->ascii.buffer, token->ascii.length);
    terminal_view_append_text(bridge->terminal_view, text);
    g_free(text);
}

static void
parser_callback(const lterm_token *token, void *user_data)
{
    CoreBridge *bridge = user_data;
    if (!bridge || token->type == LTERM_TOKEN_WAIT) {
        return;
    }
    switch (token->type) {
        case LTERM_TOKEN_XTERM_WIN_TITLE:
        case LTERM_TOKEN_XTERM_WINICON_TITLE:
        case LTERM_TOKEN_XTERM_ICON_TITLE:
            handle_osc_title(bridge, token);
            break;
        case LTERM_TOKEN_CLIPBOARD_CONTROL:
            handle_clipboard(bridge, token);
            break;
        case LTERM_TOKEN_TMUX:
        case LTERM_TOKEN_DCS:
            handle_tmux(bridge, token);
            break;
        default:
            handle_text(bridge, token);
            break;
    }
}

CoreBridge *
core_bridge_new(GtkWindow *window,
                GtkWidget *terminal_view,
                GtkWidget *title_label,
                GtkWidget *clipboard_label,
                GtkWidget *tmux_label)
{
    CoreBridge *bridge = g_new0(CoreBridge, 1);
    lterm_screen_init(&bridge->screen, 24, 80);
    lterm_pty_init(&bridge->pty);
    bridge->parser = lterm_parser_new(&bridge->screen);
    bridge->window = window;
    bridge->terminal_view = terminal_view;
    bridge->title_label = title_label;
    bridge->clipboard_label = clipboard_label;
    bridge->tmux_label = tmux_label;
    attach_screen(bridge);
    return bridge;
}

void
core_bridge_free(CoreBridge *bridge)
{
    if (!bridge) {
        return;
    }
    stop_pty(bridge);
    if (bridge->parser) {
        lterm_parser_free(bridge->parser);
    }
    lterm_screen_free(&bridge->screen);
    g_free(bridge);
}

void
core_bridge_feed_demo(CoreBridge *bridge)
{
    if (!bridge || !bridge->parser) {
        return;
    }

    const char *demo = "\x1b]0;lTerm2 GTK Shell\x07"
                       "Welcome to lTerm2 on Linux!\nEnjoy the new GTK shell.\n"
                       "\x1b]52;c;c29weQ==\x07"
                       "\x1bPtmux;session-ready%exit\x1b\\";
    lterm_parser_feed(bridge->parser,
                      (const uint8_t *)demo,
                      strlen(demo),
                      parser_callback,
                      bridge);
    attach_screen(bridge);
}

bool
core_bridge_start_shell(CoreBridge *bridge, const char *shell_path)
{
    if (!bridge) {
        return false;
    }

    stop_pty(bridge);

    if (!lterm_pty_spawn_shell(&bridge->pty, shell_path)) {
        g_warning("Failed to spawn shell for PTY session");
        return false;
    }

    bridge->pty_channel = g_io_channel_unix_new(lterm_pty_get_fd(&bridge->pty));
    g_io_channel_set_encoding(bridge->pty_channel, NULL, NULL);
    g_io_channel_set_buffered(bridge->pty_channel, FALSE);
    g_io_channel_set_close_on_unref(bridge->pty_channel, FALSE);
    bridge->pty_watch_id =
        g_io_add_watch(bridge->pty_channel, G_IO_IN | G_IO_HUP | G_IO_ERR, pty_watch_cb, bridge);

    attach_screen(bridge);
    return true;
}

static bool
handle_control_combo(uint8_t *buffer, size_t *length, guint keyval)
{
    if (keyval >= GDK_KEY_a && keyval <= GDK_KEY_z) {
        buffer[0] = (uint8_t)(keyval - GDK_KEY_a + 1);
        *length = 1;
        return true;
    }
    if (keyval >= GDK_KEY_A && keyval <= GDK_KEY_Z) {
        buffer[0] = (uint8_t)(keyval - GDK_KEY_A + 1);
        *length = 1;
        return true;
    }
    return false;
}

bool
core_bridge_handle_key(CoreBridge *bridge, guint keyval, GdkModifierType state)
{
    if (!bridge || !lterm_pty_is_active(&bridge->pty)) {
        return false;
    }

    const bool ctrl = (state & GDK_CONTROL_MASK) != 0;
    const bool alt = (state & GDK_ALT_MASK) != 0;
    uint8_t buffer[8] = {0};
    size_t length = 0;
    const char *sequence = NULL;

    if (ctrl && handle_control_combo(buffer, &length, keyval)) {
        return send_bytes_with_alt(bridge, buffer, length, alt);
    }

    switch (keyval) {
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
            buffer[0] = '\r';
            length = 1;
            break;
        case GDK_KEY_BackSpace:
            buffer[0] = 0x7F;
            length = 1;
            break;
        case GDK_KEY_Tab:
            buffer[0] = '\t';
            length = 1;
            break;
        case GDK_KEY_ISO_Left_Tab:
            sequence = "\x1b[Z";
            break;
        case GDK_KEY_Escape:
            buffer[0] = 0x1B;
            length = 1;
            break;
        case GDK_KEY_Up:
        case GDK_KEY_KP_Up:
            sequence = "\x1b[A";
            break;
        case GDK_KEY_Down:
        case GDK_KEY_KP_Down:
            sequence = "\x1b[B";
            break;
        case GDK_KEY_Right:
        case GDK_KEY_KP_Right:
            sequence = "\x1b[C";
            break;
        case GDK_KEY_Left:
        case GDK_KEY_KP_Left:
            sequence = "\x1b[D";
            break;
        case GDK_KEY_Home:
        case GDK_KEY_KP_Home:
            sequence = "\x1b[H";
            break;
        case GDK_KEY_End:
        case GDK_KEY_KP_End:
            sequence = "\x1b[F";
            break;
        case GDK_KEY_Delete:
        case GDK_KEY_KP_Delete:
            sequence = "\x1b[3~";
            break;
        case GDK_KEY_Insert:
        case GDK_KEY_KP_Insert:
            sequence = "\x1b[2~";
            break;
        case GDK_KEY_Page_Up:
        case GDK_KEY_KP_Page_Up:
            sequence = "\x1b[5~";
            break;
        case GDK_KEY_Page_Down:
        case GDK_KEY_KP_Page_Down:
            sequence = "\x1b[6~";
            break;
        default:
            break;
    }

    if (!length && !sequence) {
        gunichar ch = gdk_keyval_to_unicode(keyval);
        if (ch != 0 && !ctrl) {
            length = (size_t)g_unichar_to_utf8(ch, (char *)buffer);
        }
    }

    if (sequence) {
        return send_sequence(bridge, sequence, alt);
    }
    if (length) {
        return send_bytes_with_alt(bridge, buffer, length, alt);
    }
    return false;
}

