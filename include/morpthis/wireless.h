#ifndef MORPTHIS_WIRELESS_H
#define MORPTHIS_WIRELESS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Wi-Fi and Bluetooth, driven by the player's own workers.
//
// Nothing here blocks the caller: every command is queued to the module's
// worker and the answer appears in the status calls. That is the same contract
// the local Wireless pages use, so an app can offer the same controls without
// starting a second supplicant or Bluetooth stack.
//
// The radio modules may be absent (host build, a firmware without the driver).
// available() says so and the rest does nothing.

// ---------------------------------------------------------------------------
// Wi-Fi
// ---------------------------------------------------------------------------

#define MORPTHIS_WIFI_SSID_MAX 64
#define MORPTHIS_WIFI_MAX_NETWORKS 40

typedef enum {
	MORPTHIS_WIFI_OFF = 0,
	MORPTHIS_WIFI_ON, // radio up, associated with nothing
	MORPTHIS_WIFI_CONNECTING,
	MORPTHIS_WIFI_CONNECTED,
} morpthis_wifi_state_t;

typedef struct {
	char ssid[MORPTHIS_WIFI_SSID_MAX];
	int signal; // dBm, 0 when unknown
	int bars;	// 0..3
	bool secured;
	bool saved;
	bool current;
	int id; // wpa_supplicant network id, -1 when not saved
} morpthis_wifi_network_t;

typedef struct {
	morpthis_wifi_state_t state;
	char ssid[MORPTHIS_WIFI_SSID_MAX];
	char ip[40];
	char mac[24];
	int signal;
	int bars;
} morpthis_wifi_status_t;

typedef enum {
	MORPTHIS_WIFI_OP_IDLE = 0,
	MORPTHIS_WIFI_OP_BUSY,
	MORPTHIS_WIFI_OP_OK,
	MORPTHIS_WIFI_OP_FAILED,
} morpthis_wifi_op_t;

typedef enum {
	MORPTHIS_WIFI_OPKIND_CONNECT = 0,
	MORPTHIS_WIFI_OPKIND_DISCONNECT,
	MORPTHIS_WIFI_OPKIND_FORGET,
} morpthis_wifi_opkind_t;

typedef enum {
	MORPTHIS_WIFI_FAIL_NONE = 0,
	MORPTHIS_WIFI_FAIL_OTHER,
	MORPTHIS_WIFI_FAIL_WRONG_KEY,
	MORPTHIS_WIFI_FAIL_REFUSED,
} morpthis_wifi_fail_t;

bool morpthis_wifi_available(void);
bool morpthis_wifi_enabled(void);
void morpthis_wifi_set_enabled(bool on);
bool morpthis_wifi_busy(void);

// Snapshot of the live link. Never blocks. connected() is the short answer for
// "may the app use the network now": associated and with an address.
void morpthis_wifi_status(morpthis_wifi_status_t *out);
bool morpthis_wifi_connected(void);

void morpthis_wifi_scan(void);
bool morpthis_wifi_scan_running(void);
unsigned morpthis_wifi_networks_serial(void); // moves when the list changes
int morpthis_wifi_networks(morpthis_wifi_network_t *out, int max);

// `psk` may be NULL or empty for an open network. The result is reported once
// through morpthis_wifi_take_result().
void morpthis_wifi_connect(const char *ssid, const char *psk);
void morpthis_wifi_connect_hidden(const char *ssid, const char *psk);
void morpthis_wifi_disconnect(void);
void morpthis_wifi_forget(const char *ssid);

// Consumes the last result. Returns MORPTHIS_WIFI_OP_IDLE when there is
// nothing to report. `ssid_out` may be NULL.
morpthis_wifi_op_t morpthis_wifi_take_result(char *ssid_out, size_t size);
morpthis_wifi_fail_t morpthis_wifi_last_failure(void);
morpthis_wifi_opkind_t morpthis_wifi_last_op_kind(void);

// ---------------------------------------------------------------------------
// Bluetooth
// ---------------------------------------------------------------------------

#define MORPTHIS_BT_MAX_DEVICES 24
#define MORPTHIS_BT_NAME_MAX 64
#define MORPTHIS_BT_MAC_MAX 20

typedef enum {
	MORPTHIS_BT_OFF = 0,
	MORPTHIS_BT_ON,
	MORPTHIS_BT_CONNECTED,
} morpthis_bt_state_t;

typedef struct {
	char mac[MORPTHIS_BT_MAC_MAX];
	char name[MORPTHIS_BT_NAME_MAX];
	bool paired;
	bool connected;
	bool trusted;
	int rssi; // scan results only, 0 when unknown
} morpthis_bt_device_t;

typedef enum {
	MORPTHIS_BT_OP_IDLE = 0,
	MORPTHIS_BT_OP_BUSY,
	MORPTHIS_BT_OP_OK,
	MORPTHIS_BT_OP_PAIRED,
	MORPTHIS_BT_OP_FAILED,
} morpthis_bt_op_t;

bool morpthis_bluetooth_available(void);
bool morpthis_bluetooth_enabled(void);
void morpthis_bluetooth_set_enabled(bool on);
bool morpthis_bluetooth_busy(void);
morpthis_bt_state_t morpthis_bluetooth_state(void);

const char *morpthis_bluetooth_local_name(void);
bool morpthis_bluetooth_set_local_name(const char *name);

unsigned morpthis_bluetooth_devices_serial(void);
int morpthis_bluetooth_paired(morpthis_bt_device_t *out, int max);
int morpthis_bluetooth_found(morpthis_bt_device_t *out, int max);
bool morpthis_bluetooth_connected_device(morpthis_bt_device_t *out);

// True while the player's audio is going to Bluetooth headphones.
bool morpthis_bluetooth_audio_active(void);

void morpthis_bluetooth_scan_start(void);
void morpthis_bluetooth_scan_stop(void);
bool morpthis_bluetooth_scan_running(void);
void morpthis_bluetooth_refresh(void);

void morpthis_bluetooth_pair(const char *mac);
void morpthis_bluetooth_connect(const char *mac);
void morpthis_bluetooth_disconnect(const char *mac);
void morpthis_bluetooth_forget(const char *mac);

// Consumes the last result. `name_out` may be NULL.
morpthis_bt_op_t morpthis_bluetooth_take_result(char *name_out, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* MORPTHIS_WIRELESS_H */
