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

// The player's own glyphs, for a transport row or a corner action. The set is
// deliberately small: an app draws the player's controls, not a second icon
// language.
typedef enum {
	MORPTHIS_ICON_PLAY = 0,
	MORPTHIS_ICON_PAUSE,
	MORPTHIS_ICON_PREV,
	MORPTHIS_ICON_NEXT,
	MORPTHIS_ICON_STOP,
	MORPTHIS_ICON_QUEUE,
	MORPTHIS_ICON_MORE,
	MORPTHIS_ICON_SEARCH,
	MORPTHIS_ICON_CLOSE,

	// API 3 additions. The player's own glyph set, for a row, a card or a
	// status line. Unknown values draw a music note.
	MORPTHIS_ICON_CHEVRON_LEFT,
	MORPTHIS_ICON_CHEVRON_RIGHT,
	MORPTHIS_ICON_CHEVRON_UP,
	MORPTHIS_ICON_ALBUM,
	MORPTHIS_ICON_ARTIST,
	MORPTHIS_ICON_FOLDER,
	MORPTHIS_ICON_FILE,
	MORPTHIS_ICON_MUSIC,
	MORPTHIS_ICON_PLAYLIST,
	MORPTHIS_ICON_HEART,
	MORPTHIS_ICON_HEART_FILLED,
	MORPTHIS_ICON_STAR,
	MORPTHIS_ICON_STAR_FILLED,
	MORPTHIS_ICON_SHUFFLE,
	MORPTHIS_ICON_REPEAT_ALL,
	MORPTHIS_ICON_REPEAT_ONE,
	MORPTHIS_ICON_SORT,
	MORPTHIS_ICON_CHECK,
	MORPTHIS_ICON_PLUS,
	MORPTHIS_ICON_DELETE,
	MORPTHIS_ICON_REFRESH,
	MORPTHIS_ICON_INFO,
	MORPTHIS_ICON_ALERT,
	MORPTHIS_ICON_WIFI,
	MORPTHIS_ICON_BLUETOOTH,
	MORPTHIS_ICON_BATTERY,
	MORPTHIS_ICON_CHARGING,
	MORPTHIS_ICON_EQUALIZER,
	MORPTHIS_ICON_SETTINGS,
	MORPTHIS_ICON_SLEEP,
	MORPTHIS_ICON_SUN,
	MORPTHIS_ICON_VOLUME_HIGH,
	MORPTHIS_ICON_VOLUME_LOW,
	MORPTHIS_ICON_VOLUME_MUTE,
	MORPTHIS_ICON_HEADPHONES,
	MORPTHIS_ICON_POWER,
	MORPTHIS_ICON_REBOOT,
} morpthis_icon_t;

typedef void (*morpthis_click_cb)(morpthis_widget_t *widget, void *user);
typedef void (*morpthis_index_cb)(morpthis_widget_t *widget, int index, void *user);
typedef void (*morpthis_value_cb)(morpthis_widget_t *widget, int value, void *user);

typedef enum {
	MORPTHIS_TEXT_LEFT = 0,
	MORPTHIS_TEXT_CENTER,
	MORPTHIS_TEXT_RIGHT,
} morpthis_text_align_t;

// The drawable area, in pixels. `full_height` is what a screen made with
// morpthis_screen_create_full() gets: the panel under the status bar, without
// the title row.
int morpthis_ui_width(void);
int morpthis_ui_height(void);
int morpthis_ui_full_height(void);

// Screens.
//
// Create one screen per app view. Loading shows it and hides the previous one;
// the player owns navigation chrome (title bar and back affordance), so the
// title is the only part an app sets.
morpthis_screen_t *morpthis_screen_create(const char *title);

// A screen without the title row: its content starts under the status bar and
// runs to the bottom. The back chevron, the swipe-back and the app's BACK
// handler remain; there is no title label or player background button (and so
// no place for morpthis_screen_add_corner_button()). For a now-playing page or
// a full-bleed view.
morpthis_screen_t *morpthis_screen_create_full(const char *title);
void morpthis_screen_set_title(morpthis_screen_t *screen, const char *title);
void morpthis_screen_load(morpthis_screen_t *screen);
morpthis_screen_t *morpthis_screen_active(void);

// A page-opening action in the title row, to the left of the player's own
// background button. The player narrows the title to make room. Game screens
// have no title row, so this returns NULL there. The returned widget can be
// destroyed like any other; destroying a screen destroys its buttons too.
morpthis_widget_t *morpthis_screen_add_corner_button(morpthis_screen_t *screen,
												 morpthis_icon_t icon,
												 morpthis_click_cb cb, void *user);

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

