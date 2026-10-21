# The API

Everything an app may call is declared under `include/morpthis/`. This document
is the behavioural contract; the headers are the source of truth for types.

All calls run on the GUI thread unless stated otherwise. All pointers returned
by the SDK are owned by the player and valid for the app's lifetime.

## Entry point

```c
int morpthis_app_main(const morpthis_app_t *app);
```

The player calls it once per launch, on the GUI thread, after reading the
manifest. Set up screens and callbacks and return. A zero return means the app
started; non-zero means it did not, and the player reports the failure and
tears the app down.

The function is exported (`MORPTHIS_EXPORT` is declared on it). The app is
compiled with hidden visibility; everything else in it stays private.

An app also exports the API it was built against, so the player can refuse it
before calling in:

```c
MORPTHIS_DEFINE_API_VERSION();
```

Put that once at file scope. It defines
`int morpthis_app_api_required(void)`, which returns
`MORPTHIS_APP_API_VERSION` from the header the app compiled with. The player
treats a missing symbol as API 1 but refuses a newer major.

The manifest's `tier` decides what the player does before loading: a `light`
app leaves playback and caches alone; a `heavy` app asks the player to stop
playback and release its audio path and SQLite page caches first. It is still an
in-process app with the player's UI and back gesture. Use `heavy` only for an
app that cannot fit otherwise, and do not expect static player pages to go
away.

## app.h

| Call | Notes |
| --- | --- |
| `morpthis_app_id/name/version(app)` | Manifest values, never NULL. |
| `morpthis_app_folder(app)` | Absolute path of the app's own folder. |
| `morpthis_app_api_version(app)` | The API the player implements. |
| `morpthis_app_mem_available()` | Bytes available; `SIZE_MAX` when unknown. |
| `morpthis_app_time_ms()` | Monotonic milliseconds, wraps at 32 bits. |
| `morpthis_app_post(fn, user)` | Runs on the GUI thread. Any thread may call. |
| `morpthis_app_quit()` | Destroys the screen and returns to Apps. |

`morpthis_app_main` must return before the player starts dispatching events.
A callback that needs to do slow work starts a thread, then posts the result.
The app owns the arguments until the posted callback runs.

## ui.h

Coordinates are pixels, origin top-left. Use `morpthis_ui_width()` and
`morpthis_ui_height()` (240×320 on the V1) instead of hard-coding.

| Call | Notes |
| --- | --- |
| `morpthis_screen_create(title)` | One per view. Returns NULL on failure. |
| `morpthis_screen_load(screen)` | Shows it; the previous screen stays alive. |
| `morpthis_screen_active()` | The loaded screen, or NULL. |
| `morpthis_screen_destroy(screen)` | Frees the screen and its widgets. |
| `*_create(screen, ...)` | label, button, list, image, slider, canvas. |
| `morpthis_widget_set_text/text` | Labels and buttons. |
| `morpthis_widget_set_pos/size` | Absolute layout in pixels. |
| `morpthis_widget_set_visible/enabled` | Disabled widgets do not fire. |
| `morpthis_canvas_buffer(canvas)` | The app's RGB565 pixels; stride = width. |
| `morpthis_canvas_invalidate(canvas, x, y, w, h)` | Redraws that part on the next refresh. |
| `morpthis_ui_refresh()` | Draws the invalidated parts now. |
| `morpthis_widget_set_rotation` | Image widgets, clockwise, around the centre. |
| `morpthis_widget_set_opacity` | 0..100, the widget and its children. |
| `morpthis_widget_raise` | Draws it above its siblings. |
| `morpthis_widget_destroy` | Removes it from the screen and frees it. |
| `morpthis_list_add/clear/count` | Items are copied. |
| `morpthis_list_set_select_cb` | `(widget, index, user)`. |
| `morpthis_slider_set_value/value` | Clamped to the creation range. |
| `morpthis_toast(text)` | One line in the player's toast area. |

Buttons take `morpthis_click_cb(widget, user)`. Sliders fire their value
callback when the user moves them and when `morpthis_slider_set_value()` is
called. An image path is relative to the app folder; the player decodes it on
the GUI thread at creation and whenever its size changes, so it is for icons and
small pictures, not a full-screen photograph. A picture is never enlarged: ship
it at the size it is drawn.

Rotation is for images and turns around the widget's centre; the box keeps its
size, so a turned picture may draw outside it. Opacity and `raise` are what an
overlay uses: a full-screen image at 0..100 percent, brought to the front when
it is shown, is a hit flash or a fade.

`morpthis_canvas_create()` is the other way to draw: a framebuffer the app owns,
RGB565, `width` x `height`, stride = width. The app writes pixels and calls
`morpthis_canvas_invalidate()` for the rectangle it changed; the player blits
that part of the canvas on the next refresh. A game that redraws its own frames
uses this instead of moving widgets: it is one image draw per changed area, not
a layout pass over a tree. If the app has its own clock, it calls
`morpthis_ui_refresh()` after the invalidations so the panel shows that frame
in the same pass, instead of waiting for the display timer's phase. The canvas
and image widgets do not take presses: a touch on them reaches the app's touch
callback, as a touch on the page would.

