#include <morpthis/morpthis.h>

#include <stdio.h>

MORPTHIS_DEFINE_API_VERSION();

static void exit_clicked(morpthis_widget_t *widget, void *user) {
	(void)widget;
	(void)user;
	morpthis_logi("hello", "exit");
	morpthis_app_quit();
}

int morpthis_app_main(const morpthis_app_t *app) {
	morpthis_logi("hello", "started, api %d", morpthis_app_api_version(app));

	morpthis_screen_t *screen = morpthis_screen_create("Hello");
	if (!screen) {
		morpthis_loge("hello", "could not create the screen");
		return 1;
	}

	morpthis_widget_t *title = morpthis_label_create(screen, "Hello, Morpthis");
	morpthis_widget_set_pos(title, 20, 60);
	morpthis_widget_set_size(title, 200, 32);

	char details[96];
	snprintf(details, sizeof(details), "API %d, %dx%d, %u KB free",
			 morpthis_app_api_version(app), morpthis_ui_width(), morpthis_ui_height(),
			 (unsigned)(morpthis_app_mem_available() / 1024));
	morpthis_widget_t *info = morpthis_label_create(screen, details);
	morpthis_widget_set_pos(info, 20, 104);
	morpthis_widget_set_size(info, 200, 24);

	morpthis_widget_t *button = morpthis_button_create(screen, "Exit", exit_clicked, NULL);
	morpthis_widget_set_pos(button, 60, 180);
	morpthis_widget_set_size(button, 120, 44);

	morpthis_screen_load(screen);
	return 0;
}
