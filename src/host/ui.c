#include "internal.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define HOST_DISPLAY_WIDTH 240
#define HOST_DISPLAY_HEIGHT 320

static morpthis_screen_t *g_active;

static char *dup_text(const char *text) {
	if (!text) {
		text = "";
	}
	size_t size = strlen(text) + 1;
	char *copy = malloc(size);
	if (copy) {
		memcpy(copy, text, size);
	}
	return copy;
}

int morpthis_ui_width(void) {
	return HOST_DISPLAY_WIDTH;
}

int morpthis_ui_height(void) {
	return HOST_DISPLAY_HEIGHT;
}

int morpthis_ui_full_height(void) {
	return HOST_DISPLAY_HEIGHT;
}

morpthis_screen_t *morpthis_screen_create(const char *title) {
	morpthis_screen_t *screen = calloc(1, sizeof(*screen));
	if (!screen) {
		return NULL;
	}
	screen->base.kind = HOST_WIDGET_SCREEN;
	screen->base.visible = true;
	screen->base.enabled = true;
	screen->base.width = HOST_DISPLAY_WIDTH;
	screen->base.height = HOST_DISPLAY_HEIGHT;
	screen->title = dup_text(title);
	if (!screen->title) {
		free(screen);
		return NULL;
	}
	return screen;
}

morpthis_screen_t *morpthis_screen_create_full(const char *title) {
	return morpthis_screen_create(title);
}

void morpthis_screen_set_title(morpthis_screen_t *screen, const char *title) {
	if (!screen) {
		return;
	}
	char *copy = dup_text(title);
	if (!copy) {
		return;
	}
	free(screen->title);
	screen->title = copy;
}

void morpthis_screen_load(morpthis_screen_t *screen) {
	if (!screen) {
		return;
	}
	g_active = screen;
}

morpthis_screen_t *morpthis_screen_active(void) {
	return g_active;
}

void morpthis_widget_destroy(morpthis_widget_t *widget) {
	if (!widget) {
		return;
	}

	if (widget->parent) {
		morpthis_widget_t **link = &widget->parent->child;
		while (*link) {
			if (*link == widget) {
				*link = widget->next;
				break;
			}
			link = &(*link)->next;
		}
	}

	while (widget->child) {
		morpthis_widget_destroy(widget->child);
	}

	for (int i = 0; i < widget->item_count; i++) {
		free(widget->items[i]);
	}
	free(widget->items);
	free(widget->text);
	free(widget->image);
	free(widget->value_text);
	free(widget->pixels);
	free(widget);
}

void morpthis_screen_destroy(morpthis_screen_t *screen) {
	if (!screen) {
		return;
	}
	if (g_active == screen) {
		g_active = NULL;
	}
	while (screen->base.child) {
		morpthis_widget_destroy(screen->base.child);
	}
	free(screen->title);
	free(screen);
}

static morpthis_widget_t *widget_new(morpthis_screen_t *screen, host_widget_kind_t kind) {
	if (!screen) {
		errno = EINVAL;
		return NULL;
	}
	morpthis_widget_t *widget = calloc(1, sizeof(*widget));
	if (!widget) {
		return NULL;
	}
	widget->kind = kind;
	widget->visible = true;
	widget->enabled = true;
	widget->parent = &screen->base;
	widget->next = screen->base.child;
	screen->base.child = widget;
	return widget;
}

morpthis_widget_t *morpthis_label_create(morpthis_screen_t *screen, const char *text) {
	morpthis_widget_t *widget = widget_new(screen, HOST_WIDGET_LABEL);
	if (!widget) {
		return NULL;
	}
	widget->text = dup_text(text);
	if (!widget->text) {
		morpthis_widget_destroy(widget);
		return NULL;
	}
	return widget;
}

morpthis_widget_t *morpthis_button_create(morpthis_screen_t *screen, const char *text,
										  morpthis_click_cb cb, void *user) {
	morpthis_widget_t *widget = widget_new(screen, HOST_WIDGET_BUTTON);
	if (!widget) {
		return NULL;
	}
	widget->text = dup_text(text);
	if (!widget->text) {
		morpthis_widget_destroy(widget);
		return NULL;
	}
	widget->click_cb = cb;
	widget->click_user = user;
	return widget;
}

