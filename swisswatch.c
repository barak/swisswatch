/* swisswatch.c — Swiss Watch clock using GTK4 and Cairo */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>

typedef enum { SHAPE_TRIANGLE, SHAPE_RECTANGLE, SHAPE_ARROW, SHAPE_CIRCLE } ShapeStyle;
typedef enum { RENDER_OUTLINE, RENDER_FILL } RenderType;
typedef enum { CHILD_HAND, CHILD_MARK } ChildType;

typedef struct {
    ChildType  type;
    double     inner, outer, width, phase;
    int        cycle;
    double     stroke_width_r;
    ShapeStyle shape;
    RenderType render;
    double     cx, cy;
    double     r, g, b;
} Child;

typedef struct {
    GtkWidget *window;
    GtkWidget *drawing_area;
    gboolean   railroad;
    gboolean   circular;
    gboolean   shaped;        /* undecorated, transparent, round window */
    gboolean   is_fullscreen;
    int        tick_ms;
    Child     *children;
    int        n_children;
    long       now_local_sec;
    double     frac_sec;
    double     bg_r, bg_g, bg_b;
    guint      timer_id;
} AppState;

/* Coordinate macros: origin at clock centre, y-axis up, units = fraction of radius */
#define SX(x) ((child->cx + (x)) * rad_x)
#define SY(y) (-(child->cy + (y)) * rad_y)

static void
draw_shape(cairo_t *cr, const Child *child, double rad_x, double rad_y,
           double s, double c)
{
    double sw = child->stroke_width_r * sqrt(rad_x * rad_y);
    if (sw < 1.0) sw = 1.0;

    cairo_set_source_rgb(cr, child->r, child->g, child->b);
    cairo_new_path(cr);

    switch (child->shape) {
    case SHAPE_TRIANGLE:
        if (child->width < 1e-3) {
            cairo_move_to(cr, SX(c * child->inner), SY(s * child->inner));
            cairo_line_to(cr, SX(c * child->outer), SY(s * child->outer));
            cairo_set_line_width(cr, sw);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
            cairo_stroke(cr);
        } else {
            cairo_move_to(cr, SX(c * child->outer), SY(s * child->outer));
            cairo_line_to(cr, SX(c * child->inner - s * child->width / 2),
                              SY(s * child->inner + c * child->width / 2));
            cairo_line_to(cr, SX(c * child->inner + s * child->width / 2),
                              SY(s * child->inner - c * child->width / 2));
            if (child->render == RENDER_OUTLINE) {
                cairo_close_path(cr);
                cairo_set_line_width(cr, sw);
                cairo_stroke(cr);
            } else {
                cairo_fill(cr);
            }
        }
        break;

    case SHAPE_RECTANGLE:
    case SHAPE_ARROW:
        if (child->width < 1e-3) {
            cairo_move_to(cr, SX(c * child->inner), SY(s * child->inner));
            cairo_line_to(cr, SX(c * child->outer), SY(s * child->outer));
            cairo_set_line_width(cr, sw);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
            cairo_stroke(cr);
        } else {
            cairo_move_to(cr, SX(c * child->inner - s * child->width / 2),
                              SY(s * child->inner + c * child->width / 2));
            cairo_line_to(cr, SX(c * child->inner + s * child->width / 2),
                              SY(s * child->inner - c * child->width / 2));
            cairo_line_to(cr, SX(c * child->outer + s * child->width / 2),
                              SY(s * child->outer - c * child->width / 2));
            if (child->shape == SHAPE_ARROW)
                cairo_line_to(cr, SX(c * (child->outer + child->width / 2)),
                                  SY(s * (child->outer + child->width / 2)));
            cairo_line_to(cr, SX(c * child->outer - s * child->width / 2),
                              SY(s * child->outer + c * child->width / 2));
            if (child->render == RENDER_OUTLINE) {
                cairo_close_path(cr);
                cairo_set_line_width(cr, sw);
                cairo_stroke(cr);
            } else {
                cairo_fill(cr);
            }
        }
        break;

    case SHAPE_CIRCLE: {
        double ccx = SX(c * child->outer);
        double ccy = SY(s * child->outer);
        double rx = rad_x * child->width / 2;
        double ry = rad_y * child->width / 2;
        if (rx < 0.5 || ry < 0.5) break;
        cairo_save(cr);
        cairo_translate(cr, ccx, ccy);
        cairo_scale(cr, rx, ry);
        cairo_arc(cr, 0, 0, 1.0, 0, 2 * G_PI);
        cairo_restore(cr);
        if (child->render == RENDER_OUTLINE) {
            cairo_set_line_width(cr, sw);
            cairo_stroke(cr);
        } else {
            cairo_fill(cr);
        }
        break;
    }
    }
}

