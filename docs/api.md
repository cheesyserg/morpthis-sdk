# The API

Everything an app may call is declared under `include/morpthis/`. This document
is the behavioural contract; the headers are the source of truth for types.

All calls run on the GUI thread unless stated otherwise. All pointers returned
by the SDK are owned by the player and valid for the app's lifetime unless the
call says otherwise.

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

// An app also exports the API it was built against, so the player can refuse it
// before calling in:

```c
MORPTHIS_DEFINE_API_VERSION();
```

Put that once at file scope. It defines `int morpthis_app_api_required(void)`,
which returns `MORPTHIS_APP_API_VERSION` from the header the app compiled with.
The player treats a missing symbol as API 1 but refuses a newer major.

API 2 added `morpthis_audio_open_ex`, `morpthis_audio_write_pcm`,
`morpthis_audio_set_playing`, `morpthis_audio_set_track`,
`morpthis_app_background`, `morpthis_app_back_handled`, the player-styled icon
buttons and text size/dim/align calls, the full-height screen,
`morpthis_image_set_source`, the popover menus and the control-centre
`NOW_PLAYING` button.

API 3 adds the player's own services: local playback and queue control
(`playback.h`), the 10-band equalizer (`audio.h`), the music library,
favourites and playlists (`library.h`), the HTTP/TLS client (`net.h`), device
status, brightness, the clock, USB, power and the settings store (`system.h`),
Wi-Fi and Bluetooth (`wireless.h`), absolute-path file access and directory
walks (`fs.h`), theme/dialog/text-entry UI (`ui.h`) and
`morpthis_app_is_background()`. The SDK version is 1.2.0 and the ABI list is
`abi/morpthis-api-3.txt`.

An app with threads, a network connection or the raw audio output should also
export an optional shutdown hook:

```c
MORPTHIS_EXPORT void morpthis_app_shutdown(void) {
    // stop the app's threads, close its audio stream and its files
}
```

The player calls it once before destroying the app's screens and `dlclose()`ing
it — including when the user confirms USB mass-storage mode and the app has to
make way for the card to be shared. It runs on a worker thread with a
three-second bound, so it must not touch the UI and must not block on the GUI
thread. It must be safe to call twice: an app that already went through
`morpthis_app_quit()` has done its own teardown. A missing symbol is fine; the
player looks it up with `dlsym()`.

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
| `morpthis_app_folder(app)` | Absolute path of the mounted package root. Read-only. |
| `morpthis_app_api_version(app)` | The API the player implements. |
| `morpthis_app_mem_available()` | Bytes available; `SIZE_MAX` when unknown. |
| `morpthis_app_time_ms()` | Monotonic milliseconds, wraps at 32 bits. |
| `morpthis_app_post(fn, user)` | Runs on the GUI thread. Any thread may call. |
| `morpthis_app_quit()` | Destroys the screen and returns to Apps. |
| `morpthis_app_background()` | Hides the app's screens and keeps it running. |
| `morpthis_app_is_background()` | True while it is in the background. |
| `morpthis_app_back_handled()` | The BACK handler moved to another of the app's screens. |

`morpthis_app_main` must return before the player starts dispatching events.
A callback that needs to do slow work starts a thread, then posts the result.
The app owns the arguments until the posted callback runs.

`morpthis_app_background()` leaves the app loaded while the player goes back to
the Apps page. Its screens stay built and its threads, sockets and raw audio
output keep running, which is what lets a music app play while the user browses
the player. A tap on its tile brings the screen back instead of starting a
second copy. On the device, an app that is playing keeps the on-card package
mounted, so USB mass storage still asks before closing it. Call it from the GUI
thread; an app that calls it from its BACK handler is left in the background
instead of being quit by the chevron.

`morpthis_app_back_handled()` is the other answer to BACK. An app with its own
pages (a library that walks up a level, a settings page returning to its menu)
calls it from the BACK handler after loading the parent screen. The chevron and
the swipe-back then keep the app loaded, exactly as they keep a player page on
screen. Without either call, BACK is the way out of a non-game app and quits it.
Only the delivery of that BACK reads the flag, so a later chevron press is a
fresh decision.

## ui.h

Coordinates are pixels, origin top-left. Use `morpthis_ui_width()` and
`morpthis_ui_height()` (240×320 on the V1) instead of hard-coding.