morpthis_widget_t *morpthis_screen_add_corner_button(morpthis_screen_t *screen, morpthis_icon_t icon,
												 morpthis_click_cb cb, void *user) {
	(void)icon;
	if (!screen) {
		errno = EINVAL;
		return NULL;
	}
	morpthis_widget_t *widget = widget_new(screen, HOST_WIDGET_BUTTON);
	if (!widget) {
		return NULL;
	}
	widget->click_cb = cb;
	widget->click_user = user;
	return widget;
}

morpthis_widget_t *morpthis_icon_button_create(morpthis_screen_t *screen, morpthis_icon_t icon,
												 int size, morpthis_click_cb cb, void *user) {
	(void)size;
	return morpthis_screen_add_corner_button(screen, icon, cb, user);
}

void morpthis_widget_set_icon(morpthis_widget_t *widget, morpthis_icon_t icon) {
	(void)widget;
	(void)icon;
}

void morpthis_widget_set_text_size(morpthis_widget_t *widget, int size) {
	(void)widget;
	(void)size;
}

void morpthis_widget_set_text_dim(morpthis_widget_t *widget, bool dim) {
	(void)widget;
	(void)dim;
}

void morpthis_widget_set_text_align(morpthis_widget_t *widget, morpthis_text_align_t align) {
	(void)widget;
	(void)align;
}

morpthis_widget_t *morpthis_list_create(morpthis_screen_t *screen) {
	return widget_new(screen, HOST_WIDGET_LIST);
}

morpthis_widget_t *morpthis_image_create(morpthis_screen_t *screen, const char *relative_path) {
	morpthis_widget_t *widget = widget_new(screen, HOST_WIDGET_IMAGE);
	if (!widget) {
		return NULL;
	}
	widget->image = dup_text(relative_path);
	if (!widget->image) {
		morpthis_widget_destroy(widget);
		return NULL;
	}
	return widget;
}

void morpthis_image_set_source(morpthis_widget_t *image, const char *relative_path) {
	if (!image || image->kind != HOST_WIDGET_IMAGE) {
		return;
	}
	char *copy = dup_text(relative_path);
	if (!copy) {
		return;
	}
	free(image->image);
	image->image = copy;
}

// The host runner has no display, so there is nothing to pop a menu on. The
// app sees false and can fall back to whatever it does without one.
bool morpthis_menu_show(morpthis_widget_t *anchor, const morpthis_menu_item_t *items, int count,
						morpthis_menu_cb cb, void *user) {
	(void)anchor;
	(void)items;
	(void)count;
	(void)cb;
	(void)user;
	return false;
}

bool morpthis_menu_show_row(morpthis_widget_t *list, int row, const morpthis_menu_item_t *items,
							int count, morpthis_menu_cb cb, void *user) {
	(void)list;
	(void)row;
	(void)items;
	(void)count;
	(void)cb;
	(void)user;
	return false;
}

void morpthis_menu_close(void) {}

morpthis_widget_t *morpthis_canvas_create(morpthis_screen_t *screen, int width, int height) {
	if (width <= 0 || height <= 0) {
		errno = EINVAL;
		return NULL;
	}
	morpthis_widget_t *widget = widget_new(screen, HOST_WIDGET_CANVAS);
	if (!widget) {
		return NULL;
	}
	widget->pixels = calloc((size_t)width * (size_t)height, sizeof(uint16_t));
	if (!widget->pixels) {
		morpthis_widget_destroy(widget);
		return NULL;
	}
	widget->width = width;
	widget->height = height;
	return widget;
}

uint16_t *morpthis_canvas_buffer(morpthis_widget_t *canvas) {
	return canvas && canvas->kind == HOST_WIDGET_CANVAS ? canvas->pixels : NULL;
}

void morpthis_canvas_invalidate(morpthis_widget_t *canvas, int x, int y, int width, int height) {
	(void)canvas;
	(void)x;
	(void)y;
	(void)width;
	(void)height;
}

