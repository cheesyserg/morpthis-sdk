#ifndef MORPTHIS_NET_H
#define MORPTHIS_NET_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// The player's HTTP/1.1 client, the one the streaming services and the radio
// directory use.
//
// http:// goes over a plain socket; https:// goes over the OpenSSL already on
// the device, with the player's CA bundle and its strict/relaxed verification
// setting. That makes this the easy way for an app to talk to a server without
// carrying its own TLS stack into the player's 18 MB.
//
// Every call blocks: connect, TLS handshake, request and response. Call it from
// an app-owned thread, never from the GUI thread. A timeout bounds the connect
// and every read, and the response body is capped by `limit`.
//
// The body is malloc'd and NUL-terminated. Free the whole response with
// morpthis_http_response_free(); do not free the body yourself.

typedef struct {
	char *body;		// allocated, NUL-terminated; NULL on failure
	size_t length;	// bytes, not counting the terminator
	int status;		// HTTP status, 0 when the request failed
} morpthis_http_response_t;

typedef struct {
	// "GET", "POST", "DELETE". NULL means GET.
	const char *method;

	// Lines already terminated with "\r\n", one per header, or NULL.
	const char *extra_headers;

	// Request body, or NULL. `body_len` 0 with a non-NULL body means strlen().
	const void *body;
	size_t body_len;

	// Body type; NULL with a body present means form-urlencoded.
	const char *content_type;

	// Keep a 4xx body instead of discarding it: that is where an API says why
	// it refused. A 5xx is always a failure.
	bool want_error_body;

	// Read success from the status line and accept an empty body. Needed for
	// 201/204 and for a DELETE that answers with nothing.
	bool allow_empty_body;

	// Never follow a redirect from https:// down to http://. Set it for
	// requests carrying credentials.
	bool no_https_downgrade;

	size_t limit;	 // maximum body bytes kept; 0 means 1 MiB
	int timeout_secs; // connect and per-read timeout; 0 means 15 s
} morpthis_http_opts_t;

// A whole small response into memory. Returns false on any failure, with `out`
// zeroed. `extra_headers` may be NULL.
bool morpthis_http_get(const char *url, const char *extra_headers, bool want_error_body,
					   size_t limit, int timeout_secs, morpthis_http_response_t *out);

// The general form, with a method and a body.
bool morpthis_http_perform(const char *url, const morpthis_http_opts_t *opts,
						   morpthis_http_response_t *out);

void morpthis_http_response_free(morpthis_http_response_t *response);

// Why the last request on THIS thread failed, when the reason is one a person
// can act on (an expired clock, a missing certificate bundle, a name mismatch).
// NULL when there is nothing useful to say. Borrowed until the next request on
// the same thread.
const char *morpthis_http_last_error(void);

// Percent-encodes `in` into `out`. Returns `out`.
char *morpthis_http_url_encode(const char *in, char *out, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* MORPTHIS_NET_H */
