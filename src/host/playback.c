#include "internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// A small in-memory queue. The host has no DAC and no player library; the point
// is that an app's control flow, queue bookkeeping and status display can be
// exercised.

static char **g_paths;
static int g_count;
static int g_index = -1;
static int g_position_ms;
static morpthis_audio_state_t g_state;
static morpthis_playback_mode_t g_mode;
static double g_speed = 1.0;
static morpthis_playback_changed_cb g_changed;
static void *g_changed_user;

static void notify(void) {
	if (g_changed) {
		g_changed(g_changed_user);
	}
}

static void clear_queue(void) {
	for (int i = 0; i < g_count; i++) {
		free(g_paths[i]);
	}
	free(g_paths);
	g_paths = NULL;
	g_count = 0;
	g_index = -1;
}

static bool push_path(const char *path) {
	char **grown = realloc(g_paths, (size_t)(g_count + 1) * sizeof(*grown));
	if (!grown) {
		return false;
	}
	g_paths = grown;
	g_paths[g_count] = strdup(path ? path : "");
	if (!g_paths[g_count]) {
		return false;
	}
	g_count++;
	return true;
}

static void copy_basename(char *out, size_t size, const char *path) {
	const char *slash = path ? strrchr(path, '/') : NULL;
	snprintf(out, size, "%s", slash ? slash + 1 : (path ? path : ""));
}

bool morpthis_playback_play_file(const char *absolute_path, int start_ms) {
	if (!absolute_path || !absolute_path[0] || absolute_path[0] != '/') {
		return false;
	}
	clear_queue();
	if (!push_path(absolute_path)) {
		return false;
	}
	g_index = 0;
	g_position_ms = start_ms > 0 ? start_ms : 0;
	g_state = MORPTHIS_AUDIO_PLAYING;
	notify();
	return true;
}

bool morpthis_playback_play_queue(const char *const *paths, int count, int start_index) {
	if (!paths || count <= 0 || start_index < 0 || start_index >= count) {
		return false;
	}
	clear_queue();
	for (int i = 0; i < count; i++) {
		if (!paths[i] || !paths[i][0] || !push_path(paths[i])) {
			clear_queue();
			return false;
		}
	}
	g_index = start_index;
	g_position_ms = 0;
	g_state = MORPTHIS_AUDIO_PLAYING;
	notify();
	return true;
}

bool morpthis_playback_play_folder(const char *absolute_folder, const char *selected_name) {
	if (!absolute_folder || !selected_name) {
		return false;
	}
	char path[HOST_PATH_MAX];
	int n = snprintf(path, sizeof(path), "%s/%s", absolute_folder, selected_name);
	return n > 0 && (size_t)n < sizeof(path) && morpthis_playback_play_file(path, 0);
}

bool morpthis_playback_play_index(int index) {
	if (index < 0 || index >= g_count) {
		return false;
	}
	g_index = index;
	g_position_ms = 0;
	g_state = MORPTHIS_AUDIO_PLAYING;
	notify();
	return true;
}

bool morpthis_playback_resume(void) {
	if (g_count == 0) {
		return false;
	}
	g_state = MORPTHIS_AUDIO_PLAYING;
	notify();
	return true;
}

bool morpthis_playback_pause(void) {
	if (g_state == MORPTHIS_AUDIO_PLAYING) {
		g_state = MORPTHIS_AUDIO_PAUSED;
		notify();
	}
	return true;
}

bool morpthis_playback_toggle(void) {
	if (g_state == MORPTHIS_AUDIO_PLAYING) {
		return morpthis_playback_pause();
	}
	return morpthis_playback_resume();
}

bool morpthis_playback_stop(void) {
	g_state = MORPTHIS_AUDIO_STOPPED;
	g_position_ms = 0;
	notify();
	return true;
}

bool morpthis_playback_next(void) {
	if (g_count == 0) {
		return false;
	}
	g_index = (g_index + 1) % g_count;
	g_position_ms = 0;
	g_state = MORPTHIS_AUDIO_PLAYING;
	notify();
	return true;
}

bool morpthis_playback_prev(void) {
	if (g_count == 0) {
		return false;
	}
	g_index = (g_index + g_count - 1) % g_count;
	g_position_ms = 0;
	g_state = MORPTHIS_AUDIO_PLAYING;
	notify();
	return true;
}

bool morpthis_playback_seek(int position_ms) {
	if (g_count == 0) {
		return false;
	}
	g_position_ms = position_ms > 0 ? position_ms : 0;
	return true;
}

bool morpthis_playback_get_info(morpthis_playback_info_t *out) {
	if (!out) {
		return false;
	}
	memset(out, 0, sizeof(*out));
	out->state = g_state;
	out->mode = g_mode;
	out->queue_count = g_count;
	out->queue_index = g_index;
	out->position_ms = g_position_ms;
	out->sample_rate = 44100;
	out->channels = 2;
	out->bit_depth = 16;
	if (g_index >= 0 && g_index < g_count) {
		snprintf(out->path, sizeof(out->path), "%s", g_paths[g_index]);
		copy_basename(out->title, sizeof(out->title), g_paths[g_index]);
	}
	return g_count > 0;
}

void morpthis_playback_set_mode(morpthis_playback_mode_t mode) {
	if (mode >= MORPTHIS_PLAYBACK_NORMAL && mode <= MORPTHIS_PLAYBACK_SHUFFLE_REPEAT) {
		g_mode = mode;
		notify();
	}
}

morpthis_playback_mode_t morpthis_playback_get_mode(void) { return g_mode; }

bool morpthis_playback_set_speed(double factor) {
	if (factor < 0.25 || factor > 4.0) {
		return false;
	}
	g_speed = factor;
	return true;
}

double morpthis_playback_get_speed(void) { return g_speed; }

int morpthis_playback_queue_count(void) { return g_count; }

int morpthis_playback_queue_index(void) { return g_index; }

bool morpthis_playback_queue_path(int index, char *out, size_t size) {
	if (!out || size == 0 || index < 0 || index >= g_count) {
		return false;
	}
	snprintf(out, size, "%s", g_paths[index]);
	return true;
}

bool morpthis_playback_queue_insert(const char *absolute_path, int slot) {
	if (!absolute_path || !absolute_path[0] || absolute_path[0] != '/') {
		return false;
	}
	int at = slot < 0 ? g_index + 1 : slot;
	if (at < 0) {
		at = 0;
	}
	if (at > g_count) {
		at = g_count;
	}
	if (!push_path(absolute_path)) {
		return false;
	}
	if (at < g_count - 1) {
		char *moved = g_paths[g_count - 1];
		memmove(&g_paths[at + 1], &g_paths[at], (size_t)(g_count - 1 - at) * sizeof(*g_paths));
		g_paths[at] = moved;
		if (g_index >= at) {
			g_index++;
		}
	}
	if (g_index < 0) {
		g_index = 0;
	}
	notify();
	return true;
}

bool morpthis_playback_queue_clear(void) {
	clear_queue();
	notify();
	return true;
}

bool morpthis_playback_cover_path(char *out, size_t size) {
	(void)out;
	(void)size;
	return false;
}

void morpthis_playback_set_changed_cb(morpthis_playback_changed_cb cb, void *user) {
	g_changed = cb;
	g_changed_user = user;
}