| Call | Notes |
| --- | --- |
| `morpthis_screen_create(title)` | One per view. Returns NULL on failure. |
| `morpthis_screen_create_full(title)` | No title row: the content starts under the status bar. |
| `morpthis_screen_load(screen)` | Shows it; the previous screen stays alive. |
| `morpthis_screen_active()` | The loaded screen, or NULL. |
| `morpthis_screen_set_title(title)` | Updates the player's heading. |
| `morpthis_screen_add_corner_button(icon, cb)` | An app action in the heading row. |
| `morpthis_screen_destroy(screen)` | Frees the screen and its widgets. |
| `morpthis_ui_full_height()` | The height a full screen gets. |
| `*_create(screen, ...)` | label, button, list, image, slider, canvas. |
| `morpthis_icon_button_create(screen, icon, size, cb)` | A player glyph on a flat hit target. |
| `morpthis_widget_set_icon(widget, icon)` | Replaces the glyph; other widgets ignore it. |
| `morpthis_widget_set_text/text` | Labels and buttons. |
| `morpthis_widget_set_text_size(widget, size)` | 14, 16, 18, 20, 22, 24, 26 or 32, snapped. |
| `morpthis_widget_set_text_dim(widget, dim)` | The player's secondary text colour. |
| `morpthis_widget_set_text_align(widget, align)` | Left, centre or right. |
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
| `morpthis_list_add_entry` | Title, dim detail, optional package icon, optional chevron. |
| `morpthis_list_set_playing` | Accent bar on one row, or -1 to clear. |
| `morpthis_list_set_select_cb` | `(widget, index, user)`. |
| `morpthis_slider_set_value/value` | Clamped to the creation range. |
| `morpthis_slider_set_release_cb` | Final value when the finger lifts; what a seek bar acts on. |
| `morpthis_image_set_source(image, path)` | Re-resolves like an fs read, so a downloaded picture works. |
| `morpthis_menu_show(anchor, items, count, cb, user)` | The player's popover next to a widget. |
| `morpthis_menu_show_row(list, row, items, count, cb, user)` | The same, anchored to a list row. |
| `morpthis_menu_close()` | Closes the open menu without calling back. |
| `morpthis_toast(text)` | One line in the player's toast area. |

Buttons take `morpthis_click_cb(widget, user)`. Sliders fire their value
callback when the user moves them and when `morpthis_slider_set_value()` is
called. A seek bar wants the release callback instead of acting on every move:
it runs once, when the finger lifts (or the press is lost), with the final
value. An image path is relative to the package root; the player decodes it on
the GUI thread at creation and whenever its size changes, so it is for icons and
small pictures, not a full-screen photograph. `morpthis_image_set_source()`
replaces the picture later and resolves it through the same tree as a read, so
an app that downloads artwork into its data folder can show it without shipping
it in the package. A picture is never enlarged: ship it at the size it is drawn.

The icon set is the player's own (`PLAY`, `PAUSE`, `PREV`, `NEXT`, `STOP`,
`QUEUE`, `MORE`, `SEARCH`, `CLOSE`), so a music app's transport looks like the
player's. `morpthis_screen_add_corner_button()` adds one app action to the
heading row, to the left of the player's own background button; a full screen
has no heading row and returns NULL. A text size is one of the player's fonts,
not a free value, and a dim label follows a theme change.

`morpthis_menu_show()` is the player's popover: a compact modal card beside an
anchor with the page dimmed behind it. A track's menu is a choice list: pass
`checked = true` on the item that is currently in force and the player draws the
tick, exactly as its own quality and theme menus do. `morpthis_menu_show_row()`
anchors to a row of an SDK list, which is what a settings page wants, so the
menu opens next to the row the user tapped rather than at the page edge. Labels
are copied when the call returns, and the callback runs after the menu has
closed. A tap outside or `morpthis_menu_close()` closes it without a callback.
One menu is open at a time, and the player closes it if the app unloads.

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

The look is the player's: theme, fonts and scale are not app settings in API
2. API 3 can read the palette (`morpthis_ui_theme()`) and switch Dark/Light or
the accent, but fonts and layout scale still belong to the player.

API 3's other UI calls are the player's own chrome rather than new widgets.
`morpthis_ui_set_status_bar_visible()` shows or hides the status bar (a game
screen keeps it hidden). `morpthis_ui_confirm()` puts up the player's modal
yes/no card and runs the callback after it closes; `morpthis_ui_notice()` is the
one-button form. `morpthis_ui_prompt_text()` opens a themed card with the
player's keyboard and reports the text once, so an app that needs a server
address or a passphrase does not have to build a text-entry page. On the
headless host the modal calls return false, like the popover menus.

