#ifndef MORPTHIS_POWER_H
#define MORPTHIS_POWER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Power holds and power notifications for apps.
//
// A hold is a statement about what the app is doing, not a promise that the
// device will stay awake forever: the player still applies its own battery,
// charging, USB and thermal policy, and a hold that is cleared or expires is
// gone. The holds are owned by the loaded app, survive while it is in the
// background, and are cleared automatically when it is torn down, so an app
// that crashes cannot leave one behind.
//
// Putting the app in the background releases a SCREEN hold, because the pages
// it was keeping lit are hidden. AWAKE, NETWORK, a timed hold's deadline and
// the event callback all stay: they describe work the app is still doing.
//
//   MORPTHIS_POWER_AWAKE    no standby-to-RAM while the hold is present
//   MORPTHIS_POWER_SCREEN   the idle timer does not blank the panel
//   MORPTHIS_POWER_NETWORK  the Wi-Fi radio is not parked, and the hold also
//                           keeps the device awake
//
// The screen may still be turned off by the user or by a forced timeout while
// AWAKE is held; SCREEN is the separate "do not blank it" request. NETWORK is
// stronger than AWAKE because a parked radio cannot finish a transfer. A
// connected Bluetooth link or audio route already counts as in use, so NETWORK
// is about keeping Wi-Fi up.
//
// SCREEN is independent of the player's own screen holds: an app releasing its
// hold cannot release a player page's hold, or the reverse.
typedef enum {
	MORPTHIS_POWER_NONE = 0,
	MORPTHIS_POWER_AWAKE = 1u << 0,
	MORPTHIS_POWER_SCREEN = 1u << 1,
	MORPTHIS_POWER_NETWORK = 1u << 2,
} morpthis_power_t;

// Replaces the app's complete hold set. The holds stay until this is called
// again, the app is torn down, or a timed hold expires. Safe from any thread.
void morpthis_power_set_holds(morpthis_power_t holds);

// The same, but the whole set expires after `timeout_ms`. A timeout of 0 means
// no expiry. The player processes the deadline on its power tick (a quarter of
// a second), so a timed hold is not a precision timer. `hold_screen()` and
// `hold_awake()` change one bit without cancelling the deadline; use this call
// with timeout 0 to turn a timed set into an indefinite one.
void morpthis_power_set_holds_for(morpthis_power_t holds, uint32_t timeout_ms);

// Releases every hold the app owns. The same happens on app teardown.
void morpthis_power_release_all(void);

// The app's current hold set. Expiry is processed by the power tick, so a
// query immediately after a deadline may still show the old set for one tick.
morpthis_power_t morpthis_power_holds(void);

// Power state events. They run on the GUI thread, must return promptly, and
// must not block or wait for another thread.
//
// SUSPENDING is sent only when the player has already decided to suspend: a
// hold set there cannot stop that attempt. RESUMED is sent after the wake,
// including after a suspend attempt that failed. It is sent only to the same
// callback that received SUSPENDING, so replacing the callback across a sleep
// does not produce an unpaired resume.
//
// SCREEN_OFF is sent after the panel and its rendering timers are down;
// SCREEN_ON is sent after the panel is back up and has been repainted. The
// current state is also available from morpthis_system_screen_is_on(). A
// SCREEN_OFF callback must not assume it can draw; save state and stop work.
//
// Registering NULL clears the callback; app teardown does the same.
typedef enum {
	MORPTHIS_POWER_SUSPENDING = 0,
	MORPTHIS_POWER_RESUMED,
	MORPTHIS_POWER_SCREEN_OFF,
	MORPTHIS_POWER_SCREEN_ON,
} morpthis_power_event_t;

typedef void (*morpthis_power_event_cb)(morpthis_power_event_t event, void *user);
void morpthis_power_set_event_cb(morpthis_power_event_cb cb, void *user);

#ifdef __cplusplus
}
#endif

#endif /* MORPTHIS_POWER_H */