void morpthis_ui_refresh(void) {
	// The host's canvas is the frame the app just wrote: nothing to flush.
}

static int clamp_int(int value, int min, int max) {
	if (value < min) {
		return min;
	}
	if (value > max) {
		return max;
	}
	return value;
}

morpthis_widget_t *morpthis_slider_create(morpthis_screen_t *screen, int min, int max,
										  int value, morpthis_value_cb cb, void *user) {
	if (max < min) {
		errno = EINVAL;
		return NULL;
	}
	morpthis_widget_t *widget = widget_new(screen, HOST_WIDGET_SLIDER);
	if (!widget) {
		return NULL;
	}
	widget->min = min;
	widget->max = max;
	widget->value = clamp_int(value, min, max);
	widget->value_cb = cb;
	widget->value_user = user;
	return widget;
}

void morpthis_widget_set_text(morpthis_widget_t *widget, const char *text) {
	if (!widget) {
		return;
	}
	char *copy = dup_text(text);
	if (!copy) {
		return;
	}
	if (widget->kind == HOST_WIDGET_TEXT_INPUT) {
		free(widget->image);
		widget->image = copy;
		return;
	}
	if (widget->kind != HOST_WIDGET_LABEL && widget->kind != HOST_WIDGET_BUTTON &&
		widget->kind != HOST_WIDGET_SETTINGS_ROW) {
		free(copy);
		return;
	}
	free(widget->text);
	widget->text = copy;
}

const char *morpthis_widget_text(const morpthis_widget_t *widget) {
	if (!widget) {
		return "";
	}
	if (widget->kind == HOST_WIDGET_TEXT_INPUT) {
		return widget->image ? widget->image : "";
	}
	return widget->text ? widget->text : "";
}

void morpthis_widget_set_pos(morpthis_widget_t *widget, int x, int y) {
	if (!widget) {
		return;
	}
	widget->x = x;
	widget->y = y;
}

void morpthis_widget_set_size(morpthis_widget_t *widget, int width, int height) {
	if (!widget) {
		return;
	}
	widget->width = width == MORPTHIS_SIZE_CONTENT || width < 0 ? 0 : width;
	widget->height = height == MORPTHIS_SIZE_CONTENT || height < 0 ? 0 : height;
}

void morpthis_widget_set_visible(morpthis_widget_t *widget, bool visible) {
	if (widget) {
		widget->visible = visible;
	}
}

void morpthis_widget_set_rotation(morpthis_widget_t *widget, int degrees) {
	(void)widget;
	(void)degrees;
}

void morpthis_widget_set_opacity(morpthis_widget_t *widget, int opacity) {
	(void)widget;
	(void)opacity;
}

void morpthis_widget_raise(morpthis_widget_t *widget) {
	(void)widget;
}

void morpthis_widget_set_enabled(morpthis_widget_t *widget, bool enabled) {
	if (widget) {
		widget->enabled = enabled;
	}
}

bool morpthis_widget_is_visible(const morpthis_widget_t *widget) {
	return widget && widget->visible;
}

bool morpthis_widget_is_enabled(const morpthis_widget_t *widget) {
	return widget && widget->enabled;
}

int morpthis_list_add(morpthis_widget_t *list, const char *text) {
	if (!list || list->kind != HOST_WIDGET_LIST) {
		errno = EINVAL;
		return -1;
	}
	char *copy = dup_text(text);
	if (!copy) {
		return -1;
	}
	char **items = realloc(list->items, sizeof(*items) * (size_t)(list->item_count + 1));
	if (!items) {
		free(copy);
		return -1;
	}
	list->items = items;
	list->items[list->item_count] = copy;
	return list->item_count++;
}

int morpthis_list_add_entry(morpthis_widget_t *list, const char *title, const char *detail,
						const char *icon, bool chevron) {
	// The host has no display: it records the label a test can click. The rest
	// of the row is presentation the player owns.
	(void)icon;
	(void)chevron;
	return morpthis_list_add(list, title && title[0] ? title : (detail ? detail : ""));
}

void morpthis_list_set_playing(morpthis_widget_t *list, int index) {
	(void)list;
	(void)index;
}

