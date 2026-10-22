#include <morpthis/power.h>

// The host has no suspend state and no power timer. Keep the hold set so an
// app's queries and teardown still behave, but there is nothing to apply.
static morpthis_power_t g_holds;
static morpthis_power_event_cb g_event_cb;
static void *g_event_user;

void morpthis_power_set_holds(morpthis_power_t holds) { g_holds = holds; }

void morpthis_power_set_holds_for(morpthis_power_t holds, uint32_t timeout_ms) {
	(void)timeout_ms;
	g_holds = holds;
}

void morpthis_power_release_all(void) { g_holds = MORPTHIS_POWER_NONE; }

morpthis_power_t morpthis_power_holds(void) { return g_holds; }

void morpthis_power_set_event_cb(morpthis_power_event_cb cb, void *user) {
	g_event_cb = cb;
	g_event_user = user;
}
