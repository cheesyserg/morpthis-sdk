#ifndef MORPTHIS_AUDIO_H
#define MORPTHIS_AUDIO_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// The player's playback.
//
// An app that never calls these leaves the player's audio alone: launching it
// does not pause or stop what is playing. An app that wants the DAC takes it
// over explicitly with morpthis_audio_play() and should release it by pausing
// before quitting, unless the app is itself a player.
//
// There is no PCM access in API 1. An app that synthesises audio asks for it
// when the player has a mixer to offer.

#define MORPTHIS_TRACK_TITLE_MAX 128
#define MORPTHIS_TRACK_ARTIST_MAX 96
#define MORPTHIS_TRACK_ALBUM_MAX 96

typedef enum {
	MORPTHIS_AUDIO_STOPPED = 0,
	MORPTHIS_AUDIO_PLAYING,
	MORPTHIS_AUDIO_PAUSED
} morpthis_audio_state_t;

typedef struct {
	char title[MORPTHIS_TRACK_TITLE_MAX];
	char artist[MORPTHIS_TRACK_ARTIST_MAX];
	char album[MORPTHIS_TRACK_ALBUM_MAX];
	int duration_ms; // 0 when unknown
} morpthis_track_t;

// Copies the current track into `out` and returns true. False when nothing is
// loaded; `out` is zeroed then.
bool morpthis_audio_current(morpthis_track_t *out);
morpthis_audio_state_t morpthis_audio_state(void);

bool morpthis_audio_play(void);
bool morpthis_audio_pause(void);
bool morpthis_audio_next(void);
bool morpthis_audio_prev(void);

// 0..100.
int morpthis_audio_volume(void);
bool morpthis_audio_set_volume(int volume);

#ifdef __cplusplus
}
#endif

#endif /* MORPTHIS_AUDIO_H */