#undef SX
#undef SY

static void
draw_mark(cairo_t *cr, const Child *child, double rad_x, double rad_y)
{
    for (int k = 0; k < child->cycle; k++) {
        double ang = G_PI_2 - ((double)k / child->cycle * 2 * G_PI);
        draw_shape(cr, child, rad_x, rad_y, sin(ang), cos(ang));
    }
}

static void
draw_hand(cairo_t *cr, const Child *child, long now_local_sec, double frac_sec,
          gboolean railroad, double rad_x, double rad_y)
{
    double dnow = (double)now_local_sec + frac_sec;
    double ang;

    if (railroad && child->cycle < 464) {
        /* Second hand: sweeps 62/60 of a circle in 57.5s, then snaps to 12 */
        double pos = fmod(dnow - child->phase, 60.0);
        if (pos < 0) pos += 60.0;
        ang = (pos < 57.5) ? (pos * 2 * G_PI * 62) / 3600.0 : 0.0;
    } else if (railroad) {
        /* Minute/hour: snap to whole-minute boundaries */
        double t = dnow - child->phase;
        t -= fmod(t, 60.0) - 0.0001;
        ang = fmod(t, (double)child->cycle) * 2 * G_PI / child->cycle;
    } else {
        double pos = fmod(dnow - child->phase, (double)child->cycle);
        if (pos < 0) pos += child->cycle;
        ang = pos * 2 * G_PI / child->cycle;
    }

    draw_shape(cr, child, rad_x, rad_y, sin(G_PI_2 - ang), cos(G_PI_2 - ang));
}

static void
draw_watch(GtkDrawingArea *widget, cairo_t *cr, int width, int height, gpointer data)
{
    AppState *state = data;
    (void)widget;

    double cx = width / 2.0;
    double cy = height / 2.0;
    double rad_x, rad_y;

    if (state->circular)
        rad_x = rad_y = MIN(cx, cy);
    else
        rad_x = cx, rad_y = cy;

    if (state->shaped) {
        /* Clear entire surface to transparent, then clip to the clock face */
        cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
        cairo_paint(cr);
        cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

        cairo_save(cr);
        cairo_translate(cr, cx, cy);
        cairo_scale(cr, rad_x, rad_y);
        cairo_arc(cr, 0, 0, 1.0, 0, 2 * G_PI);
        cairo_restore(cr);
        cairo_clip(cr);
    }

    cairo_set_source_rgb(cr, state->bg_r, state->bg_g, state->bg_b);
    cairo_paint(cr);

    cairo_save(cr);
    cairo_translate(cr, cx, cy);

    for (int i = 0; i < state->n_children; i++)
        if (state->children[i].type == CHILD_MARK)
            draw_mark(cr, &state->children[i], rad_x, rad_y);

    for (int i = 0; i < state->n_children; i++)
        if (state->children[i].type == CHILD_HAND)
            draw_hand(cr, &state->children[i], state->now_local_sec,
                      state->frac_sec, state->railroad, rad_x, rad_y);

    cairo_restore(cr);
}

static void
update_time(AppState *state)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm *tm = localtime(&tv.tv_sec);
#ifdef HAVE_GMTOFF
    state->now_local_sec = tv.tv_sec + tm->tm_gmtoff;
#else
    state->now_local_sec = tm->tm_sec + tm->tm_min * 60
                         + tm->tm_hour * 3600 + (long)tm->tm_mday * 86400;
#endif
    state->frac_sec = tv.tv_usec * 1e-6;
}

