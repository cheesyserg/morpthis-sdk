#include "internal.h"

#include <string.h>
#include <time.h>

// The host keeps enough state for an app's logic to run; it has no DSP and no
// ALSA device. A test that needs to observe playback can read these back.

static morpthis_audio_state_t g_state;
static int g_volume = 50;
static int g_rate;
static bool g_output_open;

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

bool morpthis_audio_open(int rate, int channels, int buffer_ms) {
	if (rate <= 0 || channels < 1 || channels > 2 || buffer_ms < 0) {
		return false;
	}
	// Taking the output stops whatever the host thought was playing.
	g_state = MORPTHIS_AUDIO_STOPPED;
	g_rate = rate;
	g_output_open = true;
	return true;
}

size_t morpthis_audio_write(const int16_t *pcm, size_t frames) {
	if (!g_output_open || !pcm || frames == 0) {
		return 0;
	}

	// Pace the caller the way the device does, so a mixer thread does not
	// spin through its whole buffer list in a test.
	if (g_rate > 0) {
		struct timespec delay = {
			.tv_sec = (time_t)(frames / (size_t)g_rate),
			.tv_nsec = (long)((frames % (size_t)g_rate) * 1000000000L / (size_t)g_rate),
		};
		(void)nanosleep(&delay, NULL);
	}
	return frames;
}

void morpthis_audio_close(void) {
	g_output_open = false;
}