// A square, transparent button with one of the player's glyphs centred in it.
// `size` is the hit target in pixels; 0 uses the player's 44/56 px control.
// The glyph is scaled to fit. Returns NULL when the screen is gone or the
// player has no such icon.
morpthis_widget_t *morpthis_icon_button_create(morpthis_screen_t *screen, morpthis_icon_t icon,
												 int size, morpthis_click_cb cb, void *user);

// Changes an icon next to its text or in an icon button. Other widgets ignore it.
void morpthis_widget_set_icon(morpthis_widget_t *widget, morpthis_icon_t icon);

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

// A repeating callback on the GUI thread, for an app's own clock. The handle
// belongs to the app and is deleted automatically when the app unloads.
// `period_ms` is clamped to at least 1 ms. The callback may change, reset or
// delete its own timer. This is a normal player timer, not a high-resolution
// clock: do not use it for audio pacing.
typedef struct morpthis_timer morpthis_timer_t;
typedef void (*morpthis_timer_cb)(morpthis_timer_t *timer, void *user);

morpthis_timer_t *morpthis_timer_create(uint32_t period_ms, morpthis_timer_cb cb, void *user);
void morpthis_timer_set_period(morpthis_timer_t *timer, uint32_t period_ms);
void morpthis_timer_reset(morpthis_timer_t *timer);
void morpthis_timer_delete(morpthis_timer_t *timer);

void morpthis_widget_destroy(morpthis_widget_t *widget);
void morpthis_widget_set_text(morpthis_widget_t *widget, const char *text);
const char *morpthis_widget_text(const morpthis_widget_t *widget);
void morpthis_widget_set_pos(morpthis_widget_t *widget, int x, int y);

// -1 makes that dimension follow the widget's content. The other value is
// pixels. Images use a content-sized box as their decode target fallback.
#define MORPTHIS_SIZE_CONTENT (-1)
void morpthis_widget_set_size(morpthis_widget_t *widget, int width, int height);
void morpthis_widget_set_visible(morpthis_widget_t *widget, bool visible);
void morpthis_widget_set_enabled(morpthis_widget_t *widget, bool enabled);

// Labels and buttons: pick one of the player's text sizes (14, 16, 18, 20, 22,
// 24, 26, 32). Other values snap to the nearest. `dim` uses the player's
// secondary text colour and follows a theme change; false restores the primary
// colour. Other widgets ignore both calls.
void morpthis_widget_set_text_size(morpthis_widget_t *widget, int size);
void morpthis_widget_set_text_dim(morpthis_widget_t *widget, bool dim);
void morpthis_widget_set_text_align(morpthis_widget_t *widget, morpthis_text_align_t align);

// Text layout for labels and buttons. wrap() breaks a long text over several
// lines instead of clipping it; scroll() uses the player's marquee, which waits
// before moving and pauses at the end of each pass. Both are off by default.
void morpthis_widget_set_text_wrap(morpthis_widget_t *widget, bool wrap);
void morpthis_widget_set_text_scroll(morpthis_widget_t *widget, bool scroll);

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

// Moves a widget into a container (or back to the screen when `parent` is
// NULL). The widget keeps its position unless the container has a layout, in
// which case the layout decides. A container's children are destroyed with it.
void morpthis_widget_set_parent(morpthis_widget_t *widget, morpthis_widget_t *parent);

// Style overrides for an app that draws its own cards. Colours are 0xRRGGBB;
// opacity is 0..100 (-1 leaves it unchanged). These are local properties on
// top of the player's styles, so a later theme change still repaints anything
// the app did not override.
void morpthis_widget_set_bg_color(morpthis_widget_t *widget, uint32_t rgb, int opacity);
void morpthis_widget_set_text_color(morpthis_widget_t *widget, uint32_t rgb);
void morpthis_widget_set_radius(morpthis_widget_t *widget, int radius);
void morpthis_widget_set_padding(morpthis_widget_t *widget, int left, int top, int right, int bottom);

bool morpthis_widget_is_visible(const morpthis_widget_t *widget);
bool morpthis_widget_is_enabled(const morpthis_widget_t *widget);

// Lists. Items are copied. The selection callback runs on the GUI thread.
int morpthis_list_add(morpthis_widget_t *list, const char *text);

// Scrolls the row at `index` into view. False when the index is out of range.
bool morpthis_list_scroll_to(morpthis_widget_t *list, int index);

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
void morpthis_slider_set_range(morpthis_widget_t *slider, int min, int max);

// Called when the user lifts a finger from the slider, and when the press is
// lost, with the final value. A seek bar uses it: the value callback runs
// continuously while the finger moves, and the release is the moment worth
// acting on. Programmatic morpthis_slider_set_value() calls do not fire it.
void morpthis_slider_set_release_cb(morpthis_widget_t *slider, morpthis_value_cb cb, void *user);

