#ifndef MORPTHIS_UI_H
#define MORPTHIS_UI_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Widgets on the player's display.
//
// The player's own theme, fonts and layout scale are used; an app does not set
// colours or load fonts in API 1. Handles are opaque so the implementation can
// change without breaking an app.
//
// All UI calls must run on the GUI thread: the entry point and callbacks are
// already there. Do not allocate in a draw path (the player draws; an app only
// builds and updates widgets).
//
// Positions are in display pixels, origin top-left. The display is 240x320 in
// portrait; ask morpthis_ui_width()/height() rather than hard-coding it. The
// height is the content area below the player's title row, not the whole panel.

typedef struct morpthis_screen morpthis_screen_t;
typedef struct morpthis_widget morpthis_widget_t;

typedef void (*morpthis_click_cb)(morpthis_widget_t *widget, void *user);
typedef void (*morpthis_index_cb)(morpthis_widget_t *widget, int index, void *user);
typedef void (*morpthis_value_cb)(morpthis_widget_t *widget, int value, void *user);

// The drawable area, in pixels.
int morpthis_ui_width(void);
int morpthis_ui_height(void);

// Screens.
//
// Create one screen per app view. Loading shows it and hides the previous one;
// the player owns navigation chrome (title bar and back affordance), so the
// title is the only part an app sets.
morpthis_screen_t *morpthis_screen_create(const char *title);
void morpthis_screen_set_title(morpthis_screen_t *screen, const char *title);
void morpthis_screen_load(morpthis_screen_t *screen);
morpthis_screen_t *morpthis_screen_active(void);

// Destroys the screen and its widgets. Does not quit the app. Destroying the
// screen that is currently on display requests a quit instead (the player must
// not delete LVGL's active page underneath it); call morpthis_app_quit() to end
// the app from its own screen.
void morpthis_screen_destroy(morpthis_screen_t *screen);

// Widgets. A widget belongs to its screen and is destroyed with it unless it is
// destroyed earlier. `image` takes a path relative to the app folder and is
// decoded on the GUI thread, so keep it to icons and small pictures; an image is
// not interactive, and a press on it belongs to the page.
morpthis_widget_t *morpthis_label_create(morpthis_screen_t *screen, const char *text);
morpthis_widget_t *morpthis_button_create(morpthis_screen_t *screen, const char *text,
										  morpthis_click_cb cb, void *user);
morpthis_widget_t *morpthis_list_create(morpthis_screen_t *screen);
morpthis_widget_t *morpthis_image_create(morpthis_screen_t *screen, const char *relative_path);
morpthis_widget_t *morpthis_slider_create(morpthis_screen_t *screen, int min, int max,
										  int value, morpthis_value_cb cb, void *user);

// A framebuffer the app draws into: `width` x `height` RGB565 pixels, native
// endianness, stride = width. The player owns the buffer and it stays valid
// until the widget is destroyed. After changing pixels, invalidate the changed
// rectangle (relative to the canvas) so the player redraws exactly that part.
// This is the path for a game that renders its own frames; an app that only
// moves pictures should use image widgets instead. The canvas does not take
// presses: a touch on it reaches the app's touch callback, like a touch on the
// page itself.
morpthis_widget_t *morpthis_canvas_create(morpthis_screen_t *screen, int width, int height);
uint16_t *morpthis_canvas_buffer(morpthis_widget_t *canvas);
void morpthis_canvas_invalidate(morpthis_widget_t *canvas, int x, int y, int width, int height);

// Draws what has been invalidated, now, rather than on the player's next
// refresh. A game that redraws on its own clock calls this once after its
// invalidations, so the panel shows the frame it just drew in the same pass.
// Does nothing when nothing is invalidated. GUI thread only.
void morpthis_ui_refresh(void);

void morpthis_widget_destroy(morpthis_widget_t *widget);
void morpthis_widget_set_text(morpthis_widget_t *widget, const char *text);
const char *morpthis_widget_text(const morpthis_widget_t *widget);
void morpthis_widget_set_pos(morpthis_widget_t *widget, int x, int y);
void morpthis_widget_set_size(morpthis_widget_t *widget, int width, int height);
void morpthis_widget_set_visible(morpthis_widget_t *widget, bool visible);
void morpthis_widget_set_enabled(morpthis_widget_t *widget, bool enabled);

// Image widgets rotate clockwise around their centre, in whole degrees. The
// box keeps its size; a turned picture can draw outside it, as the artwork
// does when the bird tilts. Images only; other widgets ignore the call.
void morpthis_widget_set_rotation(morpthis_widget_t *widget, int degrees);

// 0 is invisible, 100 fully opaque. Applies to the widget and anything it
// contains. A full-screen image with this is a flash or a fade.
void morpthis_widget_set_opacity(morpthis_widget_t *widget, int opacity);

// Draws the widget above its siblings. Useful for an overlay that has to stay
// on top of widgets created later.
void morpthis_widget_raise(morpthis_widget_t *widget);

bool morpthis_widget_is_visible(const morpthis_widget_t *widget);
bool morpthis_widget_is_enabled(const morpthis_widget_t *widget);

// Lists. Items are copied. The selection callback runs on the GUI thread.
int morpthis_list_add(morpthis_widget_t *list, const char *text);

// A richer row: `detail` is the dim line under the title (NULL or empty for a
// single-line row), `icon` is a path relative to the package for the small
// leading picture (NULL for none), and `chevron` draws the marker used when a
// row opens another view. The player sizes the row and its fonts to match its
// own library lists; the app only supplies the text. Returns the item index,
// or -1.
int morpthis_list_add_entry(morpthis_widget_t *list, const char *title, const char *detail,
						const char *icon, bool chevron);

// Marks the row at `index` as the one playing: a short accent bar down its left
// edge. Pass -1 to clear it. Rows added with morpthis_list_add() take the same
// mark. The headless host runtime has no display and ignores it.
void morpthis_list_set_playing(morpthis_widget_t *list, int index);
void morpthis_list_clear(morpthis_widget_t *list);
int morpthis_list_count(const morpthis_widget_t *list);
void morpthis_list_set_select_cb(morpthis_widget_t *list, morpthis_index_cb cb, void *user);

// Sliders. The value is clamped to the range given at creation.
void morpthis_slider_set_value(morpthis_widget_t *slider, int value);
int morpthis_slider_value(const morpthis_widget_t *slider);

// A one-line message in the player's toast area. Copied.
void morpthis_toast(const char *text);

#ifdef __cplusplus
}
#endif

#endif /* MORPTHIS_UI_H */
