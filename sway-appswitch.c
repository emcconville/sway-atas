#include <gtk/gtk.h>
#include <gio/gdesktopappinfo.h>
#include <stdbool.h>
#include <wayland-util.h>
#include "sway-client.h"
#include "sway-appswitch.h"
#include "log.h"

#define kNoPadding 0
#define kPadding 8
#define kHalfPadding 4

void do_exit() {
    sway_disconnect();
    gtk_main_quit();
}

void app_selected(GtkWidget *b) {
    const char * app_id = gtk_widget_get_name(b);
    sway_assert(app_id != (const char *)NULL, "app_id not defined for window");
    sway_switch_to_app(app_id);
    do_exit();
}

static void list_app_selected(GtkWidget *lb) {
    GtkListBoxRow *active = gtk_list_box_get_selected_row(GTK_LIST_BOX(lb));
    sway_assert(active != (GtkListBoxRow *)NULL, "Unable to identify selected GtkListBoxRow");
    app_selected(GTK_WIDGET(active));
}


bool check_escape(GtkWidget *widget, GdkEventKey *event) {
    (void)widget;
    switch (event->keyval) {
        case GDK_KEY_Escape:
        case GDK_KEY_q:
        case GDK_KEY_Delete:
        case GDK_KEY_BackSpace: {
            do_exit();
        }
    }
    return FALSE;
}

void check_leave_focus(GtkWindow *win, GtkWidget *target) {
    (void)win;
    (void)target;
    do_exit();
}


static inline GIcon * gio_search_for_gicon(const char *token) {
    char ***r = NULL;
    size_t i = 0;
    size_t j = 0;
    GAppInfo *info = NULL;
    GIcon *icon = NULL;
    r = g_desktop_app_info_search(token);
    for (i = 0; r[i]; ++i) {
        for (j = 0; r[i][j]; ++j) {
            info = (GAppInfo *)g_desktop_app_info_new(r[i][j]);
            if (info != (GAppInfo *)NULL) {
                icon = g_app_info_get_icon(info);
                if (icon != (GIcon *)NULL) {
                    return icon;
                }                    
            }
        }
    }
    return (GIcon *)NULL;
}

GIcon * gio_resolve_gicon(struct _sway_window *win) {
    GIcon *icon = NULL;
    /**
     * APP_ID
     */
    if (win->app_id != NULL) {
        icon = gio_search_for_gicon(win->app_id);
        if (icon != (GIcon *)NULL) {
            return icon;
        }
    }
    /**
     * Class
     */
    if (win->window_class != NULL) {
        icon = gio_search_for_gicon(win->window_class);
        if (icon != (GIcon *)NULL) {
            return icon;
        }
    }
    /**
     * Name
     */
    if (win->name != NULL) {
        icon = gio_search_for_gicon(win->name);
        if (icon != (GIcon *)NULL) {
            return icon;
        }
    }
    return (GIcon *)NULL;
}

