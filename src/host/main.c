#define _POSIX_C_SOURCE 200809L

#include "internal.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void sleep_ms(long ms) {
	if (ms <= 0) {
		return;
	}
	struct timespec ts = {
		.tv_sec = (time_t)(ms / 1000),
		.tv_nsec = (long)(ms % 1000) * 1000000L,
	};
	while (nanosleep(&ts, &ts) != 0 && errno == EINTR) {
	}
}

static char *trim(char *text) {
	while (*text && isspace((unsigned char)*text)) {
		text++;
	}
	char *end = text + strlen(text);
	while (end > text && isspace((unsigned char)end[-1])) {
		*--end = '\0';
	}
	return text;
}

static bool parse_button(const char *name, morpthis_button_t *out) {
	static const struct {
		const char *name;
		morpthis_button_t button;
	} names[] = {
		{"play", MORPTHIS_BUTTON_PLAY_PAUSE},
		{"playpause", MORPTHIS_BUTTON_PLAY_PAUSE},
		{"next", MORPTHIS_BUTTON_NEXT},
		{"prev", MORPTHIS_BUTTON_PREV},
		{"volup", MORPTHIS_BUTTON_VOLUME_UP},
		{"voldown", MORPTHIS_BUTTON_VOLUME_DOWN},
		{"power", MORPTHIS_BUTTON_POWER},
		{"back", MORPTHIS_BUTTON_BACK},
	};
	for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
		if (strcmp(name, names[i].name) == 0) {
			*out = names[i].button;
			return true;
		}
	}
	return false;
}

static bool run_command(char *command) {
	if (strcmp(command, "quit") == 0) {
		morpthis_app_quit();
		return true;
	}

	if (strncmp(command, "click:", 6) == 0) {
		char *label = trim(command + 6);
		return host_ui_click_label(label);
	}

	if (strncmp(command, "touch:", 6) == 0) {
		int x = 0, y = 0;
		if (sscanf(command + 6, " %d , %d", &x, &y) != 2) {
			morpthis_loge("host", "script: bad touch \"%s\"", command);
			return false;
		}
		host_input_dispatch_touch(MORPTHIS_TOUCH_DOWN, x, y);
		host_input_dispatch_touch(MORPTHIS_TOUCH_UP, x, y);
		return true;
	}

	if (strncmp(command, "press:", 6) == 0) {
		morpthis_button_t button;
		char *name = trim(command + 6);
		if (!parse_button(name, &button)) {
			morpthis_loge("host", "script: unknown button \"%s\"", name);
			return false;
		}
		host_input_dispatch_button(button, MORPTHIS_INPUT_PRESS);
		host_input_dispatch_button(button, MORPTHIS_INPUT_RELEASE);
		return true;
	}

	if (strncmp(command, "sleep:", 6) == 0) {
		char *end = NULL;
		long ms = strtol(trim(command + 6), &end, 10);
		if (!end || *end != '\0' || ms < 0) {
			morpthis_loge("host", "script: bad sleep \"%s\"", command);
			return false;
		}
		// The player's loop keeps running while time passes, so a game's clock
		// advances during a sleep too. Pump the posted queue as it goes rather
		// than only at the end, or a screen that animates would show one frame.
		while (ms > 0 && !host_quit_requested()) {
			long slice = ms > 5 ? 5 : ms;
			sleep_ms(slice);
			host_run_posted();
			host_run_timers();
			ms -= slice;
		}
		return true;
	}

	morpthis_loge("host", "script: unknown command \"%s\"", command);
	return false;
}

bool host_run_script(const char *script) {
	if (!script || !script[0]) {
		return true;
	}

	char *copy = malloc(strlen(script) + 1);
	if (!copy) {
		return false;
	}
	strcpy(copy, script);

	bool ok = true;
	char *cursor = copy;
	while (cursor && ok) {
		char *next = strchr(cursor, ';');
		if (next) {
			*next++ = '\0';
		}
		char *command = trim(cursor);
		if (command[0]) {
			ok = run_command(command);
		}
		cursor = next;
	}

	free(copy);
	return ok;
}

static void run_context(const char *folder) {
	char id[HOST_ID_MAX];
	const char *env_id = getenv("MORPTHIS_APP_ID");
	const char *env_name = getenv("MORPTHIS_APP_NAME");
	const char *env_version = getenv("MORPTHIS_APP_VERSION");

	if (env_id && env_id[0]) {
		snprintf(id, sizeof(id), "%s", env_id);
	} else {
		const char *base = strrchr(folder, '/');
		snprintf(id, sizeof(id), "%s", base && base[1] ? base + 1 : folder);
	}

	host_runtime_init(id, env_name && env_name[0] ? env_name : id,
					  env_version && env_version[0] ? env_version : "0.0.0", folder);
}

int main(int argc, char **argv) {
	(void)argc;
	(void)argv;

	const char *folder = getenv("MORPTHIS_APP_FOLDER");
	if (!folder || !folder[0]) {
		folder = ".";
	}
	run_context(folder);

	morpthis_logi("host", "starting %s (%s)", morpthis_app_id(host_app()),
				  morpthis_app_folder(host_app()));

	int rc = morpthis_app_main(host_app());
	if (rc != 0) {
		morpthis_loge("host", "entry point returned %d", rc);
		return rc;
	}

	const char *script = getenv("MORPTHIS_HOST_SCRIPT");
	if (script && script[0] && !host_run_script(script)) {
		morpthis_loge("host", "test script failed");
		return 2;
	}

	long run_ms = 250;
	const char *run_env = getenv("MORPTHIS_RUN_MS");
	if (run_env && run_env[0]) {
		char *end = NULL;
		long value = strtol(run_env, &end, 10);
		if (end && *end == '\0' && value >= 0) {
			run_ms = value;
		}
	}

	uint32_t deadline = morpthis_app_time_ms() + (uint32_t)run_ms;
	while (!host_quit_requested()) {
		host_run_posted();
		host_run_timers();
		if (host_quit_requested()) {
			break;
		}
		if ((int32_t)(morpthis_app_time_ms() - deadline) >= 0) {
			break;
		}
		sleep_ms(1);
	}
	host_run_posted();
	host_run_timers();

	if (!host_quit_requested()) {
		morpthis_logi("host", "run ended after %ld ms without quit", run_ms);
	}
	return 0;
}
