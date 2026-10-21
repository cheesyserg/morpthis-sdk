#!/bin/sh
# Fetches the prebuilt Morpthis cross toolchain.
#
#   MORPTHIS_TOOLCHAIN_URL=https://.../morpthis-toolchain.tar.xz \
#       toolchain/fetch.sh [destination]
#
# The tarball must contain bin/, lib/ and mipsel-rockbox-linux-gnu/sysroot/ at
# its root. Set MORPTHIS_TOOLCHAIN_SHA256 to verify it, and
# MORPTHIS_TOOLCHAIN_STRIP to drop leading path components.
#
# The toolchain itself is built by the player's rockboxdev.sh with
# RBDEV_MORPTHIS_MINIMAL=1. A published tarball should be attached to a tagged
# SDK release so app authors do not have to build binutils, GCC and glibc.

set -eu

dest=${1:-/opt/morpthis-toolchain}
url=${MORPTHIS_TOOLCHAIN_URL:-}

if [ -z "$url" ]; then
	cat >&2 <<'EOF'
fetch.sh: no toolchain URL.

No prebuilt Morpthis toolchain is published yet. Either:

  * set MORPTHIS_TOOLCHAIN_URL to a tarball and run this script, or
  * build one with the player's rockboxdev/rockboxdev.sh:
        RBDEV_MORPTHIS_MINIMAL=1 ./rockboxdev.sh
    and put its bin/ in PATH (then app.mk needs no fetch at all), or
  * use the host runtime for development: make host test

EOF
	exit 2
fi

archive=${TMPDIR:-/tmp}/morpthis-toolchain.$$
downloaded=0

case "$url" in
	file://*) archive=${url#file://} ;;
	*://*) downloaded=1 ;;
	*) archive=$url ;;
esac

cleanup() {
	if [ "$downloaded" = 1 ] && [ -f "$archive" ]; then
		rm -f "$archive"
	fi
}
trap cleanup EXIT HUP INT TERM

if [ "$downloaded" = 1 ]; then
	echo "fetch.sh: downloading $url"
	if command -v curl >/dev/null 2>&1; then
		curl -fL --retry 3 -o "$archive" "$url"
	elif command -v wget >/dev/null 2>&1; then
		wget -O "$archive" "$url"
	else
		echo "fetch.sh: neither curl nor wget is installed" >&2
		exit 2
	fi
fi

if [ -n "${MORPTHIS_TOOLCHAIN_SHA256:-}" ]; then
	echo "fetch.sh: verifying sha256"
	echo "$MORPTHIS_TOOLCHAIN_SHA256  $archive" | sha256sum -c -
fi

mkdir -p "$dest"
echo "fetch.sh: unpacking into $dest"
tar -xf "$archive" -C "$dest" --strip-components="${MORPTHIS_TOOLCHAIN_STRIP:-0}"

if [ ! -x "$dest/bin/mipsel-rockbox-linux-gnu-gcc" ]; then
	echo "fetch.sh: $dest/bin/mipsel-rockbox-linux-gnu-gcc is missing" >&2
	echo "fetch.sh: check the tarball layout or MORPTHIS_TOOLCHAIN_STRIP" >&2
	exit 1
fi

echo "fetch.sh: toolchain ready at $dest"
