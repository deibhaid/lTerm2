#include "terminal_view.h"

#include <pango/pangocairo.h>
#include <string.h>

typedef struct {
    GtkWidget *area;
    GString *buffer;
    PangoFontDescription *font;
    const iterm_screen *screen;
} TerminalView;

static void
terminal_view_destroy(gpointer data)
{
    TerminalView *view = data;
    if (!view) {
        return;
    }
    if (view->font) {
        pango_font_description_free(view->font);
    }
    if (view->buffer) {
        g_string_free(view->buffer, TRUE);
    }
    g_free(view);
}

static void
terminal_view_draw(GtkDrawingArea *area,
                   cairo_t *cr,
                   int width,
                   int height,
                   gpointer user_data)
{
    TerminalView *view = user_data;
    if (!view) {
        return;
    }

    cairo_set_source_rgb(cr, 0.07, 0.07, 0.07);
    cairo_paint(cr);

    cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);

    if (view->screen && view->screen->grid.cells) {
        PangoLayout *layout = gtk_widget_create_pango_layout(GTK_WIDGET(area), "");
        if (view->font) {
            pango_layout_set_font_description(layout, view->font);
        }
        for (size_t row = 0; row < view->screen->grid.rows; ++row) {
            GString *line = g_string_new("");
            for (size_t col = 0; col < view->screen->grid.cols; ++col) {
                iterm_cell cell = view->screen->grid.cells[row * view->screen->grid.cols + col];
                gunichar ch = cell.codepoint ? cell.codepoint : ' ';
                g_string_append_unichar(line, ch);
            }
            pango_layout_set_text(layout, line->str, -1);
            cairo_move_to(cr, 6, 6 + row * 18);
            pango_cairo_show_layout(cr, layout);
            g_string_free(line, TRUE);
        }
        g_object_unref(layout);
        return;
    }

    if (view->buffer && view->buffer->len > 0) {
        PangoLayout *layout = gtk_widget_create_pango_layout(GTK_WIDGET(area), view->buffer->str);
        pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
        pango_layout_set_width(layout, width * PANGO_SCALE);
        if (view->font) {
            pango_layout_set_font_description(layout, view->font);
        }
        cairo_move_to(cr, 6, 6);
        pango_cairo_show_layout(cr, layout);
        g_object_unref(layout);
    }
}

GtkWidget *
terminal_view_new(void)
{
    TerminalView *view = g_new0(TerminalView, 1);
    view->buffer = g_string_new("");
    view->font = pango_font_description_from_string("Monospace 12");

    GtkWidget *area = gtk_drawing_area_new();
    gtk_widget_add_css_class(area, "terminal-view");
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(area), terminal_view_draw, view, terminal_view_destroy);
    gtk_widget_set_hexpand(area, TRUE);
    gtk_widget_set_vexpand(area, TRUE);

    view->area = area;
    g_object_set_data_full(G_OBJECT(area), "terminal-view", view, NULL);
    return area;
}

void
terminal_view_append_text(GtkWidget *widget, const char *text)
{
    if (!widget || !text) {
        return;
    }
    TerminalView *view = g_object_get_data(G_OBJECT(widget), "terminal-view");
    if (!view || !view->buffer) {
        return;
    }
    g_string_append(view->buffer, text);
    if (text[strlen(text) - 1] != '\n') {
        g_string_append_c(view->buffer, '\n');
    }
    gtk_widget_queue_draw(widget);
}

void
terminal_view_set_screen(GtkWidget *widget, const iterm_screen *screen)
{
    TerminalView *view = g_object_get_data(G_OBJECT(widget), "terminal-view");
    if (!view) {
        return;
    }
    view->screen = screen;
    gtk_widget_queue_draw(widget);
}