### The full widget set (API 3)

Beyond label, button, list, image, slider and canvas, the SDK wraps the rest of
the player's interface vocabulary:

| Call | What it is |
| --- | --- |
| `morpthis_switch_create/is_on/set_on` | The themed on/off switch. |
| `morpthis_progress_create/set_value/value/set_range` | A thin accent bar. |
| `morpthis_spinner_create` | The player's rotating loader. |
| `morpthis_container_create/set_layout/set_scrollable` | Transparent box, row/column flex layout, optional vertical scrolling. |
| `morpthis_widget_set_parent` | Moves a widget into a container; children follow `MORPTHIS_SIZE_CONTENT` when the container lays them out. |
| `morpthis_segmented_create/set_selected/selected` | The row of pill choices. |
| `morpthis_settings_row_create/set_value` | The player's settings row: title, dim detail, value or chevron. |
| `morpthis_text_input_create/set_text/get_text` | An inline field that opens the player's keyboard modal on tap. |
| `morpthis_qrcode_create/set_text` | A QR code in the theme's colours. |
| `morpthis_roller_create/set_selected/selected` | A wheel of options (the sleep-timer/date rollers are built from these). |
| `morpthis_arc_create/set_value/value/set_range` | A circular gauge, for a battery ring or a knob. |
| `morpthis_widget_set_text_wrap/scroll` | Wrapped text and the player's delayed marquee. |
| `morpthis_list_scroll_to` | Brings a list row into view. |
| `morpthis_slider_set_range` | Changes a slider's range without firing its callback. |
| `morpthis_timer_create/set_period/reset/delete` | A repeating GUI-thread timer, deleted with the app. |
| `morpthis_widget_set_bg_color/text_color/radius/padding` | Local style overrides on top of the theme. |
| `morpthis_toast_success/error/busy/busy_end` | The player's other toast cards. |
| `morpthis_ui_modal_show/progress/hide` | The full-screen modal with an optional progress bar. |

`morpthis_icon_t` carries the player's own glyphs -- transport, chevrons, album,
artist, folder, file, music, playlist, heart, star, shuffle, repeat, sort, check,
plus, delete, refresh, info, alert, Wi-Fi, Bluetooth, battery, charging,
equalizer, settings, sleep, sun, the three volume states, headphones, power and
reboot -- so an app's icons are the interface's icons rather than a second set.

## input.h

Physical buttons and raw touch, for an app that draws its own surface. Set one
callback each; setting NULL unsubscribes.

Buttons: `PLAY_PAUSE`, `NEXT`, `PREV`, `VOLUME_UP`, `VOLUME_DOWN`, `POWER`,
`BACK`, `NOW_PLAYING`. Actions: `PRESS`, `RELEASE`, `REPEAT`. Touch actions:
`DOWN`, `MOVE`, `UP`, `CANCEL`, with the point in display pixels.

The physical transport keys are delivered as `PRESS` only: this device reports a
press and a release at the kernel but the player has no release event to pass
on. `BACK` is delivered as `PRESS` then `RELEASE` by the chevron and swipe, and
the player then quits the app unless the app calls `morpthis_app_background()`
(for a music app that keeps playing) or `morpthis_app_back_handled()` (for one
that moved to another of its own screens) from its BACK handler. While a game
is in active play, the previous-track key and the volume keys repeat for as
long as they are held; call `morpthis_input_set_gameplay(true/false)` as the
game enters and leaves play, menus and pauses. Call `morpthis_input_set_game_menu(true/false)` while
the game's own menu is open: the volume keys are then offered to the game (for
menu navigation or text entry) instead of changing the player's volume. A game
that does not call either, or has no button callback, leaves the volume keys to
the player. On other screens the volume keys stay the player's; read
`morpthis_audio_volume()` or set it explicitly.

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
| `morpthis_audio_open_ex(rate, channels, bits, buffer_ms)` | The same, at 16, 24 or 32 bits. |
| `morpthis_audio_write(pcm, frames)` | Signed 16-bit interleaved; blocks at DAC pace. |
| `morpthis_audio_write_pcm(pcm, frames)` | Any depth opened with `open_ex`; blocks at DAC pace. |
| `morpthis_audio_set_playing(bool)` | Updates the status-bar play/pause glyph; false drops queued audio. |
| `morpthis_audio_set_track(track, playing)` | Publishes the track for the control centre and status bar. |
| `morpthis_audio_close()` | Releases the output; playback stays stopped. |

