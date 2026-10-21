#ifndef MORPTHIS_APP_H
#define MORPTHIS_APP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "morpthis/version.h"

#ifdef __cplusplus
extern "C" {
#endif

// Application lifetime.
//
// An app is a shared object the player loads into its own process. The player
// owns the GUI thread, the display, the touch panel, the ALSA device and the
// DAC; the app reaches them only through this API.
//
// The player calls morpthis_app_main() once, on the GUI thread, after it has
// created the app's screen and before it starts dispatching events. The entry
// point sets up the UI and returns. It must not block the GUI thread: put long
// work on a thread the app owns and hand the result back with
// morpthis_app_post(). The screen lives until the app calls
// morpthis_app_quit(), or until the player unloads it.
//
// The entry point returns 0 when the app started and a non-zero value when it
// could not. In the latter case the player tears the app down and reports the
// failure. It is not a request to exit; use morpthis_app_quit() for that.
//
// The app context is owned by the player and remains valid until the app is
// unloaded. Callbacks receive it through their user pointer, or the app can
// keep it in a static of its own.

typedef struct morpthis_app morpthis_app_t;

// A callback posted from an app thread and run on the GUI thread.
typedef void (*morpthis_post_fn)(void *user);

// Implemented by the app. It is exported; everything else the app defines is
// hidden.
MORPTHIS_EXPORT int morpthis_app_main(const morpthis_app_t *app);

// Optional, implemented by an app that owns threads, a network connection or
// the raw audio output. The player calls it once before it destroys the app's
// screens and unloads the shared object, including when it closes the app to
// hand the card to a computer in USB mass-storage mode or to start another app.
//
// It runs on a worker thread, not the GUI thread, and the player waits at most
// a few seconds for it: stop and join the app's threads and close anything the
// app opened, but do not touch the UI and do not block on the GUI thread. A
// shutdown that does not return is force-closed: the player releases the audio
// output and leaves the shared object and its screens in place rather than
// unmapping code a stuck thread may still be inside. The call must be
// idempotent because an app that already called morpthis_app_quit() has stopped
// its own work.
MORPTHIS_EXPORT void morpthis_app_shutdown(void);

// Implemented by the app. It returns the MORPTHIS_APP_API_VERSION the app was
// compiled with, so the player can refuse a plugin built for a newer major API
// before it calls the entry point. The player may treat a missing symbol as
// API 1, but new apps should export it.
MORPTHIS_EXPORT int morpthis_app_api_required(void);

// Defines morpthis_app_api_required() from the header the app compiled
// against. Put it once at file scope in the app's main source:
//
//     MORPTHIS_DEFINE_API_VERSION();
#define MORPTHIS_DEFINE_API_VERSION()                                          \
	MORPTHIS_EXPORT int morpthis_app_api_required(void) {                     \
		return MORPTHIS_APP_API_VERSION;                                       \
	}

// Identity, as read from app.json. Never NULL; empty when the manifest omitted
// the field and the player had no default.
const char *morpthis_app_id(const morpthis_app_t *app);
const char *morpthis_app_name(const morpthis_app_t *app);
const char *morpthis_app_version(const morpthis_app_t *app);

// The absolute path of the mounted package root. Files shipped beside the
// entry are found relative to this; morpthis_fs_app_folder() is the same
// directory. It is read-only, and it is only valid while the app is loaded.
// Files the app writes belong in morpthis_fs_data_folder().
const char *morpthis_app_folder(const morpthis_app_t *app);

// The API version the player implements. Compare it with
// MORPTHIS_APP_API_VERSION at startup and quit if the major differs. The player
// has already done this check; an app that was compiled against a newer SDK
// still cannot run.
int morpthis_app_api_version(const morpthis_app_t *app);

// Bytes the app can reasonably allocate before the device runs out. Returns
// SIZE_MAX when the player cannot tell. The budget is what the player's
// MemAvailable reports, not a per-app quota: another app or a large buffer
// elsewhere in the player reduces it.
size_t morpthis_app_mem_available(void);

// Milliseconds since boot, monotonic. Use it for animation and timestamps, not
// for wall-clock time.
uint32_t morpthis_app_time_ms(void);

// Runs fn(user) on the GUI thread. Safe to call from any thread, including
// before the entry point returns. The app owns the callback's arguments; they
// must stay valid until it runs.
void morpthis_app_post(morpthis_post_fn fn, void *user);

// Destroys the app's screen, unloads its resources and returns to the Apps
// page. Idempotent. Call it from a GUI-thread callback (a button, a posted
// function, an input event), not from a worker thread.
void morpthis_app_quit(void);

// Leaves the app in the background: the player goes back to the Apps page while
// the app's screens stay built and its threads and raw audio output keep
// running. This is what lets a music app play while the user browses the
// player. Returns false when there is no app to background. Call it from the
// GUI thread.
bool morpthis_app_background(void);

// Tells the player that the app handled BACK itself by moving to another of its
// own screens. The chevron and the swipe-back keep the app loaded then; without
// it they end the app, because a non-game app's BACK is the way out. Call it
// after loading the parent page on the GUI thread; it is only read for the BACK
// event that is being delivered.
void morpthis_app_back_handled(void);

#ifdef __cplusplus
}
#endif

#endif /* MORPTHIS_APP_H */
