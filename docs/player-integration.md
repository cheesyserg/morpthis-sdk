# Integrating the SDK into the player

This is the other half of the contract: what the player must implement, export
and do around an app. The player now has this in `src/gui/apps/apphost.c`,
`src/gui/apps/sdk_*.c` and `src/system/apps/apps_reclaim.c`; this document stays
the checklist and the rationale. The existing manifest reader and the
fork/execve fallback (`src/system/apps/`) remain for headless tools.

## 1. Include the headers

Add this repository as a submodule, a sibling checkout or a released package
and point the player's include path at `include/`. The player needs the
declarations of the functions it implements; it does not link a library. The
player's Makefile takes `MORPTHIS_SDK` (default `../../morpthis-sdk`) and adds
`$(MORPTHIS_SDK)/include` to both host and target builds.

The player is one of the two implementations of the API. The host runtime in
`src/host/` is the other, and is a useful reference, but it is not shipped.

## 2. Export the API

The player must be linked with `-rdynamic` (or an explicit dynamic list) so a
plugin's undefined `morpthis_*` symbols resolve when it is loaded. Only the API
may be exported. The player uses `src/system/apps/morpthis.ld`, a version
script with `global: morpthis_*; local: *;`, and links with `-rdynamic` on both
the host and target builds.

`abi/morpthis-api-1.txt` is the complete list. `tools/check-exports.sh` compares
a player binary against it:

```sh
CROSS_NM=mipsel-rockbox-linux-gnu-nm tools/check-exports.sh morpthis_player
```

Run it in the player's CI after `check-abi`: an extra `morpthis_*` symbol is a
leak, and a missing one is a plugin that will not load.

`morpthis_app_main` and `morpthis_app_api_required` are not in the list: the app
defines them, and the player resolves them with `dlsym()`.

## 3. Loading

The manifest's `entry` names a file relative to the app folder. Try the
in-process path first:

```c
char path[PATH_MAX];
snprintf(path, sizeof(path), "%s/%s", app->folder, app->entry);

void *handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
if (!handle) {
    // Not a plugin (an executable tool): keep the current fork/exec path.
    return apps_run(relative, timeout_ms, out);
}

int (*required)(void) = (int (*)(void))dlsym(handle, "morpthis_app_api_required");
int (*entry)(const morpthis_app_t *) =
    (int (*)(const morpthis_app_t *))dlsym(handle, "morpthis_app_main");
if (!entry) {
    dlclose(handle);
    return apps_run(relative, timeout_ms, out);
}

int app_api = required ? required() : 1; // API 1 had no symbol
if (app_api > MORPTHIS_APP_API_VERSION) {
    snprintf(out->error, sizeof(out->error), "app needs a newer player");
    dlclose(handle);
    return false;
}
```

`RTLD_NOW` is deliberate: a plugin that asks for a function this player does not
export should fail at load with a clean error, not halfway through its first
callback. `RTLD_LOCAL` keeps one app's symbols out of the next.

If `dlopen` fails, fall back to `execve`; that is still the way a headless tool
runs. Log the `dlerror()` text so a plugin with a missing symbol is not silently
treated as a tool. A file that has an entry point but fails the API check is a
plugin failure, not a tool: report it.

## 4. The app context

`morpthis_app_t` is opaque to the app and owned by the player. Fill it from the
manifest (id, name, version, tier) and the resolved absolute folder. Keep it
alive for the whole launch. The real definition is in
`src/gui/apps/sdk_internal.h`:

```c
typedef struct morpthis_app {
    char id[APPS_ID_MAX];
    char name[APPS_NAME_MAX];
    char version[APPS_VERSION_MAX];
    char folder[APPS_FOLDER_MAX];
    app_tier_t tier;

    void *dl_handle;
    bool alive;
    bool quitting;
    morpthis_screen_t *screens;
    morpthis_screen_t *active;
    // input subscriptions and posted callbacks...
} morpthis_app_t;
```

`morpthis_app_id()` and the rest return pointers into this struct. The folder is
the one the current launcher already computes and passes as the child's working
directory; it must be absolute.

## 5. Lifecycle

The order matters:

1. For a `heavy` app, reclaim first (`apps_reclaim_for_heavy()`); a `light` app
   leaves playback and caches alone.
2. Fill the app context; open the card paths the app may use.
3. Call `entry(app)` on the GUI thread.
4. If it returns non-zero, tear down and report the failure.
5. Return to the event loop. The app's screen is active; widget callbacks and
   posted functions run on the GUI thread.

On `morpthis_app_quit()`:

1. Set `quitting`; a `gui_post()` queues the teardown so it never deletes the
   screen from inside its own button callback or the swipe animation.
2. Switch back to the Apps page, then delete the app's screens and widgets.
3. Detach every posted callback that belongs to this app (`sdk_detach_app()`),
   so a queued trampoline cannot enter the shared object after `dlclose()`.