// A switch. The value callback receives 1 when it goes on and 0 when it goes
// off. set_on() paints the new state without firing the callback.
morpthis_widget_t *morpthis_switch_create(morpthis_screen_t *screen, bool on,
										  morpthis_value_cb cb, void *user);
bool morpthis_switch_is_on(const morpthis_widget_t *widget);
void morpthis_switch_set_on(morpthis_widget_t *widget, bool on);

// A progress bar, 0..max. The value is clamped. The accent fill follows the
// theme. set_range() keeps the current value if it still fits.
morpthis_widget_t *morpthis_progress_create(morpthis_screen_t *screen, int min, int max, int value);
void morpthis_progress_set_value(morpthis_widget_t *widget, int value);
int morpthis_progress_value(const morpthis_widget_t *widget);
void morpthis_progress_set_range(morpthis_widget_t *widget, int min, int max);

// The player's rotating loader, for work with no known length. The animation
// stops when the widget is destroyed.
morpthis_widget_t *morpthis_spinner_create(morpthis_screen_t *screen);

// A circular gauge, for a battery ring or a knob. The value is clamped to the
// range; a drag reports through the value callback. set_value() does not fire
// it. The background is transparent and the indicator follows the accent.
morpthis_widget_t *morpthis_arc_create(morpthis_screen_t *screen, int min, int max, int value,
									   morpthis_value_cb cb, void *user);
void morpthis_arc_set_value(morpthis_widget_t *arc, int value);
int morpthis_arc_value(const morpthis_widget_t *arc);
void morpthis_arc_set_range(morpthis_widget_t *arc, int min, int max);

// A container. With no layout it is a transparent box; with a row or column
// layout its children are arranged by the player and `gap` is the space
// between them. Scroll `vertical` when the content is taller than the box.
typedef enum {
	MORPTHIS_LAYOUT_NONE = 0,
	MORPTHIS_LAYOUT_ROW,
	MORPTHIS_LAYOUT_COLUMN,
} morpthis_layout_t;

typedef enum {
	MORPTHIS_ALIGN_START = 0,
	MORPTHIS_ALIGN_CENTER,
	MORPTHIS_ALIGN_END,
	MORPTHIS_ALIGN_SPACE_BETWEEN,
} morpthis_align_t;

morpthis_widget_t *morpthis_container_create(morpthis_screen_t *screen);
void morpthis_container_set_layout(morpthis_widget_t *container, morpthis_layout_t layout,
								   morpthis_align_t main_align, morpthis_align_t cross_align, int gap);
void morpthis_container_set_scrollable(morpthis_widget_t *container, bool vertical);

// Image widgets: replace the picture. The path is resolved like an fs read:
// the app's data folder first, then the package. That is what lets an app
// download a cover into its data folder and show it. The old pixels are freed.
void morpthis_image_set_source(morpthis_widget_t *image, const char *relative_path);

// A QR code. `size` is the square's edge in pixels. The text is encoded now;
// set_text() re-encodes it. The code uses the theme's text colour on its
// surface colour.
morpthis_widget_t *morpthis_qrcode_create(morpthis_screen_t *screen, int size);
void morpthis_qrcode_set_text(morpthis_widget_t *qrcode, const char *text);

// A wheel of options, the player's roller. The labels are copied and joined
// with newlines. The callback receives the selected index. `visible_rows` is
// how many rows the wheel shows (0 uses three).
morpthis_widget_t *morpthis_roller_create(morpthis_screen_t *screen, const char *const *options,
										  int count, int selected, int visible_rows,
										  morpthis_value_cb cb, void *user);
void morpthis_roller_set_selected(morpthis_widget_t *roller, int index);
int morpthis_roller_selected(const morpthis_widget_t *roller);

// A row of pill buttons, the player's segmented choice. `selected` is the
// initially chosen index (-1 for none); the callback receives the new index.
// The labels are copied. A row created with this is narrower than a full-width
// settings card; put it in a container or set its size explicitly.
morpthis_widget_t *morpthis_segmented_create(morpthis_screen_t *screen, const char *const *labels,
										 int count, int selected, morpthis_value_cb cb, void *user);
void morpthis_segmented_set_selected(morpthis_widget_t *segmented, int index);
int morpthis_segmented_selected(const morpthis_widget_t *segmented);

// A settings row in the player's own shape: title, optional dim detail under
// it, optional value on the right, and a chevron when there is no value. The
// callback runs on a tap. The row is the width of its parent by default.
morpthis_widget_t *morpthis_settings_row_create(morpthis_screen_t *screen, const char *title,
										  const char *detail, const char *value,
										  morpthis_click_cb cb, void *user);
void morpthis_settings_row_set_value(morpthis_widget_t *row, const char *value);

