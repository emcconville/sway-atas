#ifndef _SWAY_CLIENT_H
#define _SWAY_CLIENT_H
#include <stdbool.h>
#include <wayland-util.h>

typedef struct _sway_window {
    char *sway_id;
    char *name;
    char *app_id;
    char *window_class;
    bool focused;
    struct wl_list link;
} sway_window;

typedef struct _sway_workspace {
    const char * name;
    struct wl_list windows;
    struct wl_list link;
} sway_workspace;
void sway_connect();
void sway_disconnect();
bool sway_switch_to_app(const char *id);
bool sway_get_windows(struct wl_list *list);
void sway_only_siblings(struct wl_list *list);
#endif