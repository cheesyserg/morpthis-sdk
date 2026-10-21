#include "internal.h"

#include <string.h>

// The host keeps enough state for an app's logic to run; it has no DSP and no
// ALSA device. A test that needs to observe playback can read these back.

static morpthis_audio_state_t g_state;
static int g_volume = 50;

bool morpthis_audio_current(morpthis_track_t *out) {
	if (out) {
		memset(out, 0, sizeof(*out));
	}
	return false;
}

morpthis_audio_state_t morpthis_audio_state(void) {
	return g_state;
}

bool morpthis_audio_play(void) {
	g_state = MORPTHIS_AUDIO_PLAYING;
	return true;
}

bool morpthis_audio_pause(void) {
	if (g_state == MORPTHIS_AUDIO_PLAYING) {
		g_state = MORPTHIS_AUDIO_PAUSED;
	}
	return true;
}

bool morpthis_audio_next(void) {
	return true;
}

bool morpthis_audio_prev(void) {
	return true;
}

int morpthis_audio_volume(void) {
	return g_volume;
}

bool morpthis_audio_set_volume(int volume) {
	if (volume < 0) {
		volume = 0;
	}
	if (volume > 100) {
		volume = 100;
	}
	g_volume = volume;
	return true;
}
