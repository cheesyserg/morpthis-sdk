#ifndef MORPTHIS_APP_H
#define MORPTHIS_APP_H

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

// The absolute path of the app's own folder. Files next to the app are found
// relative to this; morpthis_fs_app_folder() is the same directory.
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

#ifdef __cplusplus
}
#endif

#endif /* MORPTHIS_APP_H */
