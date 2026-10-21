# Building

## Host

```sh
make host
make samples
make test
```

There are no host dependencies beyond a C11 compiler, `make` and `ar`. The host
runtime is a separate static library, `build/host/libmorpthis_host.a`; it
contributes the `main()` that calls the app's entry point.

`app.mk` defaults to the device build; pass `MORPTHIS_MODE=host` for the
native one. The apps repo runs `make host` and `make test`; a device package is
a separate step.

## Device

The device build needs the same toolchain the player uses:

- GCC 9.5.0, binutils 2.38, glibc 2.27 sysroot (`rockboxdev.sh` with
  `RBDEV_MORPTHIS_MINIMAL=1`).
- MIPS o32, `-march=mips32r2`, `-mno-madd4`, soft float.
- The device has glibc 2.22 and GLIBCXX 3.4.21.

Put `mipsel-rockbox-linux-gnu-gcc` in `PATH`, or set `CROSS_CC`, `CROSS_CXX`
and `CROSS_READELF`. Then:

```sh
make -C apps/calculator MORPTHIS_MODE=device
make -C apps/calculator MORPTHIS_MODE=device check-abi
```

`APP_KIND=plugin` (the default) produces a shared object named after the app.
`APP_KIND=exe` produces an ordinary executable for a headless tool; that is the
model the current player's `fork`/`execve` path can run.

### Plugins

A plugin leaves the SDK symbols undefined:

```text
$ readelf --dyn-syms calculator | grep UND
     0: 00000000     0 NOTYPE  GLOBAL DEFAULT  UND morpthis_screen_create
     ...
```

That is intentional. The player is linked with `-rdynamic` (or an explicit
version script) so those names resolve when it `dlopen`s the plugin. Only the
SDK is exported; the player's internals stay hidden.

The plugin is built with `-fPIC -fvisibility=hidden`, so only
`morpthis_app_main` is visible to `dlsym()`.

### check-abi

The toolchain links against glibc 2.27, which is newer than the device. A
binary that asks for a 2.28 symbol links cleanly and then fails on the device.
`check-abi` reads the dynamic symbol table and refuses GLIBC symbols newer than
2.22 and GLIBCXX symbols newer than 3.4.21.

The one known case is `-static-libstdc++`: the toolchain emits up to
GLIBCXX 3.4.26, and the device has 3.4.21. `app.mk` adds the static C++ runtime
for device builds. If a C++ dependency bypasses that, the ABI check catches it.

### Toolchain container

`toolchain/Dockerfile` expects a prebuilt tarball:

```sh
docker build \
  --build-arg MORPTHIS_TOOLCHAIN_URL=https://example.invalid/morpthis-toolchain.tar.xz \
  -t morpthis-sdk-toolchain toolchain/
```

`toolchain/fetch.sh` downloads and unpacks it. The SDK's `device.yml` workflow
attaches a `morpthis-toolchain.tar.xz` to a tagged release; until one exists,
build the compiler with the player's `rockboxdev.sh` or let the `morpthis-build`
workflow cache it.

For CMake projects, `cmake/morpthis-toolchain.cmake` is the equivalent of
`app.mk`'s device flags.

## Packages

A package is one uncompressed squashfs image with the app folder at its root:

```text
calculator.pkgmerp
  app.json
  icon.png
  calculator
```

The player mounts it read-only through a loop device and runs the entry from
the mount point; nothing is extracted to the card. The image is deliberately
uncompressed (`mksquashfs -noI -noD -noF -noX -comp gzip`): the kernel then serves file
data straight from the card through the page cache, with no decompression
workspace, at the cost of a larger file. On a card with room, that is the right
trade for the 64 MB player.

`make stage` in `app.mk` writes the folder to `build/device/app/<id>/`. The apps
repository's `tools/package.sh` validates the manifest, stages the build and
runs `mksquashfs`. It needs `squashfs-tools` on the build machine.

On the card:

```text
<card>/
  Apps/
    pkgmerps/
      calculator.pkgmerp
  merp_data/
    calculator/        the app's writable folder
```

The package name is the app id and carries no version: exactly one package per
app, and replacing the file is the update. The version stays in `app.json` and
the release tag. The data folder is separate so an update can replace the
package without disturbing saves, settings or downloaded content.

## CI

The SDK's `build.yml` workflow builds the host runtime and runs every sample on
every push. `device.yml` is for tags and manual runs: it checks out the player's
`rockboxdev` scripts, builds the pinned MIPS toolchain (cached by the scripts'
hash), cross-builds the samples, runs `check-abi` and uploads one `.pkgmerp` per
sample. The first run builds binutils, GCC and glibc; later runs restore the
cache.

On a tag, `device.yml` also packs the toolchain as
`morpthis-toolchain.tar.xz` (root `bin/`, `lib/`, `mipsel-rockbox-linux-gnu/`)
and attaches it to that release. That asset is what `toolchain/fetch.sh` and the
Dockerfile consume; `morpthis-build` builds its own toolchain from the same
scripts and caches it, so it does not need the tarball.
