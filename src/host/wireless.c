#include "internal.h"

#include <string.h>

// The host has no radios. Everything reports absent, and the commands are
// accepted so an app's code path does not have to special-case CI.

bool morpthis_wifi_available(void) { return false; }
bool morpthis_wifi_enabled(void) { return false; }
void morpthis_wifi_set_enabled(bool on) { (void)on; }
bool morpthis_wifi_busy(void) { return false; }

void morpthis_wifi_status(morpthis_wifi_status_t *out) {
	if (out) {
		memset(out, 0, sizeof(*out));
		out->state = MORPTHIS_WIFI_OFF;
	}
}

bool morpthis_wifi_connected(void) { return false; }
void morpthis_wifi_scan(void) {}
bool morpthis_wifi_scan_running(void) { return false; }
unsigned morpthis_wifi_networks_serial(void) { return 0; }

int morpthis_wifi_networks(morpthis_wifi_network_t *out, int max) {
	(void)out;
	(void)max;
	return 0;
}

void morpthis_wifi_connect(const char *ssid, const char *psk) {
	(void)ssid;
	(void)psk;
}

void morpthis_wifi_connect_hidden(const char *ssid, const char *psk) {
	(void)ssid;
	(void)psk;
}

void morpthis_wifi_disconnect(void) {}
void morpthis_wifi_forget(const char *ssid) { (void)ssid; }

morpthis_wifi_op_t morpthis_wifi_take_result(char *ssid_out, size_t size) {
	if (ssid_out && size) {
		ssid_out[0] = '\0';
	}
	return MORPTHIS_WIFI_OP_IDLE;
}

morpthis_wifi_fail_t morpthis_wifi_last_failure(void) { return MORPTHIS_WIFI_FAIL_NONE; }
morpthis_wifi_opkind_t morpthis_wifi_last_op_kind(void) { return MORPTHIS_WIFI_OPKIND_CONNECT; }

bool morpthis_bluetooth_available(void) { return false; }
bool morpthis_bluetooth_enabled(void) { return false; }
void morpthis_bluetooth_set_enabled(bool on) { (void)on; }
bool morpthis_bluetooth_busy(void) { return false; }
morpthis_bt_state_t morpthis_bluetooth_state(void) { return MORPTHIS_BT_OFF; }
const char *morpthis_bluetooth_local_name(void) { return ""; }

bool morpthis_bluetooth_set_local_name(const char *name) {
	(void)name;
	return false;
}

unsigned morpthis_bluetooth_devices_serial(void) { return 0; }

int morpthis_bluetooth_paired(morpthis_bt_device_t *out, int max) {
	(void)out;
	(void)max;
	return 0;
}

int morpthis_bluetooth_found(morpthis_bt_device_t *out, int max) {
	(void)out;
	(void)max;
	return 0;
}

bool morpthis_bluetooth_connected_device(morpthis_bt_device_t *out) {
	if (out) {
		memset(out, 0, sizeof(*out));
	}
	return false;
}

bool morpthis_bluetooth_audio_active(void) { return false; }
void morpthis_bluetooth_scan_start(void) {}
void morpthis_bluetooth_scan_stop(void) {}
bool morpthis_bluetooth_scan_running(void) { return false; }
void morpthis_bluetooth_refresh(void) {}
void morpthis_bluetooth_pair(const char *mac) { (void)mac; }
void morpthis_bluetooth_connect(const char *mac) { (void)mac; }
void morpthis_bluetooth_disconnect(const char *mac) { (void)mac; }
void morpthis_bluetooth_forget(const char *mac) { (void)mac; }

morpthis_bt_op_t morpthis_bluetooth_take_result(char *name_out, size_t size) {
	if (name_out && size) {
		name_out[0] = '\0';
	}
	return MORPTHIS_BT_OP_IDLE;
}
