#ifndef HRS_HANDLER_H
#define HRS_HANDLER_H

#include <stdint.h>
#include <stdbool.h>
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_bas.h"
#include "ble_hrs.h"
#include "ble_dis.h"
#include "ble_conn_params.h"
#include "bsp.h"
#include "nrf_sdh_ble.h"
#include "app_timer.h"
#include "peer_manager.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_lesc.h"
#include "nrf_ble_qwr.h"
#include "common.h"
#include "app_timer.h"

// Function declarations
void ble_stack_init(void);
void timers_init(void);
void gap_params_init(void);
void gatt_init(void);
void services_init(void);
void sensor_simulator_init(void);
void conn_params_init(void);
void peer_manager_init(void);
void advertising_init(void);
void buttons_leds_init(bool * p_erase_bonds);
void log_init(void);
void power_management_init(void);

void application_timers_start(void);
void advertising_start(bool erase_bonds);

void bsp_event_handler(bsp_event_t event);
void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context);
void pm_evt_handler(pm_evt_t const * p_evt);
void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt);
void nrf_qwr_error_handler(uint32_t nrf_error);
void battery_level_update(void);
void battery_level_meas_timeout_handler(void * p_context);
void heart_rate_meas_timeout_handler(void * p_context);
void rr_interval_timeout_handler(void * p_context);
void sensor_contact_detected_timeout_handler(void * p_context);
void delete_bonds(void);
void idle_state_handle(void);

// Define your external variables
//extern ble_hrs_t m_hrs; // Declare external variables
//extern ble_bas_t m_bas;
//extern nrf_ble_gatt_t m_gatt;
//extern nrf_ble_qwr_t m_qwr;
//extern ble_advertising_t m_advertising;
//extern app_timer_id_t m_battery_timer_id;
//extern app_timer_id_t m_heart_rate_timer_id;
//extern app_timer_id_t m_rr_interval_timer_id;
//extern app_timer_id_t m_sensor_contact_timer_id;
#endif // HRS_HANDLER_H