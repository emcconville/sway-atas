#ifndef _SWAY_APPSWITCH_H
#define _SWAY_APPSWITCH_H
#include <gtk/gtk.h>
#include <wayland-util.h>
#include "sway-client.h"
void do_exit(void);
void app_selected(GtkWidget *b);
bool check_escape(GtkWidget *widget, GdkEventKey *event);
void check_leave_focus(GtkWindow *win, GtkWidget *target);
GIcon * gio_resolve_gicon(struct _sway_window *win);
void display_grid(GtkWidget *window, struct wl_list *ws_list);
void display_list(GtkWidget *window, struct wl_list *list);
#endif