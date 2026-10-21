#ifndef MORPTHIS_FS_H
#define MORPTHIS_FS_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Files on the card.
//
// Every path is relative to the app's own folder and is resolved by the player.
// Absolute paths, backslashes and any path containing ".." are refused, as in
// the player's manifest reader: an app from a removable card must not be able
// to name a file outside its folder by accident or by intent.
//
// Functions that fail set errno and return -1, 0 or NULL as documented. The card
// may disappear at any time; check the result of every write.

// The app's folder (absolute) and the card root (absolute). Both are owned by
// the player and valid for the app's lifetime.
const char *morpthis_fs_app_folder(void);
const char *morpthis_fs_card_root(void);

typedef struct morpthis_file morpthis_file_t;

typedef enum {
	MORPTHIS_FS_READ = 0,	// fails when the file does not exist
	MORPTHIS_FS_WRITE,		// creates or truncates
	MORPTHIS_FS_APPEND,		// creates or appends
	MORPTHIS_FS_READ_WRITE	// fails when the file does not exist
} morpthis_fs_mode_t;

// Opens `relative`. Returns NULL and sets errno on failure.
morpthis_file_t *morpthis_fs_open(const char *relative, morpthis_fs_mode_t mode);

// Return the number of bytes transferred. A short read or write is not an
// error; check ferror with a second call or compare with the requested size.
size_t morpthis_fs_read(morpthis_file_t *file, void *buffer, size_t size);
size_t morpthis_fs_write(morpthis_file_t *file, const void *buffer, size_t size);

// 0 on success, -1 on failure.
int morpthis_fs_seek(morpthis_file_t *file, long offset, int whence);

// -1 on failure.
long morpthis_fs_tell(morpthis_file_t *file);

// Closes and frees the handle. Always call it, even after a failed write.
int morpthis_fs_close(morpthis_file_t *file);

// Convenience calls for small files. They open, transfer and close.
// `morpthis_fs_write_all` creates or truncates.
bool morpthis_fs_write_all(const char *relative, const void *buffer, size_t size);

// Reads at most `max` bytes into a NUL-terminated buffer. Returns the number of
// bytes read, or -1. `*out_size` receives the length.
long morpthis_fs_read_all(const char *relative, char *buffer, size_t max,
						  size_t *out_size);

bool morpthis_fs_exists(const char *relative);

// 0 on success, -1 on failure.
int morpthis_fs_remove(const char *relative);
int morpthis_fs_rename(const char *from, const char *to);
int morpthis_fs_mkdir(const char *relative);

#ifdef __cplusplus
}
#endif

#endif /* MORPTHIS_FS_H */
