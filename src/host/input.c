#include "internal.h"

static morpthis_button_cb g_button_cb;
static void *g_button_user;

static morpthis_touch_cb g_touch_cb;
static void *g_touch_user;

void morpthis_input_set_button_cb(morpthis_button_cb cb, void *user) {
	g_button_cb = cb;
	g_button_user = user;
}

void morpthis_input_set_touch_cb(morpthis_touch_cb cb, void *user) {
	g_touch_cb = cb;
	g_touch_user = user;
}

void morpthis_input_set_gameplay(bool active) {
	// The host runner injects buttons directly and has no player-owned volume
	// keys to context-switch.
	(void)active;
}

void host_input_dispatch_button(morpthis_button_t button,
								morpthis_input_action_t action) {
	if (g_button_cb) {
		g_button_cb(button, action, g_button_user);
	}
}

void host_input_dispatch_touch(morpthis_touch_action_t action, int x, int y) {
	if (g_touch_cb) {
		morpthis_point_t point = {.x = x, .y = y};
		g_touch_cb(action, point, g_touch_user);
	}
}
