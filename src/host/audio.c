#include "internal.h"

#include <string.h>
#include <time.h>

// The host keeps enough state for an app's logic to run; it has no DSP and no
// ALSA device. A test that needs to observe playback can read these back.

static morpthis_audio_state_t g_state;
static int g_volume = 50;
static int g_rate;
static int g_bits = 16;
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

static bool g_eq_enabled;
static int g_eq[MORPTHIS_EQ_BANDS];

bool morpthis_audio_eq_enabled(void) { return g_eq_enabled; }

void morpthis_audio_set_eq_enabled(bool enabled) { g_eq_enabled = enabled; }

int morpthis_audio_eq_band(int band) {
	return band >= 0 && band < MORPTHIS_EQ_BANDS ? g_eq[band] : 0;
}

bool morpthis_audio_set_eq_band(int band, int gain_db) {
	if (band < 0 || band >= MORPTHIS_EQ_BANDS) {
		return false;
	}
	if (gain_db < -12) {
		gain_db = -12;
	}
	if (gain_db > 12) {
		gain_db = 12;
	}
	g_eq[band] = gain_db;
	return true;
}

void morpthis_audio_eq_reset(void) {
	for (int i = 0; i < MORPTHIS_EQ_BANDS; i++) {
		g_eq[i] = 0;
	}
}

static bool valid_output(int rate, int channels, int bits, int buffer_ms) {
	return rate > 0 && channels >= 1 && channels <= 2 && buffer_ms >= 0 &&
		   (bits == 16 || bits == 24 || bits == 32);
}

bool morpthis_audio_open_ex(int rate, int channels, int bits, int buffer_ms) {
	if (!valid_output(rate, channels, bits, buffer_ms)) {
		return false;
	}
	// Taking the output stops whatever the host thought was playing.
	g_state = MORPTHIS_AUDIO_STOPPED;
	g_rate = rate;
	g_bits = bits;
	g_output_open = true;
	return true;
}

bool morpthis_audio_open(int rate, int channels, int buffer_ms) {
	return morpthis_audio_open_ex(rate, channels, 16, buffer_ms);
}

// Paces the caller the way the device does, so a mixer thread does not spin
// through its whole buffer list in a test.
static void pace(size_t frames) {
	if (g_rate <= 0 || frames == 0) {
		return;
	}
	struct timespec delay = {
		.tv_sec = (time_t)(frames / (size_t)g_rate),
		.tv_nsec = (long)((frames % (size_t)g_rate) * 1000000000L / (size_t)g_rate),
	};
	(void)nanosleep(&delay, NULL);
}

size_t morpthis_audio_write_pcm(const void *pcm, size_t frames) {
	if (!g_output_open || !pcm || frames == 0) {
		return 0;
	}
	pace(frames);
	return frames;
}

size_t morpthis_audio_write(const int16_t *pcm, size_t frames) {
	// The legacy call is 16-bit by contract; a 24/32-bit stream has a different
	// frame stride, and writing it here would be read as half-length frames.
	if (g_bits != 16) {
		return 0;
	}
	return morpthis_audio_write_pcm(pcm, frames);
}

void morpthis_audio_set_playing(bool playing) {
	// The host has no status bar and no listener for this; the call exists so an
	// app builds and runs unchanged. The player's implementation repaints its
	// playback indicator.
	(void)playing;
}

void morpthis_audio_set_track(const morpthis_track_t *track, bool playing) {
	// The headless host has no control centre to show it in.
	(void)track;
	(void)playing;
}

void morpthis_audio_close(void) {
	g_output_open = false;
	g_bits = 16;
}
