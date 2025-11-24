#include "terminal_view.h"

#include <pango/pangocairo.h>
#include <string.h>

typedef struct {
    GtkWidget *area;
    GString *buffer;
    PangoFontDescription *font;
    const lterm_screen *screen;
} TerminalView;

static void
color_from_index(uint8_t index, double *r, double *g, double *b)
{
    static const double basic[16][3] = {
        {0.0, 0.0, 0.0},       {0.5, 0.0, 0.0},       {0.0, 0.5, 0.0},       {0.5, 0.5, 0.0},
        {0.0, 0.0, 0.5},       {0.5, 0.0, 0.5},       {0.0, 0.5, 0.5},       {0.75, 0.75, 0.75},
        {0.5, 0.5, 0.5},       {1.0, 0.0, 0.0},       {0.0, 1.0, 0.0},       {1.0, 1.0, 0.0},
        {0.0, 0.0, 1.0},       {1.0, 0.0, 1.0},       {0.0, 1.0, 1.0},       {1.0, 1.0, 1.0},
    };
    static const double levels[6] = {0.0, 95.0/255.0, 135.0/255.0, 175.0/255.0, 215.0/255.0, 1.0};
    if (index < 16) {
        *r = basic[index][0];
        *g = basic[index][1];
        *b = basic[index][2];
        return;
    }
    if (index >= 16 && index <= 231) {
        uint8_t value = index - 16;
        uint8_t rr = value / 36;
        uint8_t gg = (value % 36) / 6;
        uint8_t bb = value % 6;
        *r = levels[rr];
        *g = levels[gg];
        *b = levels[bb];
        return;
    }
    if (index >= 232) {
        double gray = (8 + (index - 232) * 10) / 255.0;
        *r = gray;
        *g = gray;
        *b = gray;
        return;
    }
    *r = 0.8;
    *g = 0.8;
    *b = 0.8;
}

static void
terminal_view_click(GtkGestureClick *gesture,
                    int n_press,
                    double x,
                    double y,
                    gpointer user_data)
{
    (void)n_press;
    (void)x;
    (void)y;
    (void)user_data;
    GtkWidget *widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
    if (widget) {
        gtk_widget_grab_focus(widget);
    }
}

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
        pango_layout_set_text(layout, "M", -1);
        int char_width = 0;
        int char_height = 0;
        pango_layout_get_pixel_size(layout, &char_width, &char_height);
        if (char_width <= 0) {
            char_width = 10;
        }
        if (char_height <= 0) {
            char_height = 18;
        }
        const double padding_x = 6.0;
        const double padding_y = 6.0;

        for (size_t row = 0; row < view->screen->grid.rows; ++row) {
            for (size_t col = 0; col < view->screen->grid.cols; ++col) {
                const lterm_cell cell = view->screen->grid.cells[row * view->screen->grid.cols + col];
                double fg_r = 0.8, fg_g = 0.8, fg_b = 0.8;
                double bg_r = 0.07, bg_g = 0.07, bg_b = 0.07;
                color_from_index(cell.fg, &fg_r, &fg_g, &fg_b);
                color_from_index(cell.bg, &bg_r, &bg_g, &bg_b);

                const double x = padding_x + (double)col * char_width;
                const double y = padding_y + (double)row * char_height;
                cairo_save(cr);
                cairo_rectangle(cr, x, y, char_width, char_height);
                cairo_set_source_rgb(cr, bg_r, bg_g, bg_b);
                cairo_fill(cr);
                cairo_restore(cr);

                gunichar ch = cell.codepoint ? cell.codepoint : ' ';
                char utf8[8];
                gint len = g_unichar_to_utf8(ch, utf8);
                utf8[len] = '\0';
                pango_layout_set_text(layout, utf8, -1);

                PangoAttrList *attrs = NULL;
                if ((cell.flags & LTERM_CELL_FLAG_BOLD) || (cell.flags & LTERM_CELL_FLAG_UNDERLINE)) {
                    attrs = pango_attr_list_new();
                    if (cell.flags & LTERM_CELL_FLAG_BOLD) {
                        PangoAttribute *weight = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
                        weight->start_index = 0;
                        weight->end_index = G_MAXUINT;
                        pango_attr_list_insert(attrs, weight);
                    }
                    if (cell.flags & LTERM_CELL_FLAG_UNDERLINE) {
                        PangoAttribute *underline = pango_attr_underline_new(PANGO_UNDERLINE_SINGLE);
                        underline->start_index = 0;
                        underline->end_index = G_MAXUINT;
                        pango_attr_list_insert(attrs, underline);
                    }
                    pango_layout_set_attributes(layout, attrs);
                } else {
                    pango_layout_set_attributes(layout, NULL);
                }

                cairo_set_source_rgb(cr, fg_r, fg_g, fg_b);
                cairo_move_to(cr, x, y);
                pango_cairo_show_layout(cr, layout);

                if (attrs) {
                    pango_attr_list_unref(attrs);
                }
            }
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
    gtk_widget_set_focusable(area, TRUE);
    gtk_widget_set_focus_on_click(area, TRUE);
    GtkGesture *click = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), GDK_BUTTON_PRIMARY);
    g_signal_connect(click, "pressed", G_CALLBACK(terminal_view_click), NULL);
    gtk_widget_add_controller(area, GTK_EVENT_CONTROLLER(click));

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
terminal_view_set_screen(GtkWidget *widget, const lterm_screen *screen)
{
    TerminalView *view = g_object_get_data(G_OBJECT(widget), "terminal-view");
    if (!view) {
        return;
    }
    view->screen = screen;
    gtk_widget_queue_draw(widget);
}

