#ifndef LABWC_H
#define LABWC_H

#define _POSIX_C_SOURCE 200112L
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_matrix.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_server_decoration.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/util/log.h>
#include <wlr/xwayland.h>
#include <xkbcommon/xkbcommon.h>

#define XCURSOR_DEFAULT "left_ptr"
#define XCURSOR_SIZE 24
#define XCURSOR_MOVE "grabbing"
#define XWL_TITLEBAR_HEIGHT (10)
#define XWL_WINDOW_BORDER (3)
#define LAB_DISABLE_CSD (0)

enum cursor_mode {
	TINYWL_CURSOR_PASSTHROUGH,
	TINYWL_CURSOR_MOVE,
	TINYWL_CURSOR_RESIZE,
};

struct server {
	struct wl_display *wl_display;
	struct wlr_backend *backend;
	struct wlr_renderer *renderer;
	struct wlr_compositor *compositor;

	struct wlr_xdg_shell *xdg_shell;
	struct wl_listener new_xdg_surface;
	struct wl_listener xdg_toplevel_decoration;
	struct wlr_xwayland *xwayland;
	struct wl_listener new_xwayland_surface;
	struct wl_list views;

	struct wlr_cursor *cursor;
	struct wlr_xcursor_manager *cursor_mgr;
	struct wl_listener cursor_motion;
	struct wl_listener cursor_motion_absolute;
	struct wl_listener cursor_button;
	struct wl_listener cursor_axis;
	struct wl_listener cursor_frame;

	struct wlr_seat *seat;
	struct wl_listener new_input;
	struct wl_listener request_cursor;
	struct wl_listener request_set_selection;
	struct wl_list keyboards;

	/* cursor interactive */
	enum cursor_mode cursor_mode;
	struct view *grabbed_view;
	double grab_x, grab_y;
	struct wlr_box grab_box;
	uint32_t resize_edges;

	struct wlr_output_layout *output_layout;
	struct wl_list outputs;
	struct wl_listener new_output;

	/* For use in cycle (alt-tab) mode */
	struct view *cycle_view;
};

struct output {
	struct wl_list link;
	struct server *server;
	struct wlr_output *wlr_output;
	struct wl_listener frame;
};

enum view_type { LAB_XDG_SHELL_VIEW, LAB_XWAYLAND_VIEW };

/* Keep LAB_DECO_NONE last for the purpose of iterating */
enum deco_part { LAB_DECO_PART_TOP = 0, LAB_DECO_PART_LEFT, LAB_DECO_NONE };

struct view {
	enum view_type type;
	struct wl_list link;
	struct server *server;
	struct wlr_xdg_surface *xdg_surface;
	struct wlr_xwayland_surface *xwayland_surface;
	struct wlr_surface *surface;
	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener destroy;
	struct wl_listener request_move;
	struct wl_listener request_resize;
	struct wl_listener request_configure;

	bool mapped;
	/*
	 * Some X11 windows appear to create additional top levels windows
	 * which we want to ignore. These are never mapped, so we can track
	 * them that way
	 */
	bool been_mapped;
	int x, y;
};

struct keyboard {
	struct wl_list link;
	struct server *server;
	struct wlr_input_device *device;

	struct wl_listener modifiers;
	struct wl_listener key;
};

void xdg_toplevel_decoration(struct wl_listener *listener, void *data);
void xdg_surface_map(struct wl_listener *listener, void *data);
void xdg_surface_unmap(struct wl_listener *listener, void *data);
void xdg_surface_destroy(struct wl_listener *listener, void *data);
void xdg_toplevel_request_move(struct wl_listener *listener, void *data);
void xdg_toplevel_request_resize(struct wl_listener *listener, void *data);
void xdg_surface_new(struct wl_listener *listener, void *data);

int xwl_nr_parents(struct view *view);
void xwl_surface_map(struct wl_listener *listener, void *data);
void xwl_surface_unmap(struct wl_listener *listener, void *data);
void xwl_surface_destroy(struct wl_listener *listener, void *data);
void xwl_surface_configure(struct wl_listener *listener, void *data);
void xwl_surface_new(struct wl_listener *listener, void *data);

/**
 * view_get_surface_geometry - geometry relative to view
 * @view: toplevel containing the surface to process
 * Note: XDG views sometimes have an invisible border, so x and y can be
 * greater than zero.
 */
struct wlr_box view_get_surface_geometry(struct view *view);
struct wlr_box view_geometry(struct view *view);
void view_resize(struct view *view, struct wlr_box geo);
bool view_want_deco(struct view *view);
void view_focus(struct view *view);
struct view *view_front_toplevel(struct server *server);
struct view *next_toplevel(struct view *current);
struct view *view_at(struct server *server, double lx, double ly,
		     struct wlr_surface **surface, double *sx, double *sy,
		     int *view_area);

void interactive_begin(struct view *view, enum cursor_mode mode,
		       uint32_t edges);

void server_new_input(struct wl_listener *listener, void *data);
void seat_request_cursor(struct wl_listener *listener, void *data);
void seat_request_set_selection(struct wl_listener *listener, void *data);
void server_cursor_motion(struct wl_listener *listener, void *data);
void server_cursor_motion_absolute(struct wl_listener *listener, void *data);
void server_cursor_button(struct wl_listener *listener, void *data);
void server_cursor_axis(struct wl_listener *listener, void *data);
void server_cursor_frame(struct wl_listener *listener, void *data);
void server_new_output(struct wl_listener *listener, void *data);

void server_new_keyboard(struct server *server, struct wlr_input_device *device);

void output_frame(struct wl_listener *listener, void *data);

void dbg_show_one_view(struct view *view);
void dbg_show_views(struct server *server);

struct wlr_box deco_max_extents(struct view *view);
struct wlr_box deco_box(struct view *view, enum deco_part deco_part);
enum deco_part deco_at(struct view *view, double lx, double ly);

#endif /* LABWC_H */