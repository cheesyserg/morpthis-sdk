# morpthis-sdk

The contract between the Morpthis player and apps built outside the player's
repository.

The player owns the display, the touch panel, the audio device and the DAC. An
app is a shared object the player loads into its own process and calls on the
GUI thread; it reaches the device only through this SDK. The public headers are
C, versioned and free of LVGL types.

**Status.** API 3. The headers, the build template, the host runtime and two
samples are here and tested. The player has the in-process loader and the
LVGL/input/audio/fs bridges in its `src/gui/apps/`; API 3 adds the player's own
playback queue, the music library, playlists and favourites, the HTTP/TLS
client, Wi-Fi and Bluetooth, device status and controls, absolute-path file
access, theme/dialog/text-entry UI, and app background state. The pinned device
toolchain is still the missing piece for device CI. See
[docs/player-integration.md](docs/player-integration.md).

## Quick start

```sh
make host          # build the host runtime
make samples       # build hello and calculator for the host
make test          # build and smoke-test both
```

The host runtime is a headless implementation of the API: widgets are recorded,
input and timers work, files are real, and audio calls succeed without touching
a device. It is enough to develop UI logic and run CI. Run a sample directly:

```sh
MORPTHIS_APP_FOLDER=$PWD/samples/calculator \
MORPTHIS_APP_ID=calculator \
MORPTHIS_HOST_SCRIPT='press:back' \
samples/calculator/build/host/calculator
```

## Layout

```text
include/morpthis/      app, ui, input, audio, playback, library, fs, system,
                       wireless, net, log, version
src/host/              headless host runtime (not shipped on the device)
samples/               hello, calculator
app.mk                 the build template an app's Makefile includes
abi/                   the API 3 symbol list the player must export
tools/                 check-exports.sh
cmake/                 toolchain file for CMake projects
toolchain/             Dockerfile and fetch.sh for the pinned compiler
docs/                  API, build and player-integration notes
```

## An app

`apps/myapp/Makefile`:

```make
APP_ID := myapp
APP_SRCS := src/main.c
MORPTHIS_SDK ?= $(abspath ../../../morpthis-sdk)
include $(MORPTHIS_SDK)/app.mk
```

`apps/myapp/src/main.c`:

```c
#include <morpthis/morpthis.h>

int morpthis_app_main(const morpthis_app_t *app) {
    morpthis_logi("myapp", "started, api %d", morpthis_app_api_version(app));

    morpthis_screen_t *screen = morpthis_screen_create("My app");
    morpthis_widget_t *label = morpthis_label_create(screen, "Hello");
    morpthis_widget_set_pos(label, 20, 20);
    morpthis_screen_load(screen);
    return 0;
}
```

The entry point sets up the UI and returns; it must not block the GUI thread.
Long work goes on an app-owned thread and comes back through
`morpthis_app_post()`.

## Building

`app.mk` has two modes:

| Mode | Output | Use |
| --- | --- | --- |
| `MORPTHIS_MODE=device` (default) | MIPS shared object (`APP_KIND=plugin`) or executable (`exe`) | the card |
| `MORPTHIS_MODE=host` | native binary linked with the host runtime | development and CI |

The device target is built with the pinned toolchain: GCC 9.5.0, binutils 2.38,
glibc 2.27 sysroot, `-march=mips32r2 -mno-madd4`, and the device's glibc 2.22 /
GLIBCXX 3.4.21 limits. `make check-abi` refuses a binary that asks for a newer
symbol. C++ apps link `-static-libstdc++ -static-libgcc` automatically.

See [docs/building.md](docs/building.md).

## Rules the API is built around

- **No blocking the GUI thread.** The entry point returns; work is posted back.
- **No allocation in the draw path.** The player draws; the app builds widgets.
- **No LVGL, FreeType or ALSA in the public headers.** A wrapper can change.
- **Opaque handles.** No struct layout crosses the boundary except the value
  structs documented in the headers (`morpthis_track_t`, `morpthis_playback_info_t`
  and the status records), whose sizes are part of API 3.
- **No exceptions across the boundary.** C++ apps catch their own.
- **A memory budget.** `morpthis_app_mem_available()` reports what the player
  can see; a utility should fit in 4 MB, a game in 8 MB. The manifest's `tier`
  is `light` (leave playback and caches alone) or `heavy` (release them first).
- **The package is read-only; the data folder is the app's.** `fs.h` resolves
  every relative path against `Apps/merp_data/<id>` first and the mounted package
  second, and sends every write to the data folder. Absolute paths, backslashes
  and `..` are refused by that half of the API.
- **Full access is explicit.** API 3's `morpthis_fs_*_absolute` calls and the
  directory walk reach anywhere the player can, as the player's user. They are
  for the user's files, not the firmware's.
- **Networking, Bluetooth and the DAC are available.** API 3 exposes the
  player's own HTTP/TLS client and its radio workers, and the existing raw PCM
  output still takes the DAC. The GUI thread must not be blocked by any of them.

## Documentation

- [API3-CHECKLIST.md](API3-CHECKLIST.md) — the complete API 3 change list and open validation items.
- [docs/api.md](docs/api.md) — every call and its semantics.
- [docs/building.md](docs/building.md) — host and device builds, ABI, packages.
- [docs/player-integration.md](docs/player-integration.md) — what the player
  must export and how it loads an app.
