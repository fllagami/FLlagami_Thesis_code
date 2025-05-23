#ifndef   DEVICE_TP_H
#define DEVICE_TP_H
#include "sdk_config.h"
#include "ble_gap.h"
#include "ble_nus_c_common_types.h"
#include <stdbool.h>

// Maximum tx power index in connections
extern int8_t tx_range[9];
extern float ema_coefficient;

extern int min_tx_to_peri;
extern int min_tx_to_cent;
extern int max_tx_to_peri;
extern int max_tx_to_cent;

extern int max_tx_idx_own;
extern int min_tx_idx_own;
extern int default_tx_idx;
extern int RSSI_starting_limit;

void do_tpc_pdr(uint16_t h);
void do_tpc_rssi(uint16_t h, int8_t new_rssi);
void reset_pdr(uint16_t h);
void reset_tpc(uint16_t h);

void instruct_tp_change(int h, int i);
void reset_pdr_timer(int h);
void change_rssi_margins_peri(int min, int max, int ideal);
void change_rssi_margins_cent(int min, int max, int ideal);
void do_rssi_ema2(uint16_t h, int8_t new_rssi);
void do_rssi_ema(uint16_t h, int new_rssi);
void do_rssi_ema_self(uint16_t h, int8_t new_rssi);

void set_max_tx(int to_peri, int to_cent);
void set_max_tx_specific(int handle, int new_max);

uint32_t change_conn_tx(int8_t power, int8_t handle, int8_t ema, int8_t rssi);

void change_rssi_margins_peri(int min, int max, int ideal);
void change_rssi_margins_cent(int min, int max, int ideal);
float change_ema_coefficient_window(int new_windowSize);
void change_ema_coefficient_window_specific(int h, int new_windowSize);
#endif // DEVICE_TP_H