4. Close any files the SDK opened on the app's behalf.
5. `dlclose()` the handle. An app must stop its own threads before it quits;
   a thread still inside the player after `dlclose()` is a use-after-free the
   player cannot detect. Keep that rule in the app-writing docs.

There is no process boundary: a fault in the app takes the player down. Say so
in the Apps page and the docs. That is the accepted cost of fitting in the 18 MB
headroom.

## 6. UI bridge

- `morpthis_screen_create()` creates a new LVGL screen, not a child of the
  player's current screen.
- `morpthis_screen_load()` calls `lv_screen_load()` and remembers the previous
  screen so quit can restore the Apps page.
- Widgets are thin wrappers over the player's existing LVGL objects: a label, a
  button, a list, an image and a slider. The app never sees `lv_obj_t`.
- Use the player's theme and fonts. Do not read `lv_conf.h` in the app.
- `morpthis_toast()` uses the player's existing toast path.
- The display size comes from the panel profile; report it from
  `morpthis_ui_width()/height()`.

All UI calls arrive on the GUI thread. Do not add locking around LVGL; assert
`lv_is_initialized()` and, in debug builds, that the caller is the GUI thread.

## 7. Input bridge

- Register one button callback per app. Map the physical transport buttons to
  `MORPTHIS_BUTTON_*`; map the player's back affordance to
  `MORPTHIS_BUTTON_BACK`.
- The player currently delivers `PRESS` for the transport keys (the kernel's
  repeat becomes another press); `BACK` is delivered as `PRESS` then `RELEASE`
  by the chevron and swipe, followed by a quit. The SDK allows `REPEAT` and
  `RELEASE`; if the player gains release events, it can start sending them
  without an API change.
- Volume keys stay the player's; an app reads or sets the volume through the
  audio API instead of owning the rocker.
- The touch callback is for apps that draw their own surface. Widgets receive
  taps through LVGL; do not send the same tap to both.
- Clear the callbacks on quit. A callback into an unloaded plugin is a crash.

## 8. Audio bridge

- `morpthis_audio_current()` fills `morpthis_track_t` from the now-playing
  track; zero it and return false when nothing is loaded.
- `morpthis_audio_play/pause/next/prev` call the player's own transport, not a
  second decoder.
- `morpthis_audio_volume()` and `set_volume()` are the same volume the hardware
  buttons change, 0..100.
- Launching a light app must not pause or stop playback by itself. That is the
  "leave the player's audio alone" rule in `app-sdk.md`. A heavy app stops it by
  declaring its tier; the player does the stop, not the app.

## 9. Filesystem bridge

- Resolve every relative path against the app's folder. Apply the same
  `safe_relative()` rule the manifest reader uses: no leading `/`, no `\`, no
  `..` anywhere. Reject with `EINVAL`.
- Use `-D_FILE_OFFSET_BITS=64`; exFAT cards routinely produce inodes wider than
  32 bits, and `readdir()`/`stat()` fail with `EOVERFLOW` without it.
- Open with the device's libc, close on quit, and propagate `errno`. A card can
  disappear; a write must be able to fail.
- Do not expose a `/proc` or `/sys` path through the app folder. The app runs as
  root, but the SDK's contract is that it stays in its folder.

## 10. Memory

- `morpthis_app_mem_available()` returns `MemAvailable` from `/proc/meminfo`,
  or `SIZE_MAX` if it cannot be read.
- A `light` app runs with the player's caches warm. A `heavy` app declares
  `tier: heavy`; before `dlopen()` the player calls
  `apps_reclaim_for_heavy()`, which:
  - stops playback and waits for the decoder, its `FILE*` and the PCM to close
    (`audio_release_all()`), keeping the queue;
  - releases SQLite's page caches (`library_release_memory()`,
    `audiobookdb_release_memory()`, `cover_cache_release_memory()`);
  - drops the decoded artwork and list thumbnails
    (`player_release_artwork()`, `coverloader_release_all()`) and asks for the
    current track's picture again when the app exits;
  - calls `malloc_trim(0)` and logs `MemAvailable` before and after.
  This is best-effort: static player pages cannot be freed and the reclaim is
  not a per-app quota.
- The SDK's budget is guidance, not enforcement: a utility should fit in 4 MB,
  a game in 8 MB.
- Do not allocate in the draw path. The host runtime is deliberately dumb here;
  the player must keep LVGL's draw path allocation-free as it does today.

## 11. Failure reporting

Reuse the table in `docs/apps.md` and add the plugin cases:

| What happened | What the page shows |
| --- | --- |
| `dlopen` failed | Falls back to the tool path |
| `morpthis_app_main` missing | Falls back to the tool path |
| App requires API 2, player has 1 | "needs a newer player" |
| App's `morpthis_app_main` returns non-zero | `name: entry returned N` |
| App returns without a screen | `name: did not load a screen` |
| API symbol missing at `dlopen` | Falls back to `execve`, which reports `ENOEXEC`; the `dlerror()` line says why |

Log the same lines under `apps:` so a user can collect them with
`docs/collecting-logs.md`.
