#ifndef MORPTHIS_SYSTEM_H
#define MORPTHIS_SYSTEM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// The device itself: what it is, how much battery and card are left, the
// screen, the clock, the card's USB state and the player's own settings store.
//
// These are the calls the local Settings and status bar use. An app may use
// them to show the same information or to change the same things; nothing here
// is sandboxed. Powering off and rebooting are deliberately in this header and
// do exactly what the power menu does. All calls run on the GUI thread unless
// their comment says otherwise.

#define MORPTHIS_DEVICE_NAME_MAX 64
#define MORPTHIS_VERSION_MAX 32
#define MORPTHIS_SERIAL_MAX 32
#define MORPTHIS_DAC_MAX 64

typedef struct {
	char device_name[MORPTHIS_DEVICE_NAME_MAX]; // "HiBy R3 Pro II", "TempoTec V1", ...
	char os_version[MORPTHIS_VERSION_MAX];
	char build_version[MORPTHIS_VERSION_MAX];
	char serial[MORPTHIS_SERIAL_MAX];
	char dac[MORPTHIS_DAC_MAX];
} morpthis_system_info_t;

typedef struct {
	bool present;
	uint64_t total;
	uint64_t used;
	uint64_t free;
	int used_percent; // 0..100
} morpthis_storage_t;

void morpthis_system_info(morpthis_system_info_t *out);
bool morpthis_system_storage(morpthis_storage_t *out);

// Percent, or -1 when the battery level cannot be read.
int morpthis_system_battery_percent(void);
bool morpthis_system_charging(void);

// A cable is supplying VBUS (a PC or a charger), and the card is exported to a
// computer right now.
bool morpthis_system_usb_connected(void);
bool morpthis_system_mass_storage_active(void);

// The panel. brightness() is a percentage of the panel's range; set_brightness
// clamps to the player's 5 % floor and persists the value like the Settings
// slider. Call it when a drag ends, not on every pixel: each call writes the
// config. hold_screen() keeps the idle timer from blanking the panel, for a
// page the user is expected to read before touching again. It is independent
// of the player's own holds and safe from any thread. The combined and timed
// form of this and the other power holds is in power.h.
bool morpthis_system_screen_is_on(void);
void morpthis_system_screen_on(void);
void morpthis_system_screen_off(void);
void morpthis_system_screen_toggle(void);
int morpthis_system_brightness(void);
bool morpthis_system_set_brightness(int percent);
void morpthis_system_hold_screen(bool hold);

// A single-bit convenience for morpthis_power_set_holds() in power.h. It keeps
// the device out of standby while true; the screen may still turn off. Raw PCM
// output does not imply it. For a streaming app, use the power.h NETWORK bit
// as well so Wi-Fi is not parked. Safe from any thread; clear it on pause or
// stop. The player clears it automatically when the app is torn down.
void morpthis_system_hold_awake(bool hold);

// Wall clock, seconds since the epoch. set_time() writes the RTC as well.
bool morpthis_system_time(int64_t *epoch_seconds);
bool morpthis_system_set_time(int year, int month, int day, int hour, int minute);

// The screenshot gesture's job: capture the current frame into the
// Screenshots folder on the card. Asynchronous; the file appears shortly.
void morpthis_system_screenshot(void);

// The power menu's two actions. Neither returns.
void morpthis_system_poweroff(void);
void morpthis_system_reboot(void);

// The player's own INI store, [section] key = value, at
// /usr/data/device_config.ini. get_string() copies into `out` and returns the
// fallback when the key is absent. Setters only touch memory; save() writes.
bool morpthis_config_get(const char *section, const char *key, const char *fallback,
						 char *out, size_t size);
long morpthis_config_get_int(const char *section, const char *key, long fallback);
bool morpthis_config_get_bool(const char *section, const char *key, bool fallback);
void morpthis_config_set(const char *section, const char *key, const char *value);
void morpthis_config_set_int(const char *section, const char *key, long value);
void morpthis_config_set_bool(const char *section, const char *key, bool value);
bool morpthis_config_save(void);

#ifdef __cplusplus
}
#endif

#endif /* MORPTHIS_SYSTEM_H */
