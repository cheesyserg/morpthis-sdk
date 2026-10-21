#ifndef MORPTHIS_HOST_INTERNAL_H
#define MORPTHIS_HOST_INTERNAL_H

// Private to the host runtime. Nothing here is installed or visible to an app.

#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "morpthis/morpthis.h"

#define HOST_ID_MAX 64
#define HOST_NAME_MAX 96
#define HOST_VERSION_MAX 32
#define HOST_PATH_MAX 1024

// The player's context, as much of it as the host runtime needs. The public
// type is opaque; this is the definition only this library sees.
struct morpthis_app {
	char id[HOST_ID_MAX];
	char name[HOST_NAME_MAX];
	char version[HOST_VERSION_MAX];
	char folder[HOST_PATH_MAX];
};

typedef enum {
	HOST_WIDGET_SCREEN = 0,
	HOST_WIDGET_LABEL,
	HOST_WIDGET_BUTTON,
	HOST_WIDGET_LIST,
	HOST_WIDGET_IMAGE,
	HOST_WIDGET_SLIDER,
	HOST_WIDGET_CANVAS
} host_widget_kind_t;

struct morpthis_widget {
	host_widget_kind_t kind;
	char *text;
	char *image;
	uint16_t *pixels;

	int x;
	int y;
	int width;
	int height;
	bool visible;
	bool enabled;

	morpthis_click_cb click_cb;
	void *click_user;
	morpthis_index_cb select_cb;
	void *select_user;
	morpthis_value_cb value_cb;
	void *value_user;

	int min;
	int max;
	int value;

	char **items;
	int item_count;

	struct morpthis_widget *parent;
	struct morpthis_widget *next;
	struct morpthis_widget *child;
};

struct morpthis_screen {
	struct morpthis_widget base;
	char *title;
};

struct morpthis_file {
	FILE *fp;
};

extern char g_app_folder[HOST_PATH_MAX];
extern char g_card_root[HOST_PATH_MAX];

void host_runtime_init(const char *id, const char *name, const char *version,
					   const char *folder);
const morpthis_app_t *host_app(void);

bool host_quit_requested(void);
void host_run_posted(void);

// UI helpers used by the host runner and the script runner.
bool host_ui_click_label(const char *label);

// Input injection. These call the app's callbacks exactly as the player would.
void host_input_dispatch_button(morpthis_button_t button,
								morpthis_input_action_t action);
void host_input_dispatch_touch(morpthis_touch_action_t action, int x, int y);

// Runs a semicolon-separated test script (see docs/api.md). False when a
// command is unknown or a named widget is missing.
bool host_run_script(const char *script);

#endif /* MORPTHIS_HOST_INTERNAL_H */