void morpthis_list_clear(morpthis_widget_t *list) {
	if (!list || list->kind != HOST_WIDGET_LIST) {
		return;
	}
	for (int i = 0; i < list->item_count; i++) {
		free(list->items[i]);
	}
	free(list->items);
	list->items = NULL;
	list->item_count = 0;
}

int morpthis_list_count(const morpthis_widget_t *list) {
	return list && list->kind == HOST_WIDGET_LIST ? list->item_count : 0;
}

void morpthis_list_set_select_cb(morpthis_widget_t *list, morpthis_index_cb cb, void *user) {
	if (!list || list->kind != HOST_WIDGET_LIST) {
		return;
	}
	list->select_cb = cb;
	list->select_user = user;
}

void morpthis_slider_set_value(morpthis_widget_t *slider, int value) {
	if (!slider || slider->kind != HOST_WIDGET_SLIDER) {
		return;
	}
	slider->value = clamp_int(value, slider->min, slider->max);
	if (slider->value_cb) {
		slider->value_cb(slider, slider->value, slider->value_user);
	}
}

int morpthis_slider_value(const morpthis_widget_t *slider) {
	return slider && slider->kind == HOST_WIDGET_SLIDER ? slider->value : 0;
}

void morpthis_slider_set_release_cb(morpthis_widget_t *slider, morpthis_value_cb cb, void *user) {
	// The host runner has no drag input; there is never a release to report.
	(void)slider;
	(void)cb;
	(void)user;
}

void morpthis_toast(const char *text) {
	morpthis_logi("ui", "toast: %s", text ? text : "");
}

// ---------------------------------------------------------------------------
// Theme, chrome and dialogs
// ---------------------------------------------------------------------------

static bool g_status_bar_visible = true;

void morpthis_ui_theme(morpthis_theme_t *out) {
	if (!out) {
		return;
	}
	out->screen_bg = 0x000000;
	out->surface = 0x1e1e1e;
	out->panel = 0x111111;
	out->cover_bg = 0x202020;
	out->text_primary = 0xffffff;
	out->text_secondary = 0xaaaaaa;
	out->accent = 0x4a90d9;
	out->dark = true;
}

bool morpthis_ui_set_dark(bool dark) {
	(void)dark;
	return true;
}

bool morpthis_ui_set_accent(int index) { return index >= 0 && index < 12; }

void morpthis_ui_set_status_bar_visible(bool visible) { g_status_bar_visible = visible; }

bool morpthis_ui_status_bar_visible(void) { return g_status_bar_visible; }

bool morpthis_ui_confirm(const char *title, const char *message, const char *ok_label,
					 morpthis_action_cb cb, void *user) {
	// No display, no card. A host test that needs the accepted path calls its own
	// callback; returning false is the documented headless answer, like menus.
	(void)title;
	(void)message;
	(void)ok_label;
	(void)cb;
	(void)user;
	return false;
}

void morpthis_ui_notice(const char *title, const char *message) {
	(void)title;
	(void)message;
}

bool morpthis_ui_prompt_text(const char *title, const char *initial, bool password,
						 morpthis_text_cb cb, void *user) {
	(void)title;
	(void)initial;
	(void)password;
	(void)cb;
	(void)user;
	return false;
}

bool host_ui_click_label(const char *label) {
	if (!label || !label[0]) {
		errno = EINVAL;
		return false;
	}
	if (!g_active) {
		morpthis_logw("host", "click \"%s\": no active screen", label);
		return false;
	}
	for (morpthis_widget_t *widget = g_active->base.child; widget; widget = widget->next) {
		if (widget->kind != HOST_WIDGET_BUTTON || !widget->visible || !widget->enabled) {
			continue;
		}
		if (widget->text && strcmp(widget->text, label) == 0) {
			if (widget->click_cb) {
				widget->click_cb(widget, widget->click_user);
			}
			return true;
		}
	}
	morpthis_logw("host", "click \"%s\": no such button on \"%s\"", label,
				  g_active->title ? g_active->title : "");
	return false;
}

// ---------------------------------------------------------------------------
// API 3 widgets
// ---------------------------------------------------------------------------