`morpthis_track_t` has fixed arrays and an `int duration_ms`; its layout is
part of API 3.

The raw output is for an app that mixes its own sound or streams one. Open,
write and close on the same thread: a write blocks until the device has room,
which is what paces a mixer. There is no hardware mixing, so taking the output
stops whatever the player was playing; the app can close it and call
`morpthis_audio_play()` to hand it back.

The depth is the app's choice. `16` is interleaved S16. `24` is packed
S24_3LE, three bytes per sample. `32` is S32, left-justified: a 24-bit source
takes the top 24 bits, which is what the built-in DAC consumes as S24, so a
24-bit FLAC written as 32-bit left-justified is bit-perfect. `buffer_ms` is the
latency asked of the DAC; `0` uses the player's deep frame-based depth (about
750 ms), which is the right answer for a network stream on this device. Open
with 32 bits for anything deeper than 16; a device that cannot take S32 is
retried at 16 by the app, as local playback does.

`morpthis_audio_set_playing()` is the indicator and the power hold. While it
is true the player keeps the device awake with the screen off, exactly as it
does for a local track, and keeps Wi-Fi up because an app's output is usually a
stream; the screen still turns off on its own idle timer. Pausing releases the
hold and lets standby proceed. False also drops the PCM queue, so pause is
silent immediately rather than playing out the deep buffer (over Bluetooth the
transport's own buffer is the remaining tail). Call it when pause or resume
takes effect, from any thread; closing the output clears it.

`morpthis_audio_set_track()` is what makes an app's playback visible outside its
own screens. The player copies the title, artist, album and duration and shows
them on the control-centre card while the app holds the output; the play/pause
there and the status-bar glyph come back as `PLAY_PAUSE`, `NEXT` and `PREV`
buttons, and a tap on the card delivers `NOW_PLAYING`. Publish when a track
starts or changes, and again with `playing = false` when it stops; pass NULL to
clear it. It is safe from any thread, and safe before the output is open (the
track is remembered, but the status glyph only moves once audio exists).
Closing the output clears the published track.

API 3 adds the player's 10-band equalizer to this header: `eq_enabled()` and
`set_eq_enabled()`, `eq_band()`/`set_eq_band()` in whole dB from -12 to +12, and
`eq_reset()`. It is the same global curve Settings > Sound edits and is saved
the same way, so an app and the player's own page cannot disagree.

## fs.h

Every path is relative to the app and resolved by the player. Absolute paths,
backslashes and any path containing `..` are refused with `errno = EINVAL`.

A package is a read-only image; the app's writable folder is separate. The two
are presented as one tree:

- A read opens the file in the data folder first, then in the package.
- A write, append or read/write opens the data folder. A package file opened for
  update is copied there first, so a shipped default can be edited.
- `remove`, `rename` and `mkdir` act on the data folder. Removing or renaming a
  file that exists only in the package fails with `errno = EROFS`; `mkdir`
  creates a data directory even when the package has one of that name, so an
  app can add files to a shipped folder.

So the package never changes, and deleting the package leaves the user's data
behind.

| Call | Notes |
| --- | --- |
| `morpthis_fs_app_folder()` | The package root, read-only. |
| `morpthis_fs_data_folder()` | The writable folder, `Apps/merp_data/<id>` on the card. |
| `morpthis_fs_card_root()` | The card root. All three are absolute, owned by the player. |
| `morpthis_fs_open(path, mode)` | Modes: read, write, append, read/write. |
| `morpthis_fs_read/write` | Return bytes transferred. |
| `morpthis_fs_seek/tell` | `fseek`/`ftell` semantics. |
| `morpthis_fs_close` | Frees the handle; returns the close result. |
| `morpthis_fs_write_all` | Creates or truncates; true on a complete write. |
| `morpthis_fs_read_all` | NUL-terminates, bounded by `max`. |
| `morpthis_fs_exists/remove/rename/mkdir` | `0` on success, `-1` otherwise. |
| `morpthis_fs_resolve(path, out, size)` | The absolute path a read would open; for a library that opens files itself. |

The card may disappear at any time. Check every write; do not assume a file
stays open across a UI transition.

The API 3 absolute-path calls (`morpthis_fs_open_absolute`, `_exists_absolute`,
`_write_all_absolute`, `_read_all_absolute`, `_remove_absolute`,
`_rmdir_absolute`, `_rename_absolute`, `_mkdir_absolute`, `_stat_absolute`)
are not sandboxed:
they open exactly the path they are given, as the player's user. A directory
walk is `morpthis_fs_open_dir()`, `morpthis_fs_dir_next()` (returns a name,
skipping `.` and `..`), `morpthis_fs_dir_is_dir()` and `morpthis_fs_dir_close()`.

## playback.h

The player's own engine, not a second player. `morpthis_playback_play_file()`
and `play_folder()` build the same folder queue as tapping a file in the
browser; `play_queue()` takes an explicit list; `play_index()`, `next()`,
`prev()`, `pause()`, `resume()`, `toggle()`, `stop()` and `seek()` drive it.
`morpthis_playback_get_info()` fills `morpthis_playback_info_t` with the track,
codec, rate/depth, position/duration, queue position and play mode.
`queue_insert(path, -1)` is "add to queue"; `queue_path()` reads it back.
`morpthis_playback_set_changed_cb()` is called on the GUI thread when the
track, transport state, queue or mode changes, not while a progress bar moves.
Paths are absolute; the engine owns the queue and copies what it is given.

## library.h

A read-through view of the player's own index, with the same collation and the
same album identity. `morpthis_library_for_each[_ordered]()` streams a list into
a callback (`MORPTHIS_LIBRARY_TRACKS`, `ALBUMS`, `ARTISTS`, `ALBUM_ARTISTS`,
`GENRES`, `FAVOURITES`, `PLAYLIST`, `YEARS`, `MOST_PLAYED`), with the album /
artist / genre / year / search filter. `track_title`, `track_names`,
`track_album[_value]`, `album_title`, `album_artist` and `track_quality` answer
single lookups. Favourites, play counts, search and the playlist calls map to
the local Music and playlist pages. `scan_start()` rebuilds the index from the
card root, exactly like Settings > Scan; `scan_current()` copies the folder and
file being read. A playlist row is
streamed by `morpthis_library_playlist_rows()`, four at a time internally, so a
long list never becomes a large allocation.

## system.h

`morpthis_system_info()` gives the device name, OS/build versions, serial and
DAC; `storage()` gives the card's usage. `battery_percent()` returns -1 when it
cannot be read. `screen_is_on/on/off/toggle`, `brightness()` (percent) and
`set_brightness()` (clamped to the player's 5 % floor and persisted) are the
Screen settings. `hold_screen()` keeps the idle timer from blanking the panel.
`time()`/`set_time()` use the wall clock and RTC. `screenshot()` is the same
asynchronous capture as the volume-up + power gesture. `poweroff()` and
`reboot()` do not return. The config calls read and write the player's own
`/usr/data/device_config.ini`; the setters only touch memory until
`morpthis_config_save()`.

## wireless.h

Wi-Fi: `available/enabled/set_enabled/busy`, `status()` (state, SSID, IP, MAC,
signal), `connected()`, `scan()/scan_running()/networks_serial()/networks()`,
`connect[_hidden]()/disconnect()/forget()`, and `take_result()` plus
`last_failure()`/`last_op_kind()`. Bluetooth: `available/enabled/set_enabled`,
`state()`, `local_name()/set_local_name()`, paired and found device lists,
`connected_device()`, `audio_active()`, scan controls, pair/connect/disconnect/
forget, and `take_result()`. Every command is queued to the player's worker;
nothing blocks, and the result appears in the status calls.

## net.h

`morpthis_http_get()` fetches a whole small response;
`morpthis_http_perform()` takes a method, headers, body and content type. The
response body is malloc'd, NUL-terminated and released with
`morpthis_http_response_free()`. `morpthis_http_last_error()` gives the reason a
person can act on for the last request on the calling thread. Both calls block
on connect, TLS and every read, so they belong on an app-owned thread. The
player's CA bundle and its strict/relaxed verification setting apply, which is
what lets an app talk to HTTPS without carrying OpenSSL itself.

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
does not quit; the default is 250 ms. `MORPTHIS_APP_DATA` points the writable
folder at a temporary directory, so a test does not write into the source tree.
