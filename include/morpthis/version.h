#ifndef MORPTHIS_VERSION_H
#define MORPTHIS_VERSION_H

// The SDK version an app was built against. The API version is separate: the
// SDK can gain source-compatible calls without breaking players that export an
// older API, and a player refuses an app built for a newer major API.
#define MORPTHIS_SDK_VERSION_MAJOR 1
#define MORPTHIS_SDK_VERSION_MINOR 2
#define MORPTHIS_SDK_VERSION_PATCH 0
#define MORPTHIS_SDK_VERSION_STRING "1.2.0"

// The version of the plugin ABI. This is the number the player compares with
// morpthis_app_api_version() before it calls the entry point.
//
// 2 adds the hi-res audio output (open_ex/write_pcm), the status-bar playing
// flag (set_playing) and background operation (morpthis_app_background).
//
// 3 gives an app the player's own services instead of only its own surface:
// local playback and queue control (playback.h), the equalizer, the music
// library, favourites and playlists (library.h), the HTTP/TLS client (net.h),
// device status, brightness, the clock, USB and the settings store (system.h),
// Wi-Fi and Bluetooth (wireless.h), absolute-path filesystem access and
// directory walks (fs.h), the player's theme, status bar, modal dialogs, text
// entry and full widget set (ui.h), and app background state
// (morpthis_app_is_background).
//
// An API 2 app still loads on an API 3 player; an API 3 app is refused by an
// older player, which is why it should say min_player 3 in its manifest.
#define MORPTHIS_APP_API_VERSION 3

// Major and minor packed into one integer, for comparisons.
#define MORPTHIS_API_VERSION_ENCODE(major, minor) \
	((((int)(major)) << 16) | (((int)(minor)) & 0xffff))
#define MORPTHIS_SDK_API_VERSION \
	MORPTHIS_API_VERSION_ENCODE(MORPTHIS_SDK_VERSION_MAJOR, MORPTHIS_SDK_VERSION_MINOR)

// Put this at file scope in one source file, after the SDK headers, to fail the
// build when the source asks for an API newer than the installed SDK:
//
//     MORPTHIS_REQUIRE_API(1, 2);
//
// It checks the SDK, not the player. The player check happens at load time.
#define MORPTHIS_REQUIRE_API(major, minor)                                      \
	typedef char morpthis_api_requires_##major##_##minor                        \
		[(MORPTHIS_SDK_API_VERSION >= MORPTHIS_API_VERSION_ENCODE(major, minor)) \
			 ? 1                                                                \
			 : -1]

// Export marker for the entry point and the optional shutdown hook. An app is
// built with hidden visibility so only the symbols declared with this are
// reachable through dlsym(); the declarations in app.h carry it, and the
// definitions inherit it.
#if defined(_WIN32)
#define MORPTHIS_EXPORT __declspec(dllexport)
#else
#define MORPTHIS_EXPORT __attribute__((visibility("default")))
#endif

#endif /* MORPTHIS_VERSION_H */