void display_grid(GtkWidget *window, struct wl_list *ws_list) {
    struct _sway_workspace *e = NULL;
    struct _sway_window *f = NULL;
    int row = 0;
    int column = 0;
    int max_columns = 0;
    int itr = 0;
    GtkWidget *button = NULL;
    GtkWidget *grid = NULL;
    GtkWidget *image = NULL;
    GtkWidget *selected = NULL;
    GtkWidget *ws_name = NULL;
    GIcon *icon = NULL;
    grid = gtk_grid_new();
    sway_assert(grid != (GtkWidget *)NULL, "Unable to allocate GtkGrid");
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), true);
    gtk_widget_set_margin_bottom(grid, kPadding);
    gtk_widget_set_margin_start(grid, kPadding);
    gtk_widget_set_margin_end(grid, kPadding);
    gtk_widget_set_margin_top(grid, kPadding);
    selected = NULL;
    wl_list_for_each(e, ws_list, link) {

        itr = wl_list_length(&e->windows);
        if ( itr > max_columns) {
            max_columns = itr;
        }
    }
    if (max_columns > 16) {
        max_columns = 10;
    } else if (max_columns > 8) {
        max_columns = 6;
    } else if (max_columns > 4) {
        max_columns = 4;
    }
    wl_list_for_each_reverse(e, ws_list, link) {
        ws_name = gtk_label_new(e->name);
        sway_assert(ws_name != (GtkWidget *)NULL, "Unable to allocate GtkLabel");
        column = 0;
        gtk_grid_attach(GTK_GRID(grid), ws_name, column, row++, max_columns, 1);
        wl_list_for_each_reverse(f, &e->windows, link) {
            icon = gio_resolve_gicon(f);
            if (icon == (GIcon *)NULL) {
                image = gtk_image_new_from_icon_name("system-run",
                                                     GTK_ICON_SIZE_DIALOG);
            } else {
                image = gtk_image_new_from_gicon(icon, GTK_ICON_SIZE_DIALOG);
            }
            sway_assert(image != (GtkWidget *)NULL, "Unable to allocate GtkImage");
            button = gtk_button_new();
            sway_assert(button != (GtkWidget *)NULL, "Unable to allocate GtkButton");
            gtk_button_set_image(GTK_BUTTON(button), GTK_WIDGET(image));
            gtk_button_set_always_show_image(GTK_BUTTON(button), TRUE);
            gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
            gtk_widget_set_name(GTK_WIDGET(button), f->sway_id);
            if (f->name) {
                gtk_widget_set_tooltip_text(GTK_WIDGET(button), f->name);
            } else if (f->app_id) {
                gtk_widget_set_tooltip_text(GTK_WIDGET(button), f->app_id);
            } else if (f->window_class) {
                gtk_widget_set_tooltip_text(GTK_WIDGET(button), f->window_class);
            }
            if (f->focused) {
                selected = button;
            }
            g_signal_connect(button,
                             "clicked",
                             G_CALLBACK(app_selected),
                             NULL);
            if (column >= max_columns) {
                row++;
                column = 0;
            }
            gtk_grid_attach(GTK_GRID(grid), button, column++, row, 1, 1);
        }
        row++;
    }
    /**
     * Set the focus AFTER all the buttons have been added to the grid.
     * Else you may have a race condition on the LAST element packed.
     */
    if (selected != NULL) {
        gtk_widget_grab_focus(selected);
        gtk_widget_set_state_flags(
            selected,
            GTK_STATE_FLAG_FOCUSED | GTK_STATE_FLAG_ACTIVE,
            true
        );
    }
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(grid));
}

void display_list(GtkWidget *window, struct wl_list *list) {
    struct _sway_workspace *e = NULL;
    struct _sway_window *f = NULL;
    GtkWidget *box = NULL;
    GtkWidget *image = NULL;
    GtkWidget *label = NULL;
    GtkWidget *listbox = NULL;
    GtkWidget *listrow = NULL;
    GtkWidget *selected = NULL;
    GIcon *icon = NULL;
    listbox = gtk_list_box_new();
    sway_assert(listbox != (GtkWidget *)NULL, "Unable to allocate GtkListBox");
    wl_list_for_each_reverse(e, list, link) {
        wl_list_for_each_reverse(f, &e->windows, link) {
            listrow = gtk_list_box_row_new();
            gtk_widget_set_name(listrow, f->sway_id);
            icon = gio_resolve_gicon(f);
            if (icon == (GIcon *)NULL) {
                image = gtk_image_new_from_icon_name("system-run",
                                                     GTK_ICON_SIZE_DIALOG);
            } else {
                image = gtk_image_new_from_gicon(icon, GTK_ICON_SIZE_DIALOG);
            }
            sway_assert(image != (GtkWidget *)NULL, "Unable to allocate GtkImage");
            if (f->name) {
                label = gtk_label_new(f->name);
            } else if (f->app_id) {
                label = gtk_label_new(f->app_id);
            } else if (f->window_class) {
                label = gtk_label_new(f->window_class);
            } else {
                label = gtk_label_new("???");
            }
            sway_assert(label != (GtkWidget *)NULL, "Unable to allocate GtkLabel");
            box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, kPadding);
            sway_assert(box != (GtkWidget *)NULL, "Unable to allocate GtkBox");
            gtk_widget_set_margin_end(box, 8);
            gtk_box_pack_start(GTK_BOX(box), image, false, false, kPadding);
            gtk_box_pack_start(GTK_BOX(box), label, false, false, kNoPadding);
            if (f->focused) {
                selected = listrow;
            }
            gtk_container_add(GTK_CONTAINER(listrow), box);
            //g_signal_connect(listrow, "activate", G_CALLBACK(app_selected), NULL);
            gtk_list_box_insert(GTK_LIST_BOX(listbox), listrow, -1);
        }
    }
    gtk_list_box_select_row(GTK_LIST_BOX(listbox), GTK_LIST_BOX_ROW(selected));
    g_signal_connect(listbox,
                     "row-activated",
                     G_CALLBACK(list_app_selected),
                     NULL);
    gtk_container_add(GTK_CONTAINER(window), listbox);
}