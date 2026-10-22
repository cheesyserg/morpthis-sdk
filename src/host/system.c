#include "internal.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

// A headless machine: the status calls answer sensibly, the controls are
// harmless no-ops, and the settings store is an in-memory fallback-free stub.

static int g_brightness = 50;
static bool g_screen_on = true;

void morpthis_system_info(morpthis_system_info_t *out) {
	if (!out) {
		return;
	}
	memset(out, 0, sizeof(*out));
	snprintf(out->device_name, sizeof(out->device_name), "Host");
	snprintf(out->os_version, sizeof(out->os_version), "host");
	snprintf(out->build_version, sizeof(out->build_version), "0");
}

bool morpthis_system_storage(morpthis_storage_t *out) {
	if (out) {
		memset(out, 0, sizeof(*out));
	}
	return false;
}

int morpthis_system_battery_percent(void) { return -1; }
bool morpthis_system_charging(void) { return false; }
bool morpthis_system_usb_connected(void) { return false; }
bool morpthis_system_mass_storage_active(void) { return false; }

bool morpthis_system_screen_is_on(void) { return g_screen_on; }
void morpthis_system_screen_on(void) { g_screen_on = true; }
void morpthis_system_screen_off(void) { g_screen_on = false; }
void morpthis_system_screen_toggle(void) { g_screen_on = !g_screen_on; }

int morpthis_system_brightness(void) { return g_brightness; }

bool morpthis_system_set_brightness(int percent) {
	if (percent < 0) {
		percent = 0;
	}
	if (percent > 100) {
		percent = 100;
	}
	g_brightness = percent < 5 ? 5 : percent;
	return true;
}

void morpthis_system_hold_screen(bool hold) { (void)hold; }

bool morpthis_system_time(int64_t *epoch_seconds) {
	if (!epoch_seconds) {
		return false;
	}
	*epoch_seconds = (int64_t)time(NULL);
	return true;
}

bool morpthis_system_set_time(int year, int month, int day, int hour, int minute) {
	(void)year;
	(void)month;
	(void)day;
	(void)hour;
	(void)minute;
	return true;
}

void morpthis_system_screenshot(void) {}
void morpthis_system_poweroff(void) {}
void morpthis_system_reboot(void) {}

bool morpthis_config_get(const char *section, const char *key, const char *fallback,
						 char *out, size_t size) {
	(void)section;
	(void)key;
	if (!out || size == 0) {
		return false;
	}
	snprintf(out, size, "%s", fallback ? fallback : "");
	return true;
}

long morpthis_config_get_int(const char *section, const char *key, long fallback) {
	(void)section;
	(void)key;
	return fallback;
}

bool morpthis_config_get_bool(const char *section, const char *key, bool fallback) {
	(void)section;
	(void)key;
	return fallback;
}

void morpthis_config_set(const char *section, const char *key, const char *value) {
	(void)section;
	(void)key;
	(void)value;
}

void morpthis_config_set_int(const char *section, const char *key, long value) {
	(void)section;
	(void)key;
	(void)value;
}

void morpthis_config_set_bool(const char *section, const char *key, bool value) {
	(void)section;
	(void)key;
	(void)value;
}

bool morpthis_config_save(void) { return true; }
