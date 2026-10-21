#include "internal.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static char level_char(morpthis_log_level_t level) {
	switch (level) {
	case MORPTHIS_LOG_DEBUG:
		return 'D';
	case MORPTHIS_LOG_INFO:
		return 'I';
	case MORPTHIS_LOG_WARN:
		return 'W';
	case MORPTHIS_LOG_ERROR:
		return 'E';
	default:
		return '?';
	}
}

void morpthis_log(morpthis_log_level_t level, const char *tag, const char *format, ...) {
	char message[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(message, sizeof(message), format, args);
	va_end(args);

	const char *id = morpthis_app_id(host_app());
	if (tag && tag[0]) {
		fprintf(stderr, "app:%s: [%c] %s: %s\n", id, level_char(level), tag, message);
	} else {
		fprintf(stderr, "app:%s: [%c] %s\n", id, level_char(level), message);
	}
	fflush(stderr);
}
