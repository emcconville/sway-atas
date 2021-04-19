#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <gtk/gtk.h>
#include <wayland-util.h>
#include "sway-appswitch.h"
#include "sway-client.h"
#include "log.h"

void show_help() {
    const char * msg = "sway-atas usage:\n"
    " -h  --help      Show this help message.\n"
    " -l  --list      Display windows as a list.\n"
    " -s  --siblings  Only show windows that match the currently selected\n"
    "                 application.\n";
    fprintf(stderr, "%s", msg);
}

int main(int argc, char ** argv) {
    int i = 0;
    bool show_list = false;
    bool only_siblings = false;
    GtkWidget *window = NULL;
    GtkWidget *handleBar = NULL;
    struct wl_list ws_list;

    for (i=1; i < argc; ++i) {
        char * a = argv[i];
        if (a[0] == '-') {
            if(strncmp("-l", a, 2) == 0) {
                show_list = true;
            } else if(strncmp("-s", a, 2) == 0) {
                only_siblings = true;
            } else if(strncmp("-h", a, 2) == 0) {
                show_help();
                exit(1);
            } else if(strncmp("--list", a, 6) == 0) {
                show_list = true;
            } else if(strncmp("--siblings", a, 10) == 0) {
                only_siblings = true;
            } else if(strncmp("--help", a, 10) == 0) {
                show_help();
                exit(1);
            }
        }
    }

    sway_connect();
    wl_list_init(&ws_list);
    sway_get_windows(&ws_list);
    if (only_siblings) {
        sway_only_siblings(&ws_list);
    }
    gtk_init(&argc, &argv);
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    sway_assert(window != (GtkWidget *)NULL, "Unable to allocate GtkWindow");
    gtk_widget_set_name(window, "window");
	gtk_window_set_default_size(GTK_WINDOW(window), 50, 50);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    /**
     * Setting the titlebar to an empty titlebar is a sad hack.
     */
    handleBar = gtk_header_bar_new();
    sway_assert(handleBar != (GtkWidget *)NULL, "Unable to allocate GtkHeaderBar");
    gtk_window_set_titlebar(GTK_WINDOW(window), handleBar);
    gtk_widget_realize(window);
    g_signal_connect(window, "destroy", G_CALLBACK(do_exit), NULL);
    g_signal_connect(window, "key_press_event", G_CALLBACK(check_escape), NULL);
    g_signal_connect(window, "focus-out-event", G_CALLBACK(check_leave_focus), NULL);
    if (show_list) {
        display_list(window, &ws_list);
    } else {
        display_grid(window, &ws_list);
    }
    gtk_widget_show_all(GTK_WIDGET(window));
    gtk_main();
}