morpthis_widget_t *morpthis_switch_create(morpthis_screen_t *screen, bool on,
										  morpthis_value_cb cb, void *user) {
	morpthis_widget_t *widget = widget_new(screen, HOST_WIDGET_SWITCH);
	if (!widget) {
		return NULL;
	}
	widget->value = on ? 1 : 0;
	widget->value_cb = cb;
	widget->value_user = user;
	return widget;
}

bool morpthis_switch_is_on(const morpthis_widget_t *widget) {
	return widget && widget->kind == HOST_WIDGET_SWITCH && widget->value != 0;
}

void morpthis_switch_set_on(morpthis_widget_t *widget, bool on) {
	if (widget && widget->kind == HOST_WIDGET_SWITCH) {
		widget->value = on ? 1 : 0;
	}
}

morpthis_widget_t *morpthis_progress_create(morpthis_screen_t *screen, int min, int max, int value) {
	morpthis_widget_t *widget = widget_new(screen, HOST_WIDGET_PROGRESS);
	if (!widget) {
		return NULL;
	}
	morpthis_progress_set_range(widget, min, max);
	morpthis_progress_set_value(widget, value);
	return widget;
}

void morpthis_progress_set_value(morpthis_widget_t *widget, int value) {
	if (!widget || widget->kind != HOST_WIDGET_PROGRESS) {
		return;
	}
	if (value < widget->min) {
		value = widget->min;
	}
	if (value > widget->max) {
		value = widget->max;
	}
	widget->value = value;
}

int morpthis_progress_value(const morpthis_widget_t *widget) {
	return widget && widget->kind == HOST_WIDGET_PROGRESS ? widget->value : 0;
}

void morpthis_progress_set_range(morpthis_widget_t *widget, int min, int max) {
	if (!widget || widget->kind != HOST_WIDGET_PROGRESS) {
		return;
	}
	if (max < min) {
		int swap = min;
		min = max;
		max = swap;
	}
	widget->min = min;
	widget->max = max;
	morpthis_progress_set_value(widget, widget->value);
}

morpthis_widget_t *morpthis_spinner_create(morpthis_screen_t *screen) {
	return widget_new(screen, HOST_WIDGET_SPINNER);
}

morpthis_widget_t *morpthis_container_create(morpthis_screen_t *screen) {
	return widget_new(screen, HOST_WIDGET_CONTAINER);
}

void morpthis_container_set_layout(morpthis_widget_t *container, morpthis_layout_t layout,
								   morpthis_align_t main_align, morpthis_align_t cross_align, int gap) {
	(void)container;
	(void)layout;
	(void)main_align;
	(void)cross_align;
	(void)gap;
}

void morpthis_container_set_scrollable(morpthis_widget_t *container, bool vertical) {
	(void)container;
	(void)vertical;
}

morpthis_widget_t *morpthis_segmented_create(morpthis_screen_t *screen, const char *const *labels,
										 int count, int selected, morpthis_value_cb cb, void *user) {
	if (!labels || count <= 0) {
		return NULL;
	}
	morpthis_widget_t *widget = widget_new(screen, HOST_WIDGET_SEGMENTED);
	if (!widget) {
		return NULL;
	}
	widget->items = calloc((size_t)count, sizeof(*widget->items));
	if (!widget->items) {
		morpthis_widget_destroy(widget);
		return NULL;
	}
	for (int i = 0; i < count; i++) {
		widget->items[i] = dup_text(labels[i]);
		if (!widget->items[i]) {
			morpthis_widget_destroy(widget);
			return NULL;
		}
	}
	widget->item_count = count;
	widget->value = selected >= 0 && selected < count ? selected : -1;
	widget->value_cb = cb;
	widget->value_user = user;
	return widget;
}

void morpthis_segmented_set_selected(morpthis_widget_t *widget, int index) {
	if (!widget || widget->kind != HOST_WIDGET_SEGMENTED) {
		return;
	}
	widget->value = index >= 0 && index < widget->item_count ? index : -1;
}

int morpthis_segmented_selected(const morpthis_widget_t *widget) {
	return widget && widget->kind == HOST_WIDGET_SEGMENTED ? widget->value : -1;
}

