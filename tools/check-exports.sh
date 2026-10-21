#!/bin/sh
# Checks that a player binary exports exactly the API 1 symbols and no other
# morpthis_* symbol. Run it on the stripped or unstripped player:
#
#     CROSS_NM=mipsel-rockbox-linux-gnu-nm tools/check-exports.sh morpthis_player
#
# The expected list is abi/morpthis-api-1.txt. Extra morpthis_* symbols are a
# leak of player internals; missing ones mean a plugin will fail to load.

set -eu

if [ "$#" -ne 1 ]; then
	echo "usage: $0 <player-binary>" >&2
	exit 2
fi

binary=$1
if [ ! -f "$binary" ]; then
	echo "check-exports: $binary does not exist" >&2
	exit 2
fi

NM=${CROSS_NM:-nm}
if ! command -v "$NM" >/dev/null 2>&1; then
	echo "check-exports: $NM not found; set CROSS_NM" >&2
	exit 2
fi

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
expected=$here/../abi/morpthis-api-1.txt
actual=${TMPDIR:-/tmp}/morpthis-exports.$$

trap 'rm -f "$actual"' EXIT HUP INT TERM

"$NM" -D --defined-only "$binary" 2>/dev/null \
	| awk '{ print $NF }' \
	| sed 's/@.*//' \
	| grep '^morpthis_' \
	| sort -u > "$actual" || true

missing=$(comm -23 "$expected" "$actual")
extra=$(comm -13 "$expected" "$actual")

status=0

if [ -n "$missing" ]; then
	echo "check-exports: the player does not export:" >&2
	echo "$missing" | sed 's/^/  /' >&2
	status=1
fi

if [ -n "$extra" ]; then
	echo "check-exports: the player exports symbols outside API 1:" >&2
	echo "$extra" | sed 's/^/  /' >&2
	status=1
fi

if [ "$status" -eq 0 ]; then
	echo "check-exports: API 1 matches."
fi

exit "$status"
