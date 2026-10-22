#include "internal.h"

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// The same rule the player applies to a manifest: relative to the app folder,
// no absolute path, no backslash and no "..". A path from an app is trusted no
// further than a path from the card.
static bool resolve_in(const char *base, const char *relative, char *out, size_t size) {
	if (!base || !base[0]) {
		errno = ENODEV;
		return false;
	}
	if (!relative || !relative[0] || relative[0] == '/' || strchr(relative, '\\') ||
		strstr(relative, "..")) {
		errno = EINVAL;
		return false;
	}
	int n = snprintf(out, size, "%s/%s", base, relative);
	if (n < 0 || (size_t)n >= size) {
		errno = ENAMETOOLONG;
		return false;
	}
	return true;
}

static const char *data_folder(void) {
	return g_app_data[0] ? g_app_data : g_app_folder;
}

static bool path_exists(const char *path) {
	struct stat st;
	return path && path[0] && stat(path, &st) == 0;
}

// Makes the directories a relative path needs under the data folder, for the
// copy-on-write path.
static bool make_parents(const char *relative) {
	char path[HOST_PATH_MAX];
	const char *base = data_folder();
	if (snprintf(path, sizeof(path), "%s/%s", base, relative) >= (int)sizeof(path)) {
		errno = ENAMETOOLONG;
		return false;
	}

	size_t base_length = strlen(base);
	for (char *p = path + base_length + 1; *p; p++) {
		if (*p != '/') {
			continue;
		}
		*p = '\0';
		if (mkdir(path, 0755) != 0 && errno != EEXIST) {
			return false;
		}
		*p = '/';
	}
	return true;
}

static bool copy_file(const char *from, const char *to) {
	if (strcmp(from, to) == 0) {
		return true; // the host runner may use one folder for both
	}

	char buffer[4096];
	FILE *in = fopen(from, "rb");
	if (!in) {
		return false;
	}
	FILE *out = fopen(to, "wb");
	if (!out) {
		fclose(in);
		return false;
	}

	bool ok = true;
	size_t got;
	while ((got = fread(buffer, 1, sizeof(buffer), in)) > 0) {
		if (fwrite(buffer, 1, got, out) != got) {
			ok = false;
			break;
		}
	}
	if (ferror(in)) {
		ok = false;
	}
	if (fclose(out) != 0) {
		ok = false;
	}
	fclose(in);

	if (!ok) {
		remove(to);
		if (!errno) {
			errno = EIO;
		}
	}
	return ok;
}

const char *morpthis_fs_app_folder(void) {
	return g_app_folder;
}

const char *morpthis_fs_data_folder(void) {
	return data_folder();
}

const char *morpthis_fs_card_root(void) {
	return g_card_root;
}

bool morpthis_fs_resolve(const char *relative, char *out, size_t size) {
	char path[HOST_PATH_MAX];
	if (!out || size == 0) {
		errno = EINVAL;
		return false;
	}

	if (!resolve_in(data_folder(), relative, path, sizeof(path))) {
		return false;
	}
	if (!path_exists(path)) {
		if (!resolve_in(g_app_folder, relative, path, sizeof(path))) {
			return false;
		}
		if (!path_exists(path)) {
			errno = ENOENT;
			return false;
		}
	}

	int n = snprintf(out, size, "%s", path);
	if (n < 0 || (size_t)n >= size) {
		errno = ENAMETOOLONG;
		return false;
	}
	return true;
}

morpthis_file_t *morpthis_fs_open(const char *relative, morpthis_fs_mode_t mode) {
	char data_path[HOST_PATH_MAX];
	char package_path[HOST_PATH_MAX];

	const char *data = data_folder();
	if (!resolve_in(data, relative, data_path, sizeof(data_path))) {
		return NULL;
	}

	bool in_data = path_exists(data_path);
	const char *chosen = data_path;

	switch (mode) {
	case MORPTHIS_FS_READ:
		if (!in_data) {
			if (!resolve_in(g_app_folder, relative, package_path, sizeof(package_path))) {
				return NULL;
			}
			if (!path_exists(package_path)) {
				errno = ENOENT;
				return NULL;
			}
			chosen = package_path;
		}
		break;

	case MORPTHIS_FS_WRITE:
		break;

	case MORPTHIS_FS_APPEND:
	case MORPTHIS_FS_READ_WRITE:
		if (!in_data && g_app_folder[0] &&
			resolve_in(g_app_folder, relative, package_path, sizeof(package_path)) &&
			path_exists(package_path)) {
			if (!make_parents(relative) || !copy_file(package_path, data_path)) {
				return NULL;
			}
		}
		break;

	default:
		errno = EINVAL;
		return NULL;
	}

	const char *flags;
	switch (mode) {
	case MORPTHIS_FS_READ:
		flags = "rb";
		break;
	case MORPTHIS_FS_WRITE:
		flags = "wb";
		break;
	case MORPTHIS_FS_APPEND:
		flags = "ab";
		break;
	case MORPTHIS_FS_READ_WRITE:
		flags = "r+b";
		break;
	default:
		errno = EINVAL;
		return NULL;
	}

	FILE *fp = fopen(chosen, flags);
	if (!fp) {
		return NULL;
	}
	morpthis_file_t *file = calloc(1, sizeof(*file));
	if (!file) {
		fclose(fp);
		return NULL;
	}
	file->fp = fp;
	return file;
}

