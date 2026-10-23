# Morpthis API 3 — implementation checklist

Status: **SDK, player and host runtime complete.** The open boxes are app
migration and on-device validation, not missing API work.

Legend: `[x]` done, `[ ]` still to do.

---

## 1. Versioning and ABI

- [x] `MORPTHIS_APP_API_VERSION` = **3** (`include/morpthis/version.h`)
- [x] SDK version = **1.2.2**
- [x] API 2 symbols retained; API 2 apps still load on an API 3 player
- [x] `abi/morpthis-api-3.txt` — **301** symbols, sorted and unique
- [x] `tools/check-exports.sh` uses the API 3 list
- [x] SDK CI contract job checks the API 3 list
- [x] Player version script (`morpthis.ld`) still exports `morpthis_*`
- [x] Player's `apphost_launch()` refuses an app whose API is newer than 3

## 2. New public headers

- [x] `playback.h` — the player's own queue and engine
  - [x] play file / folder / explicit queue
  - [x] play queue index, resume, pause, toggle, stop, next, prev, seek
  - [x] `get_info()`: track, codec, rate/channels/depth, bitrate, DSD, lossy/live, position/duration, queue index/count, mode, state
  - [x] repeat/shuffle modes, play speed
  - [x] queue count/index/path, insert (including "after current"), clear
  - [x] cover path
  - [x] changed callback (track/state/queue/mode, not position)
- [x] `library.h` — the player's own media index
  - [x] available, track count
  - [x] scan start/stop/running/found/current folder+file
  - [x] stream tracks/albums/artists/album artists/genres/favourites/playlist/years/most played
  - [x] filters (album/artist/album artist/genre/search/year) and orders (default/album/added/year/year desc)
  - [x] track title/names/album/album value; album title/artist
  - [x] quality badge + sample rate/depth
  - [x] search across tracks/albums/artists
  - [x] favourite toggle/query, play count, record play
  - [x] playlist list/exists/create/drop/rename/append/remove/contains/move/count/page
- [x] `system.h` — device and settings
  - [x] device name, OS/build version, serial, DAC
  - [x] card present/total/used/free/percent
  - [x] battery percent, charging, USB VBUS, mass-storage active
  - [x] screen on/off/toggle, brightness get/set (5 % floor), hold screen
  - [x] wall clock get/set (RTC written)
  - [x] screenshot request
  - [x] power off / reboot
  - [x] config get string/int/bool, set, save
- [x] `power.h` — app power management
  - [x] independent AWAKE / SCREEN / NETWORK hold bits
  - [x] app SCREEN hold has its own flag, separate from player page/view holds
  - [x] holds survive while the app is backgrounded
  - [x] replace, release-all and query the app's hold set
  - [x] timed holds (`set_holds_for`, timeout 0 = indefinite)
  - [x] player-enforced expiry on the power tick
  - [x] single-bit `hold_screen()`/`hold_awake()` changes do not cancel a timed set
  - [x] suspend/resume event callback, GUI thread, cleared at teardown
  - [x] RESUME is paired with the callback that received SUSPEND
  - [x] screen-on/screen-off events at the end of the panel transition
  - [x] teardown clears holds, deadline and callback
  - [x] NETWORK implies AWAKE and keeps Wi-Fi from parking
  - [x] no hold is inferred from opening or writing the DAC
- [x] `wireless.h` — both radios
  - [x] Wi-Fi available/enabled/set/busy, status, connected, scan + serial + network list
  - [x] Wi-Fi connect/hidden/disconnect/forget, take result, last failure/op kind
  - [x] Bluetooth available/enabled/set/busy/state, local name get/set
  - [x] Bluetooth paired/found/connected device, audio-active
  - [x] Bluetooth scan start/stop/running/refresh, pair/connect/disconnect/forget, take result
- [x] `net.h` — the player's HTTP/TLS client
  - [x] GET with headers, error-body option, body limit, timeout
  - [x] general method/body/content-type request
  - [x] response body/status allocation + free
  - [x] per-thread last error
  - [x] URL encoding

## 3. Extended public headers

- [x] `fs.h` — full access
  - [x] absolute open/exists/write-all/read-all/remove/rmdir/rename/mkdir/stat
  - [x] directory handle: open/next/is-dir/close
  - [x] existing package/data sandbox untouched
