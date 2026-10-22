#ifndef MORPTHIS_PLAYBACK_H
#define MORPTHIS_PLAYBACK_H

#include <stdbool.h>
#include <stddef.h>

#include "morpthis/audio.h"

#ifdef __cplusplus
extern "C" {
#endif

// The player's own playback engine.
//
// audio.h is the transport an app uses to control whatever is loaded, and the
// raw PCM output an app uses to play its own stream. This header is the other
// half: it lets an app put the player's library on the queue, seek it, read the
// queue and follow what the player is doing. It is the same engine the player's
// own pages drive, so an app can be a first-class music view rather than a
// second player fighting for the DAC.
//
// All calls run on the GUI thread, like the rest of the UI-facing SDK. Paths
// are absolute. The card may disappear at any time: a call that reads the
// queue or starts a file reports failure rather than blocking.

#define MORPTHIS_PLAYBACK_PATH_MAX 1024
#define MORPTHIS_PLAYBACK_CODEC_MAX 24

typedef enum {
	MORPTHIS_PLAYBACK_NORMAL = 0, // play through the queue, then stop
	MORPTHIS_PLAYBACK_REPEAT_ALL,
	MORPTHIS_PLAYBACK_REPEAT_ONE,
	MORPTHIS_PLAYBACK_SHUFFLE,
	MORPTHIS_PLAYBACK_SHUFFLE_REPEAT,
} morpthis_playback_mode_t;

typedef struct {
	char path[MORPTHIS_PLAYBACK_PATH_MAX]; // empty for a live stream
	char title[MORPTHIS_TRACK_TITLE_MAX];  // may be a file name when untagged
	char artist[MORPTHIS_TRACK_ARTIST_MAX];
	char album[MORPTHIS_TRACK_ALBUM_MAX];
	char codec[MORPTHIS_PLAYBACK_CODEC_MAX]; // "FLAC", "MP3", ...; empty when unknown

	int sample_rate;  // Hz, 0 when unknown
	int channels;	  // 0 when unknown
	int bit_depth;	  // source depth: 16, 24, 32; 0 when unknown
	int bitrate_kbps; // nominal, 0 when unknown
	int dsd_multiple; // 64/128/256 for DSD, 0 otherwise
	bool lossy;
	bool live; // an internet stream rather than a file

	int position_ms;
	int duration_ms; // 0 when unknown or live

	int queue_index; // position in playing order, -1 when empty
	int queue_count;
	morpthis_playback_mode_t mode;
	morpthis_audio_state_t state;
} morpthis_playback_info_t;

// Starts `absolute_path` at `start_ms` (0 for the beginning). The queue is
// rebuilt from the file's folder, exactly as tapping a file in the browser
// does, so next and prev keep working. Returns false when the path is empty or
// cannot be read.
bool morpthis_playback_play_file(const char *absolute_path, int start_ms);

// Plays an explicit queue -- a playlist, an album, a list of folders the app
// built. `paths` must hold `count` non-NULL absolute paths and `start_index`
// must be in range. The queue is custom: the player will not rebuild it from a
// folder. Copied before the call returns.
bool morpthis_playback_play_queue(const char *const *paths, int count, int start_index);

// Plays `selected_name` inside `absolute_folder`, with the folder as the queue.
// Returns false when the file is not there.
bool morpthis_playback_play_folder(const char *absolute_folder, const char *selected_name);

// Jumps to entry `index` of the current queue.
bool morpthis_playback_play_index(int index);

// Transport. resume() starts the loaded track again after a stop; toggle()
// follows the player's own play/pause button.
bool morpthis_playback_resume(void);
bool morpthis_playback_pause(void);
bool morpthis_playback_toggle(void);
bool morpthis_playback_stop(void);
bool morpthis_playback_next(void);
bool morpthis_playback_prev(void);

// Seeks to `position_ms`. 0 is the start; a value past the end is clamped by
// the engine. False when nothing is loaded.
bool morpthis_playback_seek(int position_ms);

// Fills `out` from one snapshot. Returns false when nothing is loaded; the
// queue fields are still filled when a queue exists.
bool morpthis_playback_get_info(morpthis_playback_info_t *out);

void morpthis_playback_set_mode(morpthis_playback_mode_t mode);
morpthis_playback_mode_t morpthis_playback_get_mode(void);

// Playback speed for spoken word. 1.0 is normal; the player's stretcher keeps
// the pitch. False for a factor outside the player's accepted range.
bool morpthis_playback_set_speed(double factor);
double morpthis_playback_get_speed(void);

// The queue, in playing order. `queue_path` copies an absolute path.
int morpthis_playback_queue_count(void);
int morpthis_playback_queue_index(void);
bool morpthis_playback_queue_path(int index, char *out, size_t size);

// Inserts an absolute path into the queue. `slot` is a position in playing
// order; -1 inserts after the track playing now (the "add to queue" gesture).
// Returns false when the path is empty or the queue could not grow.
bool morpthis_playback_queue_insert(const char *absolute_path, int slot);

// Empties the queue. What is already playing is not stopped; the next/prev
// transport calls then have nowhere to go.
bool morpthis_playback_queue_clear(void);

// The picture the player's now-playing page would show for the loaded track,
// as an absolute path to a decodable image file. The path points at the track
// itself when the artwork is embedded, so it is not necessarily an image file;
// use the image widget or the player's own cover instead when it matters.
// Borrowed until the next track change. False when there is no artwork.
bool morpthis_playback_cover_path(char *out, size_t size);

// Called on the GUI thread when the track, transport state, queue or mode
// changes. Position is not a change: poll get_info() for a progress bar.
// Passing NULL unsubscribes. The callback is dropped when the app unloads.
typedef void (*morpthis_playback_changed_cb)(void *user);
void morpthis_playback_set_changed_cb(morpthis_playback_changed_cb cb, void *user);

#ifdef __cplusplus
}
#endif

#endif /* MORPTHIS_PLAYBACK_H */