size_t morpthis_fs_read(morpthis_file_t *file, void *buffer, size_t size) {
	if (!file || !file->fp || (!buffer && size > 0)) {
		errno = EINVAL;
		return 0;
	}
	return fread(buffer, 1, size, file->fp);
}

size_t morpthis_fs_write(morpthis_file_t *file, const void *buffer, size_t size) {
	if (!file || !file->fp || (!buffer && size > 0)) {
		errno = EINVAL;
		return 0;
	}
	return fwrite(buffer, 1, size, file->fp);
}

int morpthis_fs_seek(morpthis_file_t *file, long offset, int whence) {
	if (!file || !file->fp) {
		errno = EINVAL;
		return -1;
	}
	return fseek(file->fp, offset, whence);
}

long morpthis_fs_tell(morpthis_file_t *file) {
	if (!file || !file->fp) {
		errno = EINVAL;
		return -1;
	}
	return ftell(file->fp);
}

int morpthis_fs_close(morpthis_file_t *file) {
	if (!file) {
		errno = EINVAL;
		return -1;
	}
	int rc = file->fp ? fclose(file->fp) : 0;
	free(file);
	return rc;
}

bool morpthis_fs_write_all(const char *relative, const void *buffer, size_t size) {
	morpthis_file_t *file = morpthis_fs_open(relative, MORPTHIS_FS_WRITE);
	if (!file) {
		return false;
	}
	size_t written = morpthis_fs_write(file, buffer, size);
	int close_rc = morpthis_fs_close(file);
	if (written != size) {
		if (!errno) {
			errno = EIO;
		}
		return false;
	}
	return close_rc == 0;
}

long morpthis_fs_read_all(const char *relative, char *buffer, size_t max,
						  size_t *out_size) {
	if (!buffer || max == 0) {
		errno = EINVAL;
		return -1;
	}

	morpthis_file_t *file = morpthis_fs_open(relative, MORPTHIS_FS_READ);
	if (!file) {
		return -1;
	}

	size_t total = 0;
	while (total < max - 1) {
		size_t got = morpthis_fs_read(file, buffer + total, max - 1 - total);
		total += got;
		if (got == 0) {
			break;
		}
	}
	bool failed = ferror(file->fp) != 0;
	int close_rc = morpthis_fs_close(file);
	if (out_size) {
		*out_size = total;
	}
	buffer[total] = '\0';

	if (failed) {
		errno = EIO;
		return -1;
	}
	if (close_rc != 0) {
		return -1;
	}
	return (long)total;
}

bool morpthis_fs_exists(const char *relative) {
	char path[HOST_PATH_MAX];
	if (resolve_in(data_folder(), relative, path, sizeof(path)) && path_exists(path)) {
		return true;
	}
	return resolve_in(g_app_folder, relative, path, sizeof(path)) && path_exists(path);
}

int morpthis_fs_remove(const char *relative) {
	char data_path[HOST_PATH_MAX];
	char package_path[HOST_PATH_MAX];

	if (!resolve_in(data_folder(), relative, data_path, sizeof(data_path))) {
		return -1;
	}
	if (remove(data_path) == 0) {
		return 0;
	}
	if (errno != ENOENT) {
		return -1;
	}

	if (resolve_in(g_app_folder, relative, package_path, sizeof(package_path)) &&
		path_exists(package_path)) {
		errno = EROFS;
	}
	return -1;
}

int morpthis_fs_rename(const char *from, const char *to) {
	char from_path[HOST_PATH_MAX];
	char to_path[HOST_PATH_MAX];

	const char *data = data_folder();
	if (!resolve_in(data, from, from_path, sizeof(from_path)) ||
		!resolve_in(data, to, to_path, sizeof(to_path))) {
		return -1;
	}

	if (rename(from_path, to_path) == 0) {
		return 0;
	}
	if (errno != ENOENT) {
		return -1;
	}

	char package_path[HOST_PATH_MAX];
	if (resolve_in(g_app_folder, from, package_path, sizeof(package_path)) &&
		path_exists(package_path)) {
		errno = EROFS;
	}
	return -1;
}

int morpthis_fs_mkdir(const char *relative) {
	char data_path[HOST_PATH_MAX];
	if (!resolve_in(data_folder(), relative, data_path, sizeof(data_path))) {
		return -1;
	}
	if (path_exists(data_path)) {
		errno = EEXIST;
		return -1;
	}

	// Creating the data directory shadows a package directory of the same name
	// and gives the app somewhere to add files.
	return mkdir(data_path, 0755);
}

// ---------------------------------------------------------------------------
// Full access
// ---------------------------------------------------------------------------

static const char *abs_mode(morpthis_fs_mode_t mode) {
	switch (mode) {
	case MORPTHIS_FS_READ:
		return "rb";
	case MORPTHIS_FS_WRITE:
		return "wb";
	case MORPTHIS_FS_APPEND:
		return "ab";
	case MORPTHIS_FS_READ_WRITE:
		return "r+b";
	default:
		return NULL;
	}
}

