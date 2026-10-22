#include "internal.h"

#include <string.h>

// The host runner has no music database. Every query is empty and every write
// is refused; the calls exist so an app builds and its non-library paths run.

bool morpthis_library_available(void) { return false; }
int morpthis_library_track_count(void) { return 0; }

bool morpthis_library_scan_start(void) { return false; }
void morpthis_library_scan_stop(void) {}
bool morpthis_library_scan_running(void) { return false; }
int morpthis_library_scan_found(void) { return 0; }

bool morpthis_library_scan_current(char *folder_out, size_t folder_size, char *file_out, size_t file_size) {
	if (folder_out && folder_size) {
		folder_out[0] = '\0';
	}
	if (file_out && file_size) {
		file_out[0] = '\0';
	}
	return false;
}

int morpthis_library_for_each(morpthis_library_list_t list, morpthis_library_filter_t filter,
							  const char *value, morpthis_library_row_cb cb, void *user) {
	(void)list;
	(void)filter;
	(void)value;
	(void)cb;
	(void)user;
	return 0;
}

int morpthis_library_for_each_ordered(morpthis_library_list_t list, morpthis_library_filter_t filter,
									  const char *value, morpthis_library_order_t order,
									  morpthis_library_row_cb cb, void *user) {
	(void)list;
	(void)filter;
	(void)value;
	(void)order;
	(void)cb;
	(void)user;
	return 0;
}

bool morpthis_library_track_title(const char *path, char *out, size_t size) {
	(void)path;
	if (out && size) {
		out[0] = '\0';
	}
	return false;
}

bool morpthis_library_track_names(const char *path, char *title, size_t title_size,
								  char *artist, size_t artist_size) {
	(void)path;
	if (title && title_size) {
		title[0] = '\0';
	}
	if (artist && artist_size) {
		artist[0] = '\0';
	}
	return false;
}

bool morpthis_library_track_album(const char *path, char *out, size_t size) {
	(void)path;
	if (out && size) {
		out[0] = '\0';
	}
	return false;
}

bool morpthis_library_track_album_value(const char *path, char *out, size_t size) {
	(void)path;
	if (out && size) {
		out[0] = '\0';
	}
	return false;
}

bool morpthis_library_album_title(const char *album_value, char *out, size_t size) {
	(void)album_value;
	if (out && size) {
		out[0] = '\0';
	}
	return false;
}

bool morpthis_library_album_artist(const char *album_value, char *out, size_t size) {
	(void)album_value;
	if (out && size) {
		out[0] = '\0';
	}
	return false;
}

morpthis_library_quality_t morpthis_library_track_quality(const char *path, int *sample_rate, int *bit_depth) {
	(void)path;
	if (sample_rate) {
		*sample_rate = 0;
	}
	if (bit_depth) {
		*bit_depth = 0;
	}
	return MORPTHIS_LIBRARY_QUALITY_NONE;
}

int morpthis_library_search(const char *query, int per_category, morpthis_library_search_cb cb, void *user) {
	(void)query;
	(void)per_category;
	(void)cb;
	(void)user;
	return 0;
}

bool morpthis_library_favourite_toggle(const char *path, const char *title, const char *artist) {
	(void)path;
	(void)title;
	(void)artist;
	return false;
}

bool morpthis_library_is_favourite(const char *path) {
	(void)path;
	return false;
}

void morpthis_library_record_play(const char *path, const char *title, const char *artist) {
	(void)path;
	(void)title;
	(void)artist;
}

int morpthis_library_play_count(const char *path) {
	(void)path;
	return 0;
}

int morpthis_library_playlists(char out[][MORPTHIS_LIBRARY_PLAYLIST_NAME_MAX], int max) {
	(void)out;
	(void)max;
	return 0;
}

bool morpthis_library_playlist_exists(const char *name) {
	(void)name;
	return false;
}

bool morpthis_library_playlist_create(const char *name) {
	(void)name;
	return false;
}

bool morpthis_library_playlist_drop(const char *name) {
	(void)name;
	return false;
}

bool morpthis_library_playlist_rename(const char *name, const char *new_name) {
	(void)name;
	(void)new_name;
	return false;
}

bool morpthis_library_playlist_append(const char *name, const char *path, const char *title,
									  const char *artist, int seconds) {
	(void)name;
	(void)path;
	(void)title;
	(void)artist;
	(void)seconds;
	return false;
}

bool morpthis_library_playlist_remove(const char *name, const char *path) {
	(void)name;
	(void)path;
	return false;
}

bool morpthis_library_playlist_contains(const char *name, const char *path) {
	(void)name;
	(void)path;
	return false;
}

bool morpthis_library_playlist_move(const char *name, int from, int to) {
	(void)name;
	(void)from;
	(void)to;
	return false;
}

int morpthis_library_playlist_count(const char *name) {
	(void)name;
	return 0;
}

int morpthis_library_playlist_rows(const char *name, int offset, int count,
								   morpthis_library_playlist_row_cb cb, void *user) {
	(void)name;
	(void)offset;
	(void)count;
	(void)cb;
	(void)user;
	return 0;
}
