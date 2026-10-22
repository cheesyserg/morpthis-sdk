#include "internal.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "morpthis/app.h"

struct morpthis_app g_app;

char g_app_folder[HOST_PATH_MAX];
char g_app_data[HOST_PATH_MAX];
char g_card_root[HOST_PATH_MAX];

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static bool g_quit;

typedef struct post_node {
	morpthis_post_fn fn;
	void *user;
	struct post_node *next;
} post_node_t;

static post_node_t *g_posts;
static post_node_t *g_posts_tail;

uint32_t morpthis_app_time_ms(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint32_t)((uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u);
}

static void copy_string(char *out, size_t size, const char *text) {
	if (!out || size == 0) {
		return;
	}
	if (!text) {
		text = "";
	}
	size_t length = strlen(text);
	if (length >= size) {
		length = size - 1;
	}
	memcpy(out, text, length);
	out[length] = '\0';
}

// realpath() makes the folder absolute, which keeps morpthis_fs paths stable
// even when the runner was started by a relative path.
static void set_folder(const char *folder) {
	char resolved[PATH_MAX];
	if (folder && folder[0] && realpath(folder, resolved)) {
		copy_string(g_app_folder, sizeof(g_app_folder), resolved);
		return;
	}
	copy_string(g_app_folder, sizeof(g_app_folder), folder && folder[0] ? folder : ".");
}

// The card root is the app folder's parent unless the host runner was told
// otherwise. The device's player would supply the mounted card path.
static void set_card_root(void) {
	const char *env = getenv("MORPTHIS_CARD_ROOT");
	if (env && env[0]) {
		copy_string(g_card_root, sizeof(g_card_root), env);
		return;
	}
	copy_string(g_card_root, sizeof(g_card_root), g_app_folder);
	char *slash = strrchr(g_card_root, '/');
	if (slash && slash != g_card_root) {
		*slash = '\0';
	}
}

// The writable folder. On the device this is <card>/Apps/merp_data/<id>; the
// host runner keeps its old behaviour -- the app folder -- unless the test asks
// for a separate one with MORPTHIS_APP_DATA.
static void set_data_folder(void) {
	const char *env = getenv("MORPTHIS_APP_DATA");
	copy_string(g_app_data, sizeof(g_app_data), env && env[0] ? env : g_app_folder);
}

void host_runtime_init(const char *id, const char *name, const char *version,
					   const char *folder) {
	copy_string(g_app.id, sizeof(g_app.id), id && id[0] ? id : "app");
	copy_string(g_app.name, sizeof(g_app.name), name && name[0] ? name : g_app.id);
	copy_string(g_app.version, sizeof(g_app.version), version && version[0] ? version : "0.0.0");
	set_folder(folder);
	set_card_root();
	set_data_folder();
	copy_string(g_app.folder, sizeof(g_app.folder), g_app_folder);

	pthread_mutex_lock(&g_lock);
	g_quit = false;
	pthread_mutex_unlock(&g_lock);
}

const morpthis_app_t *host_app(void) {
	return &g_app;
}

const char *morpthis_app_id(const morpthis_app_t *app) {
	return app ? app->id : "";
}

const char *morpthis_app_name(const morpthis_app_t *app) {
	return app ? app->name : "";
}

const char *morpthis_app_version(const morpthis_app_t *app) {
	return app ? app->version : "";
}

const char *morpthis_app_folder(const morpthis_app_t *app) {
	return app ? app->folder : g_app_folder;
}

int morpthis_app_api_version(const morpthis_app_t *app) {
	(void)app;
	return MORPTHIS_APP_API_VERSION;
}

size_t morpthis_app_mem_available(void) {
	const char *env = getenv("MORPTHIS_MEM_AVAILABLE");
	if (env && env[0]) {
		char *end = NULL;
		unsigned long long value = strtoull(env, &end, 10);
		if (end && *end == '\0') {
			return (size_t)value;
		}
	}
	return 16u * 1024u * 1024u;
}

void morpthis_app_post(morpthis_post_fn fn, void *user) {
	if (!fn) {
		return;
	}

	post_node_t *node = calloc(1, sizeof(*node));
	if (!node) {
		return;
	}
	node->fn = fn;
	node->user = user;

	pthread_mutex_lock(&g_lock);
	if (g_posts_tail) {
		g_posts_tail->next = node;
	} else {
		g_posts = node;
	}
	g_posts_tail = node;
	pthread_mutex_unlock(&g_lock);
}

void morpthis_app_quit(void) {
	pthread_mutex_lock(&g_lock);
	g_quit = true;
	pthread_mutex_unlock(&g_lock);
}

bool morpthis_app_background(void) {
	// The host runner has no player chrome to go back to; the app simply keeps
	// running, which is what the device does with its screens still built.
	return true;
}

bool morpthis_app_is_background(void) {
	return false;
}

void morpthis_app_back_handled(void) {
	// The player reads this flag in its back guard. The host runner has no guard
	// and nothing to keep alive, so there is nothing to do.
}

bool host_quit_requested(void) {
	pthread_mutex_lock(&g_lock);
	bool quit = g_quit;
	pthread_mutex_unlock(&g_lock);
	return quit;
}

// Takes the whole queue out under the lock, then runs it without it: a posted
// callback that calls morpthis_app_post() or morpthis_app_quit() must not
// deadlock against the runner.
void host_run_posted(void) {
	for (;;) {
		pthread_mutex_lock(&g_lock);
		post_node_t *node = g_posts;
		g_posts = NULL;
		g_posts_tail = NULL;
		pthread_mutex_unlock(&g_lock);

		if (!node) {
			return;
		}

		while (node) {
			post_node_t *next = node->next;
			node->fn(node->user);
			free(node);
			node = next;
		}
	}
}