morpthis_widget_t *morpthis_settings_row_create(morpthis_screen_t *screen, const char *title,
										  const char *detail, const char *value,
										  morpthis_click_cb cb, void *user) {
	(void)detail;
	morpthis_widget_t *widget = widget_new(screen, HOST_WIDGET_SETTINGS_ROW);
	if (!widget) {
		return NULL;
	}
	widget->text = dup_text(title);
	widget->value_text = dup_text(value);
	widget->click_cb = cb;
	widget->click_user = user;
	return widget;
}

void morpthis_settings_row_set_value(morpthis_widget_t *widget, const char *value) {
	if (!widget || widget->kind != HOST_WIDGET_SETTINGS_ROW) {
		return;
	}
	char *copy = dup_text(value);
	if (!copy) {
		return;
	}
	free(widget->value_text);
	widget->value_text = copy;
}

morpthis_widget_t *morpthis_text_input_create(morpthis_screen_t *screen, const char *title,
										  const char *initial, bool password,
										  morpthis_text_cb cb, void *user) {
	morpthis_widget_t *widget = widget_new(screen, HOST_WIDGET_TEXT_INPUT);
	if (!widget) {
		return NULL;
	}
	widget->text = dup_text(title);
	widget->image = dup_text(initial);
	widget->text_cb = cb;
	widget->text_user = user;
	widget->password = password;
	return widget;
}

void morpthis_text_input_set_text(morpthis_widget_t *widget, const char *text) {
	morpthis_widget_set_text(widget, text);
}

const char *morpthis_text_input_get_text(const morpthis_widget_t *widget) {
	return widget && widget->kind == HOST_WIDGET_TEXT_INPUT && widget->image ? widget->image : "";
}

void morpthis_widget_set_parent(morpthis_widget_t *widget, morpthis_widget_t *parent) {
	if (!widget || widget == parent) {
		return;
	}
	if (widget->parent) {
		morpthis_widget_t **link = &widget->parent->child;
		while (*link && *link != widget) {
			link = &(*link)->next;
		}
		if (*link == widget) {
			*link = widget->next;
		}
	}
	widget->parent = parent ? parent : (g_active ? &g_active->base : NULL);
	widget->next = NULL;
	if (widget->parent) {
		widget->next = widget->parent->child;
		widget->parent->child = widget;
	}
}

void morpthis_widget_set_bg_color(morpthis_widget_t *widget, uint32_t rgb, int opacity) {
	(void)widget;
	(void)rgb;
	(void)opacity;
}

void morpthis_widget_set_text_color(morpthis_widget_t *widget, uint32_t rgb) {
	(void)widget;
	(void)rgb;
}

void morpthis_widget_set_radius(morpthis_widget_t *widget, int radius) {
	(void)widget;
	(void)radius;
}

void morpthis_widget_set_padding(morpthis_widget_t *widget, int left, int top, int right, int bottom) {
	(void)widget;
	(void)left;
	(void)top;
	(void)right;
	(void)bottom;
}

bool morpthis_list_scroll_to(morpthis_widget_t *list, int index) {
	return list && list->kind == HOST_WIDGET_LIST && index >= 0 && index < list->item_count;
}

void morpthis_slider_set_range(morpthis_widget_t *slider, int min, int max) {
	if (!slider || slider->kind != HOST_WIDGET_SLIDER) {
		return;
	}
	if (max < min) {
		int swap = min;
		min = max;
		max = swap;
	}
	slider->min = min;
	slider->max = max;
	if (slider->value < min) {
		slider->value = min;
	}
	if (slider->value > max) {
		slider->value = max;
	}
}

void morpthis_toast_success(const char *text) { morpthis_logi("ui", "toast success: %s", text ? text : ""); }
void morpthis_toast_error(const char *text) { morpthis_logi("ui", "toast error: %s", text ? text : ""); }
void morpthis_toast_busy(const char *text) { morpthis_logi("ui", "toast busy: %s", text ? text : ""); }
void morpthis_toast_busy_end(void) { morpthis_logi("ui", "toast busy end"); }

