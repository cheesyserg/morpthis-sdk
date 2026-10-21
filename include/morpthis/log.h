#ifndef MORPTHIS_LOG_H
#define MORPTHIS_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

// One line to the player's log.
//
// Lines are prefixed with the app id, so they can be read next to the player's
// own output. Keep it to a start line and an exit line, plus warnings; the
// device's log buffer is shared with everything else the player is doing.

#if defined(__GNUC__) || defined(__clang__)
#define MORPTHIS_PRINTF(format_index, first_arg) \
	__attribute__((format(printf, format_index, first_arg)))
#else
#define MORPTHIS_PRINTF(format_index, first_arg)
#endif

typedef enum {
	MORPTHIS_LOG_DEBUG = 0,
	MORPTHIS_LOG_INFO,
	MORPTHIS_LOG_WARN,
	MORPTHIS_LOG_ERROR
} morpthis_log_level_t;

// `tag` is a short subsystem name ("calc", "fs"); it may be NULL.
void morpthis_log(morpthis_log_level_t level, const char *tag, const char *format, ...)
	MORPTHIS_PRINTF(3, 4);

#define morpthis_logd(tag, ...) morpthis_log(MORPTHIS_LOG_DEBUG, tag, __VA_ARGS__)
#define morpthis_logi(tag, ...) morpthis_log(MORPTHIS_LOG_INFO, tag, __VA_ARGS__)
#define morpthis_logw(tag, ...) morpthis_log(MORPTHIS_LOG_WARN, tag, __VA_ARGS__)
#define morpthis_loge(tag, ...) morpthis_log(MORPTHIS_LOG_ERROR, tag, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* MORPTHIS_LOG_H */
