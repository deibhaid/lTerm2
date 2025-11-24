#include "core_bridge.h"

#include <stdint.h>
#include <string.h>

#include <glib.h>

#include "iterm_screen.h"
#include "terminal_view.h"
#include "iterm_parser.h"
#include "iterm_token.h"
#include "iterm_token_types.h"

struct _CoreBridge {
    iterm_parser *parser;
    iterm_screen screen;
    GtkWindow *window;
    GtkWidget *terminal_view;
    GtkWidget *title_label;
    GtkWidget *clipboard_label;
    GtkWidget *tmux_label;
};

static const char *
describe_token_type(iterm_token_type type)
{
    switch (type) {
        case ITERM_TOKEN_OSC:
        case ITERM_TOKEN_XTERM_WIN_TITLE:
        case ITERM_TOKEN_XTERM_ICON_TITLE:
        case ITERM_TOKEN_XTERM_WINICON_TITLE:
            return "OSC";
        case ITERM_TOKEN_CLIPBOARD_CONTROL:
            return "Clipboard";
        case ITERM_TOKEN_DCS:
            return "DCS";
        case ITERM_TOKEN_ANSI_RIS:
            return "ANSI RIS";
        default:
            return "Token";
    }
}

static void
handle_osc_title(CoreBridge *bridge, const iterm_token *token)
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
handle_clipboard(CoreBridge *bridge, const iterm_token *token)
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
handle_tmux(CoreBridge *bridge, const iterm_token *token)
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
handle_text(CoreBridge *bridge, const iterm_token *token)
{
    if (!bridge || !bridge->terminal_view) {
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
parser_callback(const iterm_token *token, void *user_data)
{
    CoreBridge *bridge = user_data;
    if (!bridge || token->type == ITERM_TOKEN_WAIT) {
        return;
    }
    switch (token->type) {
        case ITERM_TOKEN_XTERM_WIN_TITLE:
        case ITERM_TOKEN_XTERM_WINICON_TITLE:
        case ITERM_TOKEN_XTERM_ICON_TITLE:
            handle_osc_title(bridge, token);
            break;
        case ITERM_TOKEN_CLIPBOARD_CONTROL:
            handle_clipboard(bridge, token);
            break;
        case ITERM_TOKEN_TMUX:
        case ITERM_TOKEN_DCS:
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
    iterm_screen_init(&bridge->screen, 24, 80);
    bridge->parser = iterm_parser_new(&bridge->screen);
    bridge->window = window;
    bridge->terminal_view = terminal_view;
    bridge->title_label = title_label;
    bridge->clipboard_label = clipboard_label;
    bridge->tmux_label = tmux_label;
    return bridge;
}

void
core_bridge_free(CoreBridge *bridge)
{
    if (!bridge) {
        return;
    }
    if (bridge->parser) {
        iterm_parser_free(bridge->parser);
    }
    iterm_screen_free(&bridge->screen);
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
    iterm_parser_feed(bridge->parser,
                      (const uint8_t *)demo,
                      strlen(demo),
                      parser_callback,
                      bridge);
    terminal_view_set_screen(bridge->terminal_view, &bridge->screen);
}

