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
	if (!widget || (widget->kind != HOST_WIDGET_LABEL && widget->kind != HOST_WIDGET_BUTTON)) {
		return;
	}
	char *copy = dup_text(text);
	if (!copy) {
		return;
	}
	free(widget->text);
	widget->text = copy;
}

const char *morpthis_widget_text(const morpthis_widget_t *widget) {
	return widget && widget->text ? widget->text : "";
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
	widget->width = width < 0 ? 0 : width;
	widget->height = height < 0 ? 0 : height;
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

void morpthis_toast(const char *text) {
	morpthis_logi("ui", "toast: %s", text ? text : "");
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
