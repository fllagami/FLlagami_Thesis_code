#ifndef   DEVICE_CONN_PARAMS_H
#define DEVICE_CONN_PARAMS_H
#include "sdk_config.h"
#include "ble_gap.h"
#include "ble_nus_c_common_types.h"
#include <stdbool.h>

typedef struct {
    uint16_t min_interval;                //1.25 ms units, see @ref BLE_GAP_CP_LIMITS.
    uint16_t max_interval;               //1.25 ms units, see @ref BLE_GAP_CP_LIMITS.
    uint16_t slave_latency;
    uint16_t timeout;                   //10 ms units
} conn_params_t;

extern bool incl_sink_conns;
extern bool incl_min_cel_buffer;
extern int event_len_buffer;
extern bool no_cel_buffer;
extern bool same_cel_buffer;

extern ble_gap_conn_params_t conn_params_BAD;
extern ble_gap_conn_params_t conn_params_test_rssi;
extern ble_gap_conn_params_t conn_params_periodic;
extern ble_gap_conn_params_t conn_params_idle;
extern ble_gap_conn_params_t conn_params_stored;
extern ble_gap_conn_params_t conn_params_maintain_relay;
extern ble_gap_conn_params_t conn_params_phone;
extern ble_gap_conn_params_t conn_params_conn_phone_wait;
extern ble_gap_conn_params_t conn_params_phone;
extern ble_gap_conn_params_t conn_params_phone_init;
extern ble_gap_conn_params_t conn_params_on_gather_type1;
extern ble_gap_conn_params_t conn_params_on_gather_type1_sink;
extern ble_gap_conn_params_t conn_params_on_gather_type2;
extern ble_gap_conn_params_t conn_params_on_gather_type3;
extern ble_gap_conn_params_t conn_params_on_gather_sink;
extern ble_gap_conn_params_t conn_params_relay_default;

extern ble_gap_conn_params_t power_saving_conn_params;
extern ble_gap_conn_params_t periodic_conn_params;
extern ble_gap_conn_params_t phone_default_conn_params;
extern ble_gap_conn_params_t initial_conn_params;
extern ble_gap_conn_params_t on_request_power_saving_conn_params;
extern ble_gap_conn_params_t on_request_conn_params;
extern ble_gap_conn_params_t peri_request_to_phone_conn_params;
extern ble_gap_conn_params_t phone_scan_for_peris_conn_params;
extern ble_gap_conn_params_t phone_with_request_conn_params;
extern ble_gap_conn_params_t phone_no_request_conn_params;

void get_desired_conn_params(ble_gap_conn_params_t* temp, int min, int max, int sl, int tout);
void copy_conn_params(ble_gap_conn_params_t* temp, ble_gap_conn_params_t* temp2);
void copy_conn_params2(ble_gap_conn_params_t* temp, ble_gap_conn_params_t* temp2);
bool compare_conn_params(ble_gap_conn_params_t* temp, ble_gap_conn_params_t* temp2);
void calculate_on_request_params(uint16_t num_req);
void calculate_peri_to_relay_on_request_params(uint16_t num_req);
bool check_update(int h);
void calculate_relay_params();
void calculate_t1_sink_params(uint16_t min_freq, int min_buffer, int max_buffer);

extern uint16_t conn_update_timer_handle;
void clear_updates_for_handle(int handle);
bool update_conn_params_send_tlv(uint16_t handle, ble_gap_conn_params_t params, uint8_t * tlv, int len, bool force);
bool update_all_relay_conn_tlv(uint8_t * tlv, int len);
bool update_conn_params(uint16_t handle, ble_gap_conn_params_t params);
bool update_all_conn_for_gather_t23(void);
void calibrate_conn_params(uint16_t handle_not_to_cal, uint16_t extra_not_to_call);
void not_used_devices_conn_update();
void reset_not_used_devices_conn_update();

extern bool updating_onreq_conn;
void update_onreq_conn(int lastToUpdate);
ret_code_t change_gap_params(bool forPhone);

void set_periodic_conn_params(int h);
void set_own_periodic_conn_params(void);
void update_type_periodic_conn_params(uint16_t handle);

extern bool updating_ongather_conn;
bool update_all_conn_for_gather(void);
extern bool updating_ongather_conn_t23;
void set_conn_time_buffers(bool use_min_cel_buffer, bool no_min_max_buffers, bool both_buffers_max, int buffer_size);
#endif // DEVICE_CONN_PARAMS_H