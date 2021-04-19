#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <json.h>
#include <wayland-util.h>
#include "ipc-client.h"
#include "sway-client.h"

#define kCommandBufferSize 1024

static int i3_fd = 0;

void sway_i3_walk_workspace(json_object *node, struct wl_list *list);
void sway_i3_walk_windows(json_object *node, struct wl_list *list);

void sway_connect()	 {
	char *socket_path = NULL;
	struct timeval timeout = {.tv_sec = 3, .tv_usec = 0};
    socket_path = get_socketpath();
    if (socket_path == NULL) {
        exit(EXIT_FAILURE);
    }
	i3_fd = ipc_open_socket(socket_path);
	ipc_set_recv_timeout(i3_fd, timeout);
	free(socket_path);
}

void sway_disconnect() {
	close(i3_fd);
}

bool sway_switch_to_app(const char *id) {
	uint32_t type = 0;
	uint32_t len = 0;
	char command[kCommandBufferSize];
	type = IPC_COMMAND;
	snprintf(command, kCommandBufferSize, "[con_id=%s] focus", id);
	len = strlen(command);
	char *resp = ipc_single_command(i3_fd, type, command, &len);
	free(resp);
	return true;
}

bool sway_get_windows(struct wl_list *list) {
	uint32_t type = 0;
	uint32_t len = 0;
	char *command = strdup("");
    type = IPC_GET_TREE;
	bool ret = false;
	len = strlen(command);
	char *resp = ipc_single_command(i3_fd, type, command, &len);
    json_object *obj = json_tokener_parse(resp);
    if (obj != (json_object *)NULL) {
		if (json_object_is_type(obj, json_type_object)) {
			sway_i3_walk_workspace(obj, list);
			ret = true;
		}
		json_object_put(obj);
	}
	free(command);
	free(resp);
	return ret;
}

void sway_only_siblings(struct wl_list *list) {
	struct _sway_workspace *workspace  = NULL;
	struct _sway_window *window = NULL;
	struct _sway_window *active = NULL;
	struct _sway_window  *tmp = NULL;
	active = NULL;
	wl_list_for_each(workspace, list, link) {
		wl_list_for_each(window, &workspace->windows, link) {
			if (window->focused) {
				active = window;
			}
		}
	}
	if (active == NULL) {
		return;
	}
	if (active->window_class == NULL && active->app_id == NULL) {
		// Try to match on name.
		wl_list_for_each(workspace, list, link) {
			wl_list_for_each(window, &workspace->windows, link) {
				if (strcmp(active->name, window->name) != 0) {
					wl_list_remove(&window->link);
				}
			}
		}
	} else {
		// Try to match BOTH app_id & window_class.
		if (active->window_class != (char *)NULL) {
			wl_list_for_each(workspace, list, link) {
				wl_list_for_each_safe(window, tmp, &workspace->windows, link) {
					if (window->window_class == (char *)NULL || strcmp(active->window_class, window->window_class) != 0) {
						wl_list_remove(&window->link);
					}
				}
			}
		} else if (active->app_id != (char *)NULL) {
			wl_list_for_each(workspace, list, link) {
				wl_list_for_each_safe(window, tmp, &workspace->windows, link) {
					if (window->app_id == (char *)NULL || strcmp(active->app_id, window->app_id) != 0) {
						wl_list_remove(&window->link);
					}
				}
			}
		}
	}
}

void sway_i3_walk_workspace(json_object *node, struct wl_list *list) {
    json_object *type = NULL;
    json_object *name = NULL;
    json_object *children = NULL;
    json_object *child = NULL;
	size_t child_len = 0;
	size_t idx = 0;
	const char *type_value = NULL;
	const char *name_value = NULL;
	sway_workspace *workspace = NULL;
    json_object_object_get_ex(node, "type", &type);
    json_object_object_get_ex(node, "name", &name);
    json_object_object_get_ex(node, "nodes", &children);
	child_len = json_object_array_length(children);
    type_value = json_object_get_string(type);
    if (type_value == NULL) {
        return;
    }
    if (strcmp(type_value, "workspace") == 0) {
        name_value = json_object_get_string(name);
        if (strncmp(name_value, "__i3", 4) != 0) {
			workspace = malloc(sizeof(struct _sway_workspace));
			workspace->name = strdup(name_value);
			wl_list_init(&workspace->windows);
			for (idx= 0; idx < child_len; ++idx) {
				child = json_object_array_get_idx(children, idx);
				sway_i3_walk_windows(child, &workspace->windows);
			}
            wl_list_insert(list, &workspace->link);
        }
    } else {
        for (idx = 0; idx < child_len; ++idx) {
            child = json_object_array_get_idx(children, idx);
            sway_i3_walk_workspace(child, list);
        }
    }
}

void sway_i3_walk_windows(json_object *node, struct wl_list *list) {
	json_object *layout = NULL;
	json_object *name = NULL;
	json_object *app_id = NULL;
	json_object *window = NULL;
	json_object *class = NULL;
	json_object *children = NULL;
	json_object *child = NULL;
	json_object *id = NULL;
	json_object *focused = NULL;
	size_t child_len = 0;
	size_t idx = 0;
	const char *value = NULL;
	sway_window * sw = NULL;
	json_object_object_get_ex(node, "layout", &layout);
	json_object_object_get_ex(node, "nodes", &children);
	json_object_object_get_ex(node, "id", &id);
	child_len = json_object_array_length(children);
	value = json_object_get_string(layout);
	if (strcmp(value, "none") == 0) {
		sw = malloc(sizeof(sway_window));
		sw->sway_id = strdup(json_object_get_string(id));
		if (json_object_object_get_ex(node, "name", &name)) {
			if (json_object_is_type(name, json_type_null)) {
				sw->name = NULL;
			} else {
				sw->name = strdup(json_object_get_string(name));
			}
		} else {
			sw->name = NULL;
		}
		if (json_object_object_get_ex(node, "app_id", &app_id)) {
			if (json_object_is_type(app_id, json_type_null)) {
				sw->app_id = NULL;
			} else {
				const char * t = json_object_get_string(app_id);
				sw->app_id = strdup(t);
			}
		} else {
			sw->app_id = NULL;
		}
		if (json_object_object_get_ex(node, "window_properties", &window)) {
			json_object_object_get_ex(window, "class", &class);
			if (json_object_is_type(class, json_type_null)) {
				sw->window_class = NULL;
			} else {
				sw->window_class = strdup(json_object_get_string(class));
			}
		} else {
			sw->window_class = NULL;
		}
		if (json_object_object_get_ex(node, "focused", &focused)) {
			if (json_object_get_boolean(focused)) {
				sw->focused = true;
			} else {
				sw->focused = false;
			}
		}
		wl_list_insert(list, &sw->link);
	} else {
        for (idx = 0; idx < child_len; ++idx) {
            child = json_object_array_get_idx(children, idx);
            sway_i3_walk_windows(child, list);
        }
	}
}