static gboolean
on_tick(gpointer data)
{
    AppState *state = data;
    update_time(state);
    gtk_widget_queue_draw(state->drawing_area);
    return G_SOURCE_CONTINUE;
}

/* --- Window management --- */

static void
step_size(AppState *state, int delta)
{
    if (state->is_fullscreen) return;
    int w = gtk_widget_get_width(GTK_WIDGET(state->window));
    int h = gtk_widget_get_height(GTK_WIDGET(state->window));
    int new_size = MAX(50, MIN(w, h) + delta);
    /* Clear size request first so the window can shrink, then set new size */
    gtk_widget_set_size_request(state->drawing_area, -1, -1);
    gtk_window_set_default_size(GTK_WINDOW(state->window), new_size, new_size);
}

static void
toggle_fullscreen(AppState *state)
{
    state->is_fullscreen = !state->is_fullscreen;
    if (state->is_fullscreen)
        gtk_window_fullscreen(GTK_WINDOW(state->window));
    else
        gtk_window_unfullscreen(GTK_WINDOW(state->window));
}

static void
toggle_shaped(AppState *state)
{
    state->shaped = !state->shaped;
    gtk_window_set_decorated(GTK_WINDOW(state->window), !state->shaped);
    if (state->shaped)
        gtk_widget_add_css_class(GTK_WIDGET(state->window), "swisswatch-shaped");
    else
        gtk_widget_remove_css_class(GTK_WIDGET(state->window), "swisswatch-shaped");
    gtk_widget_queue_draw(state->drawing_area);
}

static void
toggle_circular(AppState *state)
{
    state->circular = !state->circular;
    gtk_widget_queue_draw(state->drawing_area);
}

static void
toggle_railroad(AppState *state)
{
    state->railroad = !state->railroad;
    gtk_widget_queue_draw(state->drawing_area);
}

static gboolean
on_key_pressed(GtkEventController *ctrl, guint keyval, guint keycode,
               GdkModifierType mods, gpointer data)
{
    AppState *state = data;
    (void)ctrl; (void)keycode; (void)mods;

    switch (keyval) {
    case GDK_KEY_Escape:
    case GDK_KEY_q:
    case GDK_KEY_Q:
        gtk_window_close(GTK_WINDOW(state->window));
        return TRUE;
    case GDK_KEY_plus:
    case GDK_KEY_equal:
    case GDK_KEY_KP_Add:
        step_size(state, +50);
        return TRUE;
    case GDK_KEY_minus:
    case GDK_KEY_underscore:
    case GDK_KEY_KP_Subtract:
        step_size(state, -50);
        return TRUE;
    case GDK_KEY_f:
    case GDK_KEY_F:
    case GDK_KEY_F11:
        toggle_fullscreen(state);
        return TRUE;
    case GDK_KEY_s:
    case GDK_KEY_S:
        toggle_shaped(state);
        return TRUE;
    case GDK_KEY_c:
    case GDK_KEY_C:
        toggle_circular(state);
        return TRUE;
    case GDK_KEY_r:
    case GDK_KEY_R:
        toggle_railroad(state);
        return TRUE;
    }
    return FALSE;
}

static void
on_activate(GtkApplication *app, gpointer user_data)
{
    AppState *state = user_data;

    state->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(state->window), "SwissWatch");
    gtk_window_set_default_size(GTK_WINDOW(state->window), 300, 300);

    state->drawing_area = gtk_drawing_area_new();
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(state->drawing_area),
                                   draw_watch, state, NULL);

    /* CSS installed once so toggling shaped at runtime only needs to
     * add/remove the CSS class and flip gtk_window_set_decorated */
    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css,
        "window.swisswatch-shaped { background: transparent; }");
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    /* GtkWindowHandle enables drag-to-move when undecorated; harmless when decorated */
    GtkWidget *handle = gtk_window_handle_new();
    gtk_window_handle_set_child(GTK_WINDOW_HANDLE(handle), state->drawing_area);
    gtk_window_set_child(GTK_WINDOW(state->window), handle);

    gtk_window_set_decorated(GTK_WINDOW(state->window), !state->shaped);
    if (state->shaped)
        gtk_widget_add_css_class(GTK_WIDGET(state->window), "swisswatch-shaped");

    /* Keyboard shortcuts: capture phase so they fire regardless of focus */
    GtkEventController *key_ctrl = gtk_event_controller_key_new();
    gtk_event_controller_set_propagation_phase(
        GTK_EVENT_CONTROLLER(key_ctrl), GTK_PHASE_CAPTURE);
    gtk_widget_add_controller(GTK_WIDGET(state->window),
                              GTK_EVENT_CONTROLLER(key_ctrl));
    g_signal_connect(key_ctrl, "key-pressed", G_CALLBACK(on_key_pressed), state);

    update_time(state);
    state->timer_id = g_timeout_add(state->tick_ms, on_tick, state);

    if (state->is_fullscreen)
        gtk_window_fullscreen(GTK_WINDOW(state->window));

    gtk_window_present(GTK_WINDOW(state->window));
}