morpthis_file_t *morpthis_fs_open_absolute(const char *path, morpthis_fs_mode_t mode) {
	if (!path || path[0] != '/') {
		errno = EINVAL;
		return NULL;
	}
	const char *flags = abs_mode(mode);
	if (!flags) {
		errno = EINVAL;
		return NULL;
	}
	FILE *fp = fopen(path, flags);
	if (!fp) {
		return NULL;
	}
	morpthis_file_t *file = calloc(1, sizeof(*file));
	if (!file) {
		fclose(fp);
		return NULL;
	}
	file->fp = fp;
	return file;
}

bool morpthis_fs_exists_absolute(const char *path) { return path && path[0] == '/' && path_exists(path); }

bool morpthis_fs_write_all_absolute(const char *path, const void *buffer, size_t size) {
	morpthis_file_t *file = morpthis_fs_open_absolute(path, MORPTHIS_FS_WRITE);
	if (!file) {
		return false;
	}
	size_t written = morpthis_fs_write(file, buffer, size);
	int close_rc = morpthis_fs_close(file);
	return written == size && close_rc == 0;
}

long morpthis_fs_read_all_absolute(const char *path, char *buffer, size_t max, size_t *out_size) {
	if (!buffer || max == 0) {
		errno = EINVAL;
		return -1;
	}
	morpthis_file_t *file = morpthis_fs_open_absolute(path, MORPTHIS_FS_READ);
	if (!file) {
		return -1;
	}
	size_t total = 0;
	while (total < max - 1) {
		size_t got = morpthis_fs_read(file, buffer + total, max - 1 - total);
		total += got;
		if (got == 0) {
			break;
		}
	}
	bool failed = ferror(file->fp) != 0;
	int close_rc = morpthis_fs_close(file);
	if (out_size) {
		*out_size = total;
	}
	buffer[total] = '\0';
	if (failed || close_rc != 0) {
		return -1;
	}
	return (long)total;
}

int morpthis_fs_remove_absolute(const char *path) {
	if (!path || path[0] != '/') {
		errno = EINVAL;
		return -1;
	}
	return remove(path);
}

int morpthis_fs_rmdir_absolute(const char *path) {
	if (!path || path[0] != '/') {
		errno = EINVAL;
		return -1;
	}
	return rmdir(path);
}

int morpthis_fs_rename_absolute(const char *from, const char *to) {
	if (!from || !to || from[0] != '/' || to[0] != '/') {
		errno = EINVAL;
		return -1;
	}
	return rename(from, to);
}

int morpthis_fs_mkdir_absolute(const char *path) {
	if (!path || path[0] != '/') {
		errno = EINVAL;
		return -1;
	}
	return mkdir(path, 0755);
}

bool morpthis_fs_stat_absolute(const char *path, morpthis_fs_stat_t *out) {
	if (!path || path[0] != '/' || !out) {
		errno = EINVAL;
		return false;
	}
	struct stat st;
	if (stat(path, &st) != 0) {
		return false;
	}
	out->size = (uint64_t)st.st_size;
	out->mtime = (int64_t)st.st_mtime;
	out->is_dir = S_ISDIR(st.st_mode);
	return true;
}

struct morpthis_dir {
	DIR *dir;
	char base[HOST_PATH_MAX];
	char name[256];
	bool is_dir;
};

morpthis_dir_t *morpthis_fs_open_dir(const char *absolute_path) {
	if (!absolute_path || absolute_path[0] != '/') {
		errno = EINVAL;
		return NULL;
	}
	DIR *dir = opendir(absolute_path);
	if (!dir) {
		return NULL;
	}
	morpthis_dir_t *handle = calloc(1, sizeof(*handle));
	if (!handle) {
		closedir(dir);
		return NULL;
	}
	handle->dir = dir;
	snprintf(handle->base, sizeof(handle->base), "%s", absolute_path);
	return handle;
}

const char *morpthis_fs_dir_next(morpthis_dir_t *handle) {
	if (!handle || !handle->dir) {
		return NULL;
	}
	struct dirent *entry;
	while ((entry = readdir(handle->dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			continue;
		}
		snprintf(handle->name, sizeof(handle->name), "%s", entry->d_name);
		char path[HOST_PATH_MAX + sizeof(handle->name)];
		if (snprintf(path, sizeof(path), "%s/%s", handle->base, handle->name) < (int)sizeof(path)) {
			struct stat st;
			handle->is_dir = stat(path, &st) == 0 && S_ISDIR(st.st_mode);
		} else {
			handle->is_dir = false;
		}
		return handle->name;
	}
	return NULL;
}

bool morpthis_fs_dir_is_dir(const morpthis_dir_t *handle) { return handle ? handle->is_dir : false; }

void morpthis_fs_dir_close(morpthis_dir_t *handle) {
	if (!handle) {
		return;
	}
	if (handle->dir) {
		closedir(handle->dir);
	}
	free(handle);
}