Destroying the screen that is currently on display requests a quit rather than
deleting LVGL's active page underneath it. Quit with `morpthis_app_quit()`.

The look is the player's: theme, fonts and scale are not app settings in API 1.

## input.h

Physical buttons and raw touch, for an app that draws its own surface. Set one
callback each; setting NULL unsubscribes.

Buttons: `PLAY_PAUSE`, `NEXT`, `PREV`, `VOLUME_UP`, `VOLUME_DOWN`, `POWER`,
`BACK`. Actions: `PRESS`, `RELEASE`, `REPEAT`. Touch actions: `DOWN`, `MOVE`,
`UP`, `CANCEL`, with the point in display pixels.

The physical transport keys are delivered as `PRESS` only: this device reports a
press and a release at the kernel but the player has no release event to pass
on. `BACK` is delivered as `PRESS` then `RELEASE` by the chevron and swipe, and
the player then quits the app. While a game is in active play, the player
offers it the volume keys as repeated `PRESS` events; call
`morpthis_input_set_gameplay(true/false)` as the game enters and leaves play,
menus and pauses. A game that does not call it, or has no button callback,
leaves the volume keys to the player. On other screens the volume keys stay
the player's; read `morpthis_audio_volume()` or set it explicitly.

Widget taps are delivered to the widget, not to the touch callback.

## audio.h

An app that never calls these leaves the player's playback alone. An app that
calls `morpthis_audio_play()` takes the device and should pause before it
quits, unless it is itself a player.

| Call | Notes |
| --- | --- |
| `morpthis_audio_current(track)` | False when nothing is loaded; zeroes `track`. |
| `morpthis_audio_state()` | `STOPPED`, `PLAYING`, `PAUSED`. |
| `play/pause/next/prev` | False when the player cannot honour it. |
| `morpthis_audio_volume()` | 0..100. |
| `morpthis_audio_set_volume(v)` | Clamps. |
| `morpthis_audio_open(rate, channels, buffer_ms)` | Takes the output; stops playback and radio. |
| `morpthis_audio_write(pcm, frames)` | Signed 16-bit interleaved; blocks at DAC pace. |
| `morpthis_audio_close()` | Releases the output; playback stays stopped. |

`morpthis_track_t` has fixed arrays and an `int duration_ms`; its layout is
part of API 1.

The raw output is for an app that mixes its own sound (a game's effects). Open,
write and close on the same thread: a write blocks until the device has room,
which is what paces a mixer. There is no hardware mixing, so taking the output
stops whatever the player was playing; the app can close it and call
`morpthis_audio_play()` to hand it back.

## fs.h

Every path is relative to the app folder. Absolute paths, backslashes and any
path containing `..` are refused with `errno = EINVAL`.

| Call | Notes |
| --- | --- |
| `morpthis_fs_app_folder()/card_root()` | Absolute, owned by the player. |
| `morpthis_fs_open(path, mode)` | Modes: read, write, append, read/write. |
| `morpthis_fs_read/write` | Return bytes transferred. |
| `morpthis_fs_seek/tell` | `fseek`/`ftell` semantics. |
| `morpthis_fs_close` | Frees the handle; returns the close result. |
| `morpthis_fs_write_all` | Creates or truncates; true on a complete write. |
| `morpthis_fs_read_all` | NUL-terminates, bounded by `max`. |
| `morpthis_fs_exists/remove/rename/mkdir` | `0` on success, `-1` otherwise. |

The card may disappear at any time. Check every write; do not assume a file
stays open across a UI transition.

## log.h

```c
morpthis_logi("calc", "started, api %d", api);
morpthis_loge("calc", "could not open %s", path);
```

Levels are debug, info, warn and error. The player prefixes the app id and
writes one line. Log a start line and an exit line, and warnings when something
fails; the device's log buffer is shared.

```c
MORPTHIS_REQUIRE_API(1, 0); // after the headers, at file scope
```

fails the build if the installed SDK is older than the API a source asks for.
It cannot check the player; the player checks at load time.

## Host test scripts

The host runner reads `MORPTHIS_HOST_SCRIPT`, a semicolon-separated list:

| Command | Effect |
| --- | --- |
| `click:<label>` | Clicks the visible button with that exact text. |
| `press:<button>` | Dispatches press and release for `play`, `next`, `prev`, `volup`, `voldown`, `power` or `back`. |
| `touch:<x>,<y>` | Dispatches touch down and up at a display pixel. |
| `sleep:<ms>` | Waits, running the posted queue as it goes, as the player's loop would. |
| `quit` | Calls `morpthis_app_quit()`. |

A missing button or an unknown command makes the runner fail with status 2, so
CI catches a screen that lost its control. `MORPTHIS_RUN_MS` bounds a run that
does not quit; the default is 250 ms.