// An inline text field. A tap opens the player's keyboard in a modal; accepting
// writes the text back and calls `cb` with `accepted` true. Cancelling calls it
// with false and the unchanged text. `title` is the prompt's heading. The field
// itself is read-only so the keyboard is always the one the player drew.
typedef void (*morpthis_text_cb)(bool accepted, const char *text, void *user);

morpthis_widget_t *morpthis_text_input_create(morpthis_screen_t *screen, const char *title,
										  const char *initial, bool password,
										  morpthis_text_cb cb, void *user);
void morpthis_text_input_set_text(morpthis_widget_t *input, const char *text);
const char *morpthis_text_input_get_text(const morpthis_widget_t *input);

// Menus. The player's popover: a compact modal card next to an anchor, with the
// page dimmed behind it and the current choice ticked. One menu is open at a
// time; showing another replaces it, and a tap outside closes it.
//
// morpthis_menu_show() anchors to a widget, typically the button that opened
// it. morpthis_menu_show_row() anchors to a row of a list, which is what a
// settings page uses so the menu appears next to the row the user tapped. Both
// copy the labels before returning and return false when the player cannot show
// a menu (the host runtime has no display and always returns false).
//
// The callback runs on the GUI thread after the menu has closed. Picking an
// item calls it with that index; a tap outside, or morpthis_menu_close(), does
// not call it.
#define MORPTHIS_MENU_MAX 7

typedef struct {
	const char *label;
	bool checked; // draws the tick; use it for the current choice
} morpthis_menu_item_t;

typedef void (*morpthis_menu_cb)(int index, void *user);

bool morpthis_menu_show(morpthis_widget_t *anchor, const morpthis_menu_item_t *items, int count,
						morpthis_menu_cb cb, void *user);
bool morpthis_menu_show_row(morpthis_widget_t *list, int row, const morpthis_menu_item_t *items,
							int count, morpthis_menu_cb cb, void *user);
void morpthis_menu_close(void);

// A one-line message in the player's toast area. Copied.
void morpthis_toast(const char *text);

// The three cards the player uses: a green success tick, a red error, and a
// spinner that stays until morpthis_toast_busy_end() or the next toast.
void morpthis_toast_success(const char *text);
void morpthis_toast_error(const char *text);
void morpthis_toast_busy(const char *text);
void morpthis_toast_busy_end(void);

// The player's full-screen modal, for work whose length is known (a copy, a
// scan) or unknown (pairing). progress() takes -1 to hide the bar. The app's
// modal is closed on background/unload. One modal exists at a time.
void morpthis_ui_modal_show(const char *title, const char *subtitle);
void morpthis_ui_modal_progress(int percent);
void morpthis_ui_modal_hide(void);

// ---------------------------------------------------------------------------
// Theme, chrome and modal dialogs
// ---------------------------------------------------------------------------

// A snapshot of the palette everything on screen is drawn from. Colours are
// 0xRRGGBB, so an app can tint its own surfaces to match the player.
// `morpthis_ui_set_dark()` and `morpthis_ui_set_accent()` change the player's
// appearance exactly as the Appearance page does and repaint every page.
typedef struct {
	uint32_t screen_bg;
	uint32_t surface;
	uint32_t panel;
	uint32_t cover_bg;
	uint32_t text_primary;
	uint32_t text_secondary;
	uint32_t accent;
	bool dark;
} morpthis_theme_t;

void morpthis_ui_theme(morpthis_theme_t *out);
bool morpthis_ui_set_dark(bool dark);
bool morpthis_ui_set_accent(int index); // 0..11, the Appearance page's presets

// The status bar above the app. Full screens keep it; a game does not and this
// call cannot bring it back.
void morpthis_ui_set_status_bar_visible(bool visible);
bool morpthis_ui_status_bar_visible(void);

// The player's modal yes/no card. `message` may be NULL; `ok_label` NULL uses
// "OK". The callback runs on the GUI thread after the card closes. There is
// one dialog at a time; showing another replaces it, and a tap outside is a
// cancel (no callback).
typedef void (*morpthis_action_cb)(void *user);

bool morpthis_ui_confirm(const char *title, const char *message, const char *ok_label,
					 morpthis_action_cb cb, void *user);
void morpthis_ui_notice(const char *title, const char *message);

// The player's own keyboard, in a modal card. `initial` may be NULL; `password`
// draws bullets. The callback runs once, after the card has closed, with
// `accepted` false when the user backed out. `text` is valid only for the
// duration of the call -- copy it. Returns false when the player cannot show
// the prompt (the host runtime has no display).
bool morpthis_ui_prompt_text(const char *title, const char *initial, bool password,
						 morpthis_text_cb cb, void *user);

#ifdef __cplusplus
}
#endif

#endif /* MORPTHIS_UI_H */
