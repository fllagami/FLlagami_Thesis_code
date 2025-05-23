#ifndef   NUS_HANDLER_H
#define NUS_HANDLER_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "nrf.h"
#include "app_error.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "ble_link_ctx_manager.h"
#include "ble_nus2.h"


void ble_evt_handler2(ble_evt_t const * p_ble_evt, void * p_context);
void idle_state_handle(void);

// Initializing function
void timers_init(void);
ret_code_t power_management_init(void);
ret_code_t ble_stack_init(void);
ret_code_t gatt_init(void);
ret_code_t services_init(void);
ret_code_t advertising_init(void);
ret_code_t conn_params_init(void);
//ret_code_t adv_start(bool erase_bonds);
ret_code_t gap_params_init(void);
ret_code_t peer_manager_init(void);
ret_code_t scan_init(void);
ret_code_t db_discovery_init(void);
ret_code_t nus_c_init(void);
void register_vendor_uuid(void);
uint32_t sys_timer_start();
//void rtc_init(void);

void stHandles(void);
//void change_adv_tx(int8_t power);
//void stop_adv(void);
//void send_nus_data_c(char *format, ...);
//ret_code_t change_conn_tx(int8_t power, int8_t handle);
//void start_rssi(uint16_t handle);
//void restart_peri_relay_info(void);
//void error_from_usb(char* type, int len);
//void disconnect_handle(uint16_t handle);
extern int not_sent[30];
extern char all_received[1500];
//int send_tlv_from_c(int h, bool rssi, uint8_t* tlv, uint16_t length, char* type);
//void send_all_rec_usb(void);
//extern bool more_data_left;
//extern int max_chunk;

//extern int send_missing_request_counter;
//extern uint8_t last_received_by_cent;


ret_code_t scan_on_interval_start(void);
void scan_on_interval_stop(void);

//void reset_device(void);


//extern bool start_send_data_not_received;




#endif // NUS_HANDLER_H