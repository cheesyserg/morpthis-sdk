#ifndef MORPTHIS_AUDIO_H
#define MORPTHIS_AUDIO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// The player's playback and the raw output.
//
// An app that never calls these leaves the player's audio alone: launching it
// does not pause or stop what is playing. An app that wants the DAC takes it
// over explicitly with morpthis_audio_play() and should release it by pausing
// before quitting, unless the app is itself a player.
//
// An app that writes its own samples takes the output with
// morpthis_audio_open(). That stops playback and radio, and nothing else can
// play until morpthis_audio_close(). Open, write and close from one thread: a
// write blocks at the rate the DAC plays, so that thread is the app's mixer.
// The device has no hardware mixer; the samples are what the DAC plays.

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

// Raw PCM output.
//
// The stream is interleaved and native endianness. `channels` is 1 or 2 and
// `buffer_ms` is the latency to ask the DAC for (0 uses the player's usual deep
// depth, about 750 ms and independent of its local playback buffer setting; a
// game wants 30-60). Stops playback and radio first. Returns false when the
// output cannot be taken (no card, or the device is busy). A second open with
// the same terms is a no-op; different terms close the old stream and open a
// new one, which is what a music app changing sample rate needs.
//
// `morpthis_audio_open` is the 16-bit form. The extended form chooses the
// sample format:
//
//   16  signed 16-bit, 2 bytes per sample
//   24  packed signed 24-bit, 3 bytes per sample (ALSA S24_3LE)
//   32  signed 32-bit, left-justified: a 24-bit sample occupies the top 24 bits
//
// The built-in DAC takes 24-bit samples in its S24 container, so a 24-bit FLAC
// written as 32-bit left-justified reaches it bit-perfect. A USB DAC is given
// S32 when it accepts it. Open, write and close from one thread; a write blocks
// at the rate the DAC plays.
bool morpthis_audio_open(int rate, int channels, int buffer_ms);
bool morpthis_audio_open_ex(int rate, int channels, int bits, int buffer_ms);

// Writes `frames` frames in the depth selected by open_ex(), or 16-bit after
// the legacy open(). Returns the frames written; 0 when the output is not open
// or has failed. `morpthis_audio_write` is the 16-bit convenience and returns 0
// if the output was opened at another depth; use this call for 24/32-bit data.
size_t morpthis_audio_write_pcm(const void *pcm, size_t frames);
size_t morpthis_audio_write(const int16_t *pcm, size_t frames);

// Tells the player whether the app's output is producing sound right now, so
// the status-bar indicator follows pause and resume. The power manager treats
// it like local playback too: while true the device stays awake with the screen
// off (and Wi-Fi is not parked), and pausing releases that hold. The screen
// still turns off on its own idle timer. Safe to call from any thread,
// including the app's mixer and the GUI thread; it is a no-op when no output is
// open. Closing the output clears it automatically.
void morpthis_audio_set_playing(bool playing);

// Publishes the track the app is playing, for the player's control centre.
// `track` is copied; NULL clears it. The player shows title, artist, album and
// duration on the now-playing card, and taps there return to the app's
// MORPTHIS_BUTTON_NOW_PLAYING handler. `playing` also updates the status-bar
// glyph. Call it when a track starts, changes or stops; it is safe from any
// thread. Closing the output clears the published track.
void morpthis_audio_set_track(const morpthis_track_t *track, bool playing);

// Releases the output. Playback does not resume by itself; the user starts it
// again. Idempotent.
void morpthis_audio_close(void);

#ifdef __cplusplus
}
#endif

#endif /* MORPTHIS_AUDIO_H */
