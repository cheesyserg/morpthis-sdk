#include "internal.h"

#include <stdint.h>
#include <stdlib.h>

// A tiny scheduler for the host runner. The device uses LVGL timers; this is
// the same contract without a display, so an app's polling and animation code
// still runs under the test script's `sleep`.

struct morpthis_timer {
	uint32_t period_ms;
	uint32_t due_ms;
	morpthis_timer_cb cb;
	void *user;
	bool deleted;
	struct morpthis_timer *next;
};

static morpthis_timer_t *g_timers;
static morpthis_timer_t *g_graveyard;
static bool g_running;

morpthis_timer_t *morpthis_timer_create(uint32_t period_ms, morpthis_timer_cb cb, void *user) {
	if (!cb) {
		return NULL;
	}
	morpthis_timer_t *timer = calloc(1, sizeof(*timer));
	if (!timer) {
		return NULL;
	}
	timer->period_ms = period_ms ? period_ms : 1;
	timer->due_ms = morpthis_app_time_ms() + timer->period_ms;
	timer->cb = cb;
	timer->user = user;
	timer->next = g_timers;
	g_timers = timer;
	return timer;
}

void morpthis_timer_set_period(morpthis_timer_t *timer, uint32_t period_ms) {
	if (!timer) {
		return;
	}
	timer->period_ms = period_ms ? period_ms : 1;
	timer->due_ms = morpthis_app_time_ms() + timer->period_ms;
}

void morpthis_timer_reset(morpthis_timer_t *timer) {
	if (timer) {
		timer->due_ms = morpthis_app_time_ms() + timer->period_ms;
	}
}

void morpthis_timer_delete(morpthis_timer_t *timer) {
	if (!timer) {
		return;
	}
	morpthis_timer_t **link = &g_timers;
	while (*link && *link != timer) {
		link = &(*link)->next;
	}
	if (*link == timer) {
		*link = timer->next;
	}
	timer->deleted = true;
	if (g_running) {
		// A callback may delete itself or a timer later in the walk. Keep the
		// allocation until the walk is over so the loop's `next` cannot dangle.
		timer->next = g_graveyard;
		g_graveyard = timer;
	} else {
		free(timer);
	}
}

void host_run_timers(void) {
	if (g_running) {
		return;
	}
	g_running = true;
	uint32_t now = morpthis_app_time_ms();
	for (morpthis_timer_t *timer = g_timers; timer;) {
		morpthis_timer_t *next = timer->next;
		if (!timer->deleted && (int32_t)(now - timer->due_ms) >= 0) {
			timer->due_ms = now + timer->period_ms;
			timer->cb(timer, timer->user);
		}
		timer = next;
	}
	g_running = false;

	while (g_graveyard) {
		morpthis_timer_t *next = g_graveyard->next;
		free(g_graveyard);
		g_graveyard = next;
	}
}
