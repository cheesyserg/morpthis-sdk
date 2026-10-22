#include "internal.h"

#include <stdlib.h>
#include <string.h>

// The host runner does not open sockets. The calls fail cleanly with a reason;
// an app can still exercise its error paths, and CI never touches the network.

bool morpthis_http_get(const char *url, const char *extra_headers, bool want_error_body,
					   size_t limit, int timeout_secs, morpthis_http_response_t *out) {
	(void)url;
	(void)extra_headers;
	(void)want_error_body;
	(void)limit;
	(void)timeout_secs;
	if (out) {
		memset(out, 0, sizeof(*out));
	}
	return false;
}

bool morpthis_http_perform(const char *url, const morpthis_http_opts_t *opts,
						   morpthis_http_response_t *out) {
	(void)url;
	(void)opts;
	if (out) {
		memset(out, 0, sizeof(*out));
	}
	return false;
}

void morpthis_http_response_free(morpthis_http_response_t *response) {
	if (!response) {
		return;
	}
	free(response->body);
	response->body = NULL;
	response->length = 0;
	response->status = 0;
}

const char *morpthis_http_last_error(void) { return "the host runtime has no network"; }

char *morpthis_http_url_encode(const char *in, char *out, size_t size) {
	if (!out || size == 0) {
		return out;
	}
	size_t used = 0;
	const unsigned char *p = (const unsigned char *)(in ? in : "");
	static const char hex[] = "0123456789ABCDEF";
	while (*p && used + 1 < size) {
		unsigned char c = *p++;
		bool safe = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
					c == '-' || c == '_' || c == '.' || c == '~';
		if (safe) {
			out[used++] = (char)c;
		} else if (used + 3 < size) {
			out[used++] = '%';
			out[used++] = hex[c >> 4];
			out[used++] = hex[c & 0x0f];
		} else {
			break;
		}
	}
	out[used] = '\0';
	return out;
}