/*
 * Swiss Railway Clock (SBB/CFF/FFS) — the default configuration.
 * Dimensions from the original SWatch.ad application defaults file.
 */
static const Child swisswatch_children[] = {
    /* Hour marks */
    { CHILD_MARK, .inner=.725, .outer=.971, .width=0,     .cycle=12,    .stroke_width_r=.072,
      .shape=SHAPE_RECTANGLE, .render=RENDER_OUTLINE, .r=0,  .g=0,  .b=0  },
    /* Minute marks */
    { CHILD_MARK, .inner=.899, .outer=.971, .width=0,     .cycle=60,    .stroke_width_r=.029,
      .shape=SHAPE_RECTANGLE, .render=RENDER_OUTLINE, .r=0,  .g=0,  .b=0  },
    /* Clock bezel */
    { CHILD_MARK, .inner=0,    .outer=0,    .width=1.999, .cycle=1,     .stroke_width_r=.029,
      .shape=SHAPE_CIRCLE,    .render=RENDER_OUTLINE, .r=.5, .g=.5, .b=.5 },
    /* Hour hand */
    { CHILD_HAND, .inner=-.232,.outer=.638, .width=.116,  .cycle=43200, .stroke_width_r=0,
      .shape=SHAPE_RECTANGLE, .render=RENDER_FILL,    .r=0,  .g=0,  .b=0  },
    /* Minute hand */
    { CHILD_HAND, .inner=-.232,.outer=.928, .width=.079,  .cycle=3600,  .stroke_width_r=0,
      .shape=SHAPE_RECTANGLE, .render=RENDER_FILL,    .r=0,  .g=0,  .b=0  },
    /* Second hand rod */
    { CHILD_HAND, .inner=-.319,.outer=.720, .width=0,     .cycle=60,    .stroke_width_r=.043,
      .shape=SHAPE_RECTANGLE, .render=RENDER_OUTLINE, .r=1,  .g=0,  .b=0  },
    /* Second hand dot */
    { CHILD_HAND, .inner=0,    .outer=.617, .width=.217,  .cycle=60,    .stroke_width_r=0,
      .shape=SHAPE_CIRCLE,    .render=RENDER_FILL,    .r=1,  .g=0,  .b=0  },
    /* Centre cap (red) */
    { CHILD_HAND, .inner=0,    .outer=0,    .width=.072,  .cycle=1,     .stroke_width_r=0,
      .shape=SHAPE_CIRCLE,    .render=RENDER_FILL,    .r=1,  .g=0,  .b=0  },
    /* Centre pin (grey) */
    { CHILD_HAND, .inner=0,    .outer=0,    .width=.02,   .cycle=1,     .stroke_width_r=0,
      .shape=SHAPE_CIRCLE,    .render=RENDER_FILL,    .r=.5, .g=.5, .b=.5 },
};

/* --- Command-line option storage --- */

static gboolean opt_railroad    = FALSE;
static gboolean opt_norailroad  = FALSE;
static gboolean opt_circular    = FALSE;
static gboolean opt_nocircular  = FALSE;
static gboolean opt_shape       = FALSE;
static gboolean opt_noshape     = FALSE;
static gboolean opt_fullscreen  = FALSE;
static gboolean opt_nofullscreen = FALSE;
static gboolean opt_version     = FALSE;
static gdouble  opt_tick        = 0.0;