- [x] `ui.h` — complete widget and chrome surface (see section 4)
- [x] `audio.h` — 10-band equalizer: enabled get/set, band get/set, reset
- [x] `app.h` — `morpthis_app_is_background()`
- [x] `morpthis.h` — includes all new headers

## 4. UI element coverage

### Screens and navigation

- [x] regular screen with title row
- [x] full-height screen without title row
- [x] game screen (whole panel, no player chrome)
- [x] title set/update
- [x] corner action buttons in the title row
- [x] player-drawn background/put-away button
- [x] chevron + edge-swipe BACK kept, app `back_handled()` / `background()` step

### Basic drawing and content

- [x] label
- [x] text button
- [x] icon button (player glyphs)
- [x] image from package/data, rotation, opacity
- [x] canvas with RGB565 pixels + rectangle invalidation + `ui_refresh()`
- [x] QR code, re-encodable
- [x] list with plain rows, two-line rows, leading icon, chevron, playing mark
- [x] list scroll-to-row

### Controls

- [x] slider (value/release/range)
- [x] switch (on/off, programmatic set)
- [x] progress bar (min/max/value)
- [x] spinner
- [x] segmented pills (labels, selected, callback)
- [x] roller/wheel (options, selected, callback)
- [x] arc gauge (range, value, callback)

### Layout and containers

- [x] transparent container
- [x] row/column flex layout, main/cross alignment, gap
- [x] scrollable container (vertical)
- [x] reparent a widget into a container (`morpthis_widget_set_parent`)
- [x] pixel or content sizing (`MORPTHIS_SIZE_CONTENT`)
- [x] padding override
- [x] radius/background/text-colour overrides

### Text

