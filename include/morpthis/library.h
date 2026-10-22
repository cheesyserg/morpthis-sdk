#ifndef MORPTHIS_LIBRARY_H
#define MORPTHIS_LIBRARY_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// The player's music library, as the local Music pages see it.
//
// The index is the player's own SQLite database on the card. These calls read
// it through the same code, with the same collation and the same notion of an
// album as the browser, so an app's list can sit beside the player's without
// disagreeing with it.
//
// Everything except the scan controls runs on the GUI thread and reads a
// snapshot. The library may be closed when no card is present; the calls then
// report empty rather than failing noisily.

#define MORPTHIS_LIBRARY_PATH_MAX 1024
#define MORPTHIS_LIBRARY_NAME_MAX 256
#define MORPTHIS_LIBRARY_PLAYLIST_NAME_MAX 128

typedef enum {
	MORPTHIS_LIBRARY_TRACKS = 0,
	MORPTHIS_LIBRARY_ALBUMS,
	MORPTHIS_LIBRARY_ARTISTS,
	MORPTHIS_LIBRARY_ALBUM_ARTISTS,
	MORPTHIS_LIBRARY_GENRES,
	MORPTHIS_LIBRARY_FAVOURITES,
	MORPTHIS_LIBRARY_PLAYLIST, // `value` is the playlist name
	MORPTHIS_LIBRARY_YEARS,
	MORPTHIS_LIBRARY_MOST_PLAYED,
} morpthis_library_list_t;

typedef enum {
	MORPTHIS_LIBRARY_FILTER_NONE = 0,
	MORPTHIS_LIBRARY_FILTER_ALBUM,
	MORPTHIS_LIBRARY_FILTER_ARTIST,
	MORPTHIS_LIBRARY_FILTER_ALBUM_ARTIST,
	MORPTHIS_LIBRARY_FILTER_GENRE,
	MORPTHIS_LIBRARY_FILTER_SEARCH,
	MORPTHIS_LIBRARY_FILTER_YEAR,
} morpthis_library_filter_t;

typedef enum {
	MORPTHIS_LIBRARY_ORDER_DEFAULT = 0,
	MORPTHIS_LIBRARY_ORDER_ALBUM,
	MORPTHIS_LIBRARY_ORDER_ADDED,
	MORPTHIS_LIBRARY_ORDER_YEAR,
	MORPTHIS_LIBRARY_ORDER_YEAR_DESC,
} morpthis_library_order_t;

typedef enum {
	MORPTHIS_LIBRARY_QUALITY_NONE = 0,
	MORPTHIS_LIBRARY_QUALITY_LOSSY,
	MORPTHIS_LIBRARY_QUALITY_CD,
	MORPTHIS_LIBRARY_QUALITY_HIFI,
	MORPTHIS_LIBRARY_QUALITY_DSD,
} morpthis_library_quality_t;

// One row of a list. `path` and `artist` are non-NULL for tracks only. Return
// false from the callback to stop early.
typedef bool (*morpthis_library_row_cb)(const char *name, const char *path, const char *artist, void *user);

bool morpthis_library_available(void);
int morpthis_library_track_count(void);

// A library scan. start() returns false when one is already running or the
// library is not open; it runs on the player's worker, so the call returns at
// once. It wipes and rebuilds the index, exactly like Settings > Scan. Progress
// is found() and scan_current().
bool morpthis_library_scan_start(void);
void morpthis_library_scan_stop(void);
bool morpthis_library_scan_running(void);
int morpthis_library_scan_found(void);
bool morpthis_library_scan_current(char *folder_out, size_t folder_size,
								   char *file_out, size_t file_size);

// Streams a list in the library's own order. Returns the rows delivered.
// An album row's `name` is an album value, not just the title: it carries the
// record's key after an 0x1f separator, so two "Greatest Hits" stay apart. Pass
// it to morpthis_library_album_title() for the text a person reads, and back as
// `value` to filter a track list. A playlist list needs the playlist name in
// `value`.
int morpthis_library_for_each(morpthis_library_list_t list, morpthis_library_filter_t filter,
							  const char *value, morpthis_library_row_cb cb, void *user);
int morpthis_library_for_each_ordered(morpthis_library_list_t list, morpthis_library_filter_t filter,
									  const char *value, morpthis_library_order_t order,
									  morpthis_library_row_cb cb, void *user);

// Track and album lookups by absolute path / album value.
bool morpthis_library_track_title(const char *path, char *out, size_t size);
bool morpthis_library_track_names(const char *path, char *title, size_t title_size,
								  char *artist, size_t artist_size);
bool morpthis_library_track_album(const char *path, char *out, size_t size);
bool morpthis_library_track_album_value(const char *path, char *out, size_t size);
bool morpthis_library_album_title(const char *album_value, char *out, size_t size);
bool morpthis_library_album_artist(const char *album_value, char *out, size_t size);

// The quality badge for one indexed track. `sample_rate` and `bit_depth` may
// be NULL; they receive what the scan stored, zero when unknown.
morpthis_library_quality_t morpthis_library_track_quality(const char *path, int *sample_rate, int *bit_depth);

typedef enum {
	MORPTHIS_LIBRARY_SEARCH_TRACK = 0,
	MORPTHIS_LIBRARY_SEARCH_ALBUM,
	MORPTHIS_LIBRARY_SEARCH_ARTIST,
} morpthis_library_search_type_t;

typedef void (*morpthis_library_search_cb)(morpthis_library_search_type_t type, const char *name,
										   const char *path, void *user);

// Matches anywhere in the name, case- and accent-insensitively, as the
// player's search does. `per_category` caps each kind of hit (0 means the
// player's default). Returns the number of hits delivered.
int morpthis_library_search(const char *query, int per_category, morpthis_library_search_cb cb, void *user);

// Favourites. `title` and `artist` may be NULL; they are stored for the list.
bool morpthis_library_favourite_toggle(const char *path, const char *title, const char *artist);
bool morpthis_library_is_favourite(const char *path);

// Play counts.
void morpthis_library_record_play(const char *path, const char *title, const char *artist);
int morpthis_library_play_count(const char *path);

// Playlists. Names are the user's, not file names.
int morpthis_library_playlists(char out[][MORPTHIS_LIBRARY_PLAYLIST_NAME_MAX], int max);
bool morpthis_library_playlist_exists(const char *name);
bool morpthis_library_playlist_create(const char *name);
bool morpthis_library_playlist_drop(const char *name);
bool morpthis_library_playlist_rename(const char *name, const char *new_name);
bool morpthis_library_playlist_append(const char *name, const char *path, const char *title,
									  const char *artist, int seconds);
bool morpthis_library_playlist_remove(const char *name, const char *path);
bool morpthis_library_playlist_contains(const char *name, const char *path);
bool morpthis_library_playlist_move(const char *name, int from, int to);
int morpthis_library_playlist_count(const char *name);

// Streams `count` rows from `offset`, as the playlist page shows them.
// `seconds` is -1 when unknown; `present` says the file is on the card.
// Returns the rows delivered.
typedef void (*morpthis_library_playlist_row_cb)(const char *path, const char *title, const char *artist,
												 int seconds, bool present, void *user);
int morpthis_library_playlist_rows(const char *name, int offset, int count,
								   morpthis_library_playlist_row_cb cb, void *user);

#ifdef __cplusplus
}
#endif

#endif /* MORPTHIS_LIBRARY_H */
