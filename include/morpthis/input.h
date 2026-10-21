#ifndef MORPTHIS_INPUT_H
#define MORPTHIS_INPUT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Buttons and touch.
//
// Widgets already receive taps; this interface is for an app that draws its own
// surface or wants the physical buttons. Set a callback once in the entry point;
// the player calls it on the GUI thread. A callback set by a later call
// replaces the previous one; setting NULL unsubscribes.
//
// On the V1 the transport keys arrive as PRESS only (the kernel repeat becomes
// another PRESS); BACK arrives as PRESS then RELEASE from the chevron or swipe,
// and the player then quits the app. Volume keys stay the player's.

typedef enum {
	MORPTHIS_BUTTON_PLAY_PAUSE = 0,
	MORPTHIS_BUTTON_NEXT,
	MORPTHIS_BUTTON_PREV,
	MORPTHIS_BUTTON_VOLUME_UP,
	MORPTHIS_BUTTON_VOLUME_DOWN,
	MORPTHIS_BUTTON_POWER,
	MORPTHIS_BUTTON_BACK
} morpthis_button_t;

typedef enum {
	MORPTHIS_INPUT_PRESS = 0,
	MORPTHIS_INPUT_RELEASE,
	MORPTHIS_INPUT_REPEAT
} morpthis_input_action_t;

typedef void (*morpthis_button_cb)(morpthis_button_t button,
								   morpthis_input_action_t action, void *user);

typedef struct {
	int x;
	int y;
} morpthis_point_t;

typedef enum {
	MORPTHIS_TOUCH_DOWN = 0,
	MORPTHIS_TOUCH_MOVE,
	MORPTHIS_TOUCH_UP,
	MORPTHIS_TOUCH_CANCEL
} morpthis_touch_action_t;

typedef void (*morpthis_touch_cb)(morpthis_touch_action_t action,
								  morpthis_point_t point, void *user);

void morpthis_input_set_button_cb(morpthis_button_cb cb, void *user);
void morpthis_input_set_touch_cb(morpthis_touch_cb cb, void *user);

#ifdef __cplusplus
}
#endif

#endif /* MORPTHIS_INPUT_H */