- [x] player text sizes 14–32
- [x] primary/dim colour
- [x] left/center/right alignment
- [x] wrapped labels
- [x] marquee scrolling labels (player's scrolltext)
- [x] inline text field that opens the player keyboard
- [x] modal text prompt with accept/cancel, password mode

### Player chrome and overlays

- [x] popover menu anchored to a widget
- [x] popover menu anchored to a list row
- [x] toast plain / success / error / busy + busy end
- [x] confirm (yes/no) and notice (one button)
- [x] full-screen modal + optional progress bar
- [x] status bar show/hide/query
- [x] theme palette read (RGB), Dark/Light switch, accent switch
- [x] player icon set: transport, chevrons, library, media, actions, radios,
      battery, audio, power (46 glyphs: 9 original + 37 new)

### Timers and composition notes

- [x] repeating GUI-thread timer create/period/reset/delete, auto-deleted on unload
- [x] settings row (title, detail, value, chevron, tap)
- [x] settings rows with switch/slider: composable from
      container + label + switch/slider (no single combined call)
- [x] duration picker: composable from two rollers
- [x] chart/custom graph drawing: canvas is the general surface
- [x] AZ index / cover flow / main-menu grid: layout compositions, not
      dedicated widgets; the primitives above cover them

## 5. Player implementation

- [x] `sdk_playback.c` — queue, transport, metadata, cover, change timer
- [x] `sdk_library.c` — lists, search, favourites, playlists, scans
- [x] `sdk_system.c` — info/storage/battery/screen/clock/USB/config,
      locked power-hold mask, separate app screen hold, timed expiry,
      suspend/resume/screen callbacks and teardown cleanup
- [x] `sdk_audio.c` — suspends non-playing app PCM, parks mixer writers and
      reopens them after the wake
- [x] `apphost.c` — awake/network hold getters, power tick, GUI-thread suspend,
      resume and screen-state notifications
- [x] `sdk_wireless.c` — Wi-Fi and Bluetooth wrappers
- [x] `sdk_net.c` — HTTP/TLS response lifetime and error text
- [x] `sdk_fs.c` — absolute-path calls and directory walk
- [x] `sdk_audio.c` — equalizer calls
- [x] `sdk_app.c` — background state, chrome cleanup on background
- [x] `sdk_ui.c` — switch/progress/spinner/arc/container/segmented/settings
      row/text input/QR/roller/timer/toast/modal/style/text-layout
- [x] `apphost.c` — callbacks/chrome/timers/holds cleaned up before `dlclose`
- [x] `keyboard.c/.h` — `keyboard_destroy()` (needed by repeatable modals)
- [x] `power.c/.h` — extracted `power_reboot()` shared with the power menu;
      separate app screen hold; screen on/off notifications

## 6. Host runtime

- [x] `src/host/playback.c` — in-memory queue and state
- [x] `src/host/library.c` — empty/false answers
- [x] `src/host/system.c` — benign status, no-op controls, config fallbacks
- [x] `src/host/power.c` — hold-set storage and no-op suspend events
- [x] `src/host/wireless.c` — absent radios
- [x] `src/host/net.c` — clean failure + URL encoding
- [x] `src/host/fs.c` — absolute calls and directory walk
- [x] `src/host/ui.c` — every new widget/chrome call
- [x] `src/host/timer.c` — real timers serviced by the runner's sleep/loop
- [x] `src/host/audio.c` — equalizer state

## 7. Documentation and CI

- [x] `README.md` updated to API 3
- [x] `docs/api.md` documents every new module, the full widget set and the
      power holds/events
- [x] `docs/player-integration.md` points at `abi/morpthis-api-3.txt` and
      describes hold ownership, deadlines, the suspend handoff, the separate
      screen hold and screen events
- [x] player `docs/app-sdk.md` describes API 3 and `min_player: 3`
- [x] app authoring notes mention API 3 requirements
- [x] CI symbol-list check updated
- [x] `check-exports.sh` wording and path updated

## 8. Validation already run

- [x] `make test` in `morpthis-sdk` (host runtime + samples)
- [x] `make -C tests test` in the player
- [x] Subsonic host `test` and `test-integration` (HTTP and HTTPS)
- [x] `gcc -fsyntax-only -Wall` over the changed app-host, SDK bridge,
      audio and USB sources (LVGL stub; `power.c` reviewed by hand because the
      submodule has no LVGL source here)
- [x] host library exports match `abi/morpthis-api-3.txt` exactly (301 symbols)
- [x] every API 3 symbol has a player definition
- [x] `abi/morpthis-api-3.txt` sorted and unique
- [x] host timer smoke test (period, fire, self-delete)
- [x] `git diff --check` clean (CRLF notices only)

## 9. Device validation (open)

- [ ] Build the player with the pinned MIPS toolchain
- [ ] Run `tools/check-exports.sh` against the stripped player binary
- [ ] Flash the player firmware
- [ ] Smoke-test: launch an app, local playback from the SDK, library list,
      seek, EQ, Wi-Fi/Bluetooth status, HTTP request, absolute fs read/write,
      switch/progress/spinner/container/settings row/text prompt/QR/roller/arc
- [ ] Hold AWAKE from a test app; verify standby is skipped and the log names
      the hold, while a game with a silent mixer still suspends
- [ ] Hold NETWORK with no audio; verify Wi-Fi stays associated and the device
      does not suspend
- [ ] Hold SCREEN while a player page also holds the screen, then close each
      in turn; verify neither release drops the other hold
- [ ] Set a timed hold and verify it expires within one 250 ms power tick
- [ ] Verify SUSPENDING/RESUMED and SCREEN_OFF/SCREEN_ON arrive on the GUI
      thread in that order, including after a failed suspend attempt
- [ ] Pull the SD card mid-playback; verify the player recovers cleanly
- [ ] Measure free RAM with a representative app using the new widgets

## 10. App migration (open)

- [x] Rebuilt Subsonic against SDK 1.2, set `min_player: 3`, and moved its
      settings page and search to the API 3 settings rows and text prompt
- [ ] Move Subsonic's HTTP calls to `net.h` if a single TLS stack is wanted
- [ ] Use `playback.h` only where the app wants the player's queue; it can keep
      its own raw PCM output for server-side transcoding
- [ ] Add one API 3 sample app (a settings/now-playing screen) to exercise the
      new widgets in CI
- [ ] Update the build repo's pinned SDK ref when this lands

## 11. Deliberate boundaries

These are not missing wrappers; they are player-owned pages/behaviour with no
sensible app-level equivalent:

- LVGL's `lv_chart`, `lv_spangroup` and custom table layouts are replaced by
  canvas drawing and the container/text primitives.
- The player's own pages (AirPlay, DLNA, SonixLink, Wi-Fi transfer) are not
  injectable; apps reach the same network through `net.h` and the same audio
  device through `audio.h`/`playback.h`.
- LVGL types and player internal headers are still not exposed across the ABI;
  the SDK wrappers are the contract. This keeps apps working across LVGL and
  player upgrades without recompiling against internal structs.