void morpthis_ui_modal_show(const char *title, const char *subtitle) {
	morpthis_logi("ui", "modal: %s — %s", title ? title : "", subtitle ? subtitle : "");
}

void morpthis_ui_modal_progress(int percent) { (void)percent; }
void morpthis_ui_modal_hide(void) {}

morpthis_widget_t *morpthis_qrcode_create(morpthis_screen_t *screen, int size) {
	morpthis_widget_t *widget = widget_new(screen, HOST_WIDGET_QRCODE);
	if (widget) {
		widget->width = size > 0 ? size : 0;
		widget->height = size > 0 ? size : 0;
	}
	return widget;
}

void morpthis_qrcode_set_text(morpthis_widget_t *qrcode, const char *text) {
	if (!qrcode || qrcode->kind != HOST_WIDGET_QRCODE) {
		return;
	}
	char *copy = dup_text(text);
	if (!copy) {
		return;
	}
	free(qrcode->text);
	qrcode->text = copy;
}

morpthis_widget_t *morpthis_roller_create(morpthis_screen_t *screen, const char *const *options,
										  int count, int selected, int visible_rows,
										  morpthis_value_cb cb, void *user) {
	(void)visible_rows;
	if (!options || count <= 0) {
		return NULL;
	}
	morpthis_widget_t *widget = widget_new(screen, HOST_WIDGET_ROLLER);
	if (!widget) {
		return NULL;
	}
	widget->items = calloc((size_t)count, sizeof(*widget->items));
	if (!widget->items) {
		morpthis_widget_destroy(widget);
		return NULL;
	}
	for (int i = 0; i < count; i++) {
		widget->items[i] = dup_text(options[i]);
		if (!widget->items[i]) {
			morpthis_widget_destroy(widget);
			return NULL;
		}
	}
	widget->item_count = count;
	widget->value = selected >= 0 && selected < count ? selected : 0;
	widget->value_cb = cb;
	widget->value_user = user;
	return widget;
}

void morpthis_roller_set_selected(morpthis_widget_t *roller, int index) {
	if (!roller || roller->kind != HOST_WIDGET_ROLLER) {
		return;
	}
	if (index < 0) {
		index = 0;
	}
	if (index >= roller->item_count) {
		index = roller->item_count > 0 ? roller->item_count - 1 : 0;
	}
	roller->value = index;
}

int morpthis_roller_selected(const morpthis_widget_t *roller) {
	return roller && roller->kind == HOST_WIDGET_ROLLER ? roller->value : 0;
}

void morpthis_widget_set_text_wrap(morpthis_widget_t *widget, bool wrap) {
	(void)widget;
	(void)wrap;
}

void morpthis_widget_set_text_scroll(morpthis_widget_t *widget, bool scroll) {
	if (widget) {
		widget->text_scroll = scroll;
	}
}

morpthis_widget_t *morpthis_arc_create(morpthis_screen_t *screen, int min, int max, int value,
									   morpthis_value_cb cb, void *user) {
	morpthis_widget_t *widget = widget_new(screen, HOST_WIDGET_ARC);
	if (!widget) {
		return NULL;
	}
	widget->value_cb = cb;
	widget->value_user = user;
	morpthis_arc_set_range(widget, min, max);
	morpthis_arc_set_value(widget, value);
	return widget;
}

void morpthis_arc_set_value(morpthis_widget_t *widget, int value) {
	if (!widget || widget->kind != HOST_WIDGET_ARC) {
		return;
	}
	if (value < widget->min) {
		value = widget->min;
	}
	if (value > widget->max) {
		value = widget->max;
	}
	widget->value = value;
}

int morpthis_arc_value(const morpthis_widget_t *widget) {
	return widget && widget->kind == HOST_WIDGET_ARC ? widget->value : 0;
}

void morpthis_arc_set_range(morpthis_widget_t *widget, int min, int max) {
	if (!widget || widget->kind != HOST_WIDGET_ARC) {
		return;
	}
	if (max < min) {
		int swap = min;
		min = max;
		max = swap;
	}
	widget->min = min;
	widget->max = max;
	if (widget->value < min) {
		widget->value = min;
	}
	if (widget->value > max) {
		widget->value = max;
	}
}