static const GOptionEntry option_entries[] = {
    { "railroad",    0,   0,                    G_OPTION_ARG_NONE,   &opt_railroad,
      "Swiss Railway Clock second-hand mode (default)", NULL },
    /* historical single-letter aliases, hidden from --help */
    { "sbb",         0,   G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE,   &opt_railroad,   NULL, NULL },
    { "cff",         0,   G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE,   &opt_railroad,   NULL, NULL },
    { "ffs",         0,   G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE,   &opt_railroad,   NULL, NULL },
    { "norailroad",  0,   0,                    G_OPTION_ARG_NONE,   &opt_norailroad,
      "Smooth, continuous second-hand motion", NULL },
    { "circular",    0,   0,                    G_OPTION_ARG_NONE,   &opt_circular,
      "Keep face circular when window is non-square (default)", NULL },
    { "nocircular",  0,   0,                    G_OPTION_ARG_NONE,   &opt_nocircular,
      "Allow elliptical face when window is non-square", NULL },
    { "shape",       0,   0,                    G_OPTION_ARG_NONE,   &opt_shape,
      "Undecorated transparent circular window (default)", NULL },
    { "noshape",     0,   0,                    G_OPTION_ARG_NONE,   &opt_noshape,
      "Standard decorated rectangular window", NULL },
    { "fullscreen",  0,   0,                    G_OPTION_ARG_NONE,   &opt_fullscreen,
      "Start in fullscreen mode", NULL },
    { "nofullscreen",0,   0,                    G_OPTION_ARG_NONE,   &opt_nofullscreen,
      "Start in normal (non-fullscreen) mode (default)", NULL },
    { "tick",        0,   0,                    G_OPTION_ARG_DOUBLE,  &opt_tick,
      "Update interval in seconds (default: 0.06)", "SECONDS" },
    { "version",    'V',  0,                    G_OPTION_ARG_NONE,   &opt_version,
      "Show version information and exit", NULL },
    { NULL }
};

static int
on_handle_local_options(GApplication *app, GVariantDict *options, gpointer user_data)
{
    (void)app; (void)options;
    AppState *state = user_data;

    if (opt_version) {
        g_print("%s %s\n"
                "Copyright (C) 1992 Simon Leinen\n"
                "License GPLv2+: GNU General Public License v2 or later.\n",
                PACKAGE_NAME, PACKAGE_VERSION);
        return 0;
    }

    if (opt_norailroad) {
        state->railroad = FALSE;
        state->tick_ms  = 1000;
    }
    /* --railroad (or aliases) overrides --norailroad if both are given */
    if (opt_railroad)
        state->railroad = TRUE;

    if (opt_nocircular)
        state->circular = FALSE;
    if (opt_circular)
        state->circular = TRUE;

    if (opt_noshape)
        state->shaped = FALSE;
    if (opt_shape)
        state->shaped = TRUE;

    if (opt_fullscreen)
        state->is_fullscreen = TRUE;
    if (opt_nofullscreen)
        state->is_fullscreen = FALSE;

    if (opt_tick > 0.0) {
        state->tick_ms = (int)(opt_tick * 1000.0);
        if (state->tick_ms < 16) state->tick_ms = 16;
    }

    return -1;  /* continue */
}

int
main(int argc, char *argv[])
{
    AppState state = {
        .railroad = TRUE,
        .circular = TRUE,
        .shaped   = TRUE,   /* undecorated round window by default */
        .tick_ms  = 60,     /* 60 ms → smooth second-hand sweep */
        .bg_r = 1.0, .bg_g = 0.98, .bg_b = 0.98,  /* snow1 */
    };

    state.n_children = G_N_ELEMENTS(swisswatch_children);
    state.children   = g_memdup2(swisswatch_children, sizeof(swisswatch_children));

    GtkApplication *app = gtk_application_new("org.debian.swisswatch",
                                               G_APPLICATION_NON_UNIQUE);
    g_application_add_main_option_entries(G_APPLICATION(app), option_entries);
    g_signal_connect(app, "handle-local-options", G_CALLBACK(on_handle_local_options), &state);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), &state);

    int status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);
    g_free(state.children);
    return status;
}
