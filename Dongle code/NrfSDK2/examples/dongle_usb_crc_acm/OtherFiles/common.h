#ifndef COMMON_H
#define COMMON_H
#include "sdk_config.h"
#include <stdbool.h>
#include <stdint.h>
// Define your constants here
//#define LED_USB_RESUME      (BSP_BOARD_LED_0)
//#define LED_CDC_ACM_OPEN    (BSP_BOARD_LED_1)
//#define LED_CDC_ACM_RX      (BSP_BOARD_LED_2)
//#define LED_CDC_ACM_TX      (BSP_BOARD_LED_3)

#define led_green           (BSP_BOARD_LED_0)
#define led_RED             (BSP_BOARD_LED_1)
#define led_GREEN           (BSP_BOARD_LED_2)
#define led_BLUE            (BSP_BOARD_LED_3)

//#define LED_CDC_ACM_OPEN    (BSP_BOARD_LED_1)
//#define LED_CDC_ACM_RX      (BSP_BOARD_LED_2)
//#define LED_CDC_ACM_TX      (BSP_BOARD_LED_3)
//#define PERIPHERAL_ADVERTISING_LED      BSP_BOARD_LED_2
//#define PERIPHERAL_CONNECTED_LED        BSP_BOARD_LED_3
//#define CENTRAL_SCANNING_LED            BSP_BOARD_LED_0
//#define CENTRAL_CONNECTED_LED           BSP_BOARD_LED_1

// Button
#define BTN_CDC_DATA_SEND       0
#define BTN_CDC_NOTIFY_SEND     1

#define BTN_CDC_DATA_KEY_RELEASE        (bsp_event_t)(BSP_EVENT_KEY_LAST + 1)

// USB
/**
 * @brief Enable power USB detection
 *
 * Configure if example supports USB port connection
 */
#ifndef USBD_POWER_DETECTION
#define USBD_POWER_DETECTION true
#endif

#define CDC_ACM_COMM_INTERFACE  0
#define CDC_ACM_COMM_EPIN       NRF_DRV_USBD_EPIN2

#define CDC_ACM_DATA_INTERFACE  1
#define CDC_ACM_DATA_EPIN       NRF_DRV_USBD_EPIN1
#define CDC_ACM_DATA_EPOUT      NRF_DRV_USBD_EPOUT1

#define READ_SIZE 64
#define MAX_FILE_SIZE 40000//81000 //255 chunks * 244 size + 255 for lengths = 62475
#define MessageStorageSize 20000
#define DO_TLV 1

// BLE
#define MANUFACTURER_NAME                   "NordicSemiconductor"                   /**< Manufacturer. Will be passed to Device Information Service. */

#define APP_ADV_DURATION                    24000                                   /**< The advertising duration (180 seconds) in units of 10 milliseconds. */
#define APP_BLE_OBSERVER_PRIO           3                                           /**< Application's BLE observer priority. You shouldn't need to modify this value. */

#define APP_BLE_CONN_CFG_TAG                1                                       /**< A tag identifying the SoftDevice BLE configuration. */

//#define BATTERY_LEVEL_MEAS_INTERVAL         APP_TIMER_TICKS(2000)                   /**< Battery level measurement interval (ticks). */
//#define MIN_BATTERY_LEVEL                   81                                      /**< Minimum simulated battery level. */
//#define MAX_BATTERY_LEVEL                   100                                     /**< Maximum simulated 7battery level. */
//#define BATTERY_LEVEL_INCREMENT             1                                       /**< Increment between each simulated battery level measurement. */

//#define HEART_RATE_MEAS_INTERVAL            APP_TIMER_TICKS(1000)                   /**< Heart rate measurement interval (ticks). */
//#define MIN_HEART_RATE                      140                                     /**< Minimum heart rate as returned by the simulated measurement function. */
//#define MAX_HEART_RATE                      300                                     /**< Maximum heart rate as returned by the simulated measurement function. */
//#define HEART_RATE_INCREMENT                10                                      /**< Value by which the heart rate is incremented/decremented for each call to the simulated measurement function. */

//#define RR_INTERVAL_INTERVAL                APP_TIMER_TICKS(300)                    /**< RR interval interval (ticks). */
//#define MIN_RR_INTERVAL                     100                                     /**< Minimum RR interval as returned by the simulated measurement function. */
//#define MAX_RR_INTERVAL                     500                                     /**< Maximum RR interval as returned by the simulated measurement function. */
//#define RR_INTERVAL_INCREMENT               1                                       /**< Value by which the RR interval is incremented/decremented for each call to the simulated measurement function. */

//#define SENSOR_CONTACT_DETECTED_INTERVAL    APP_TIMER_TICKS(5000)                   /**< Sensor Contact Detected toggle interval (ticks). */

//#define MIN_CONN_INTERVAL                   MSEC_TO_UNITS(200, UNIT_1_25_MS)        /**< Minimum acceptable connection interval (0.4 seconds). */
//#define MAX_CONN_INTERVAL                   MSEC_TO_UNITS(300, UNIT_1_25_MS)        /**< Maximum acceptable connection interval (0.65 second). */
//#define SLAVE_LATENCY                       0                                       /**< Slave latency. */
//#define CONN_SUP_TIMEOUT                    MSEC_TO_UNITS(4000, UNIT_10_MS)         /**< Connection supervisory timeout (4 seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY      APP_TIMER_TICKS(50000000)                   /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY       APP_TIMER_TICKS(90000000)//default 30000                  /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT        5                                       /**< Number of attempts before giving up the connection parameter negotiation. */

#define LESC_DEBUG_MODE                     0                                       /**< Set to 1 to use LESC debug keys, allows you to use a sniffer to inspect traffic. */

#define SEC_PARAM_BOND                      1                                       /**< Perform bonding. */
#define SEC_PARAM_MITM                      0                                       /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC                      1                                       /**< LE Secure Connections enabled. */
#define SEC_PARAM_KEYPRESS                  0                                       /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES           BLE_GAP_IO_CAPS_NONE                    /**< No I/O capabilities. */
#define SEC_PARAM_OOB                       0                                       /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE              7                                       /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE              16                                      /**< Maximum encryption key size. */

#define DEAD_BEEF                           0xDEADBEEF                              /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */
#define UART_TX_BUF_SIZE                256                                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE                256                                         /**< UART RX buffer size. */

#define OPCODE_LENGTH        1
#define HANDLE_LENGTH        2

/**@brief   Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */
#if defined(NRF_SDH_BLE_GATT_MAX_MTU_SIZE) && (NRF_SDH_BLE_GATT_MAX_MTU_SIZE != 0)
    #define BLE_NUS_MAX_DATA_LEN (NRF_SDH_BLE_GATT_MAX_MTU_SIZE - OPCODE_LENGTH - HANDLE_LENGTH)//-3)
#else
    #define BLE_NUS_MAX_DATA_LEN (BLE_GATT_MTU_SIZE_DEFAULT - OPCODE_LENGTH - HANDLE_LENGTH)
    #warning NRF_SDH_BLE_GATT_MAX_MTU_SIZE is not defined.
#endif

#define NUS_BASE_UUID                   {{0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x00, 0x00, 0x40, 0x6E}} /**< Used vendor-specific UUID. */

#define BLE_UUID_NUS_SERVICE            0x0001                      /**< The UUID of the Nordic UART Service. */
#define BLE_UUID_NUS_RX_CHARACTERISTIC  0x0002                      /**< The UUID of the RX Characteristic. */
#define BLE_UUID_NUS_TX_CHARACTERISTIC  0x0003                      /**< The UUID of the TX Characteristic. */

#define BLE_NUS_MAX_RX_CHAR_LEN        BLE_NUS_MAX_DATA_LEN /**< Maximum length of the RX Characteristic (in bytes). */
#define BLE_NUS_MAX_TX_CHAR_LEN        BLE_NUS_MAX_DATA_LEN /**< Maximum length of the TX Characteristic (in bytes). */
#define MAX_CHUNK_SIZE BLE_NUS_MAX_DATA_LEN

#define LOG_FILE_ID 0x5555  // File ID for log data
#define LOG_RECORD_KEY 0x6666  // Record key for log data

//#define SCAN_TIMER_INTERVAL                 400
#define APP_ADV_INTERVAL                    40                                     /**< The advertising interval (in units of 0.625 ms. This value corresponds to 187.5 ms). */
#define MAX_PACKAGE_SIZE                    244   // Max size of useful data in 1 package, 1 less than max 244
#define PHONE_ID                                'P'

ret_code_t scan_quick(void);
extern int og_stored_bytes;
extern bool og_signal_sent;
// Queue tlv data to centrals
extern int connecting_handle;
typedef struct {
    uint8_t data[244];
    uint16_t length;
    uint16_t handle;
}package_storage_t;
extern bool no_scan;
extern int package_own_counter;
extern int package_head;
extern int package_tail;
extern int package_tail_periodic;
extern int package_count;
extern bool package_same_idx_is_ok;
extern int package_max_size;
extern int package_storage_size;
extern package_storage_t package_storage[];
extern bool allAdvAsked;
extern bool relay_sent_peris;
extern int adv_phoning;
extern int adv_ph_time;
extern bool doStar;
// Count of queued messages to peripherals
extern int peri_tlv_count;
extern bool start_fast_relay_ready;
extern bool connToRpi;
// General common functions
uint32_t scan_start(void);
void scan_stop(void);

uint32_t adv_start(bool erase_bonds);
void stop_adv(void);
void change_adv_tx(int8_t power);

void send_usb_data(bool time, const char *format, ...);
void check_error(const char* type, int err);
void request_usb_data(void);

void send_tlv_usb(bool time, char* type, uint8_t *tlv, int last_num_idx, int last_idx, int handle);
void save_tlv_as_string(uint8_t *tlv, uint16_t received_len);
void send_file_usb(void);
char* format_string(const char *format, ...);

uint32_t disconnect_handle(uint16_t handle);
void start_rssi(uint16_t handle);

uint32_t periodic_timer_start();
uint32_t periodic_timer_stop();

uint32_t conn_update_timer_start(int time);
//char* get_time();

// For resetting
void reset_data_gather_ble_nus();
void reset_on_req_queue();

// Functions for sending as a central
int send_tlv_from_c(int h, uint8_t* tlv, uint16_t length, char* type);
void send_tlv_to_peripherals(void);
void send_tlv_to_cent(void);

// Functions for sending as a peripheral
void send_stored_tlv(void);
void send_chunk_tlv(void);//(ble_nus_t *p_nus, uint16_t conn_handle);
//void send_specific_tlv(void);//(ble_nus_t * p_nus, uint16_t conn_handle);
void send_relay_tlv(void);//(ble_nus_t *p_nus, uint16_t conn_handle);
void send_periodic_tlv(void);//(ble_nus_t *p_nus, uint16_t conn_handle);
int send_tlv_from_p(int16_t h, uint8_t* tlv, uint16_t length);
void add_periodic_bytes(int new_bytes);
void add_other_device_periodic_bytes_at_start(char id, int new_bytes);
// Flags and helper variables
extern int PERIODIC_TIMER_INTERVAL;
extern bool start_scan_wait_conn_param_update;
extern uint16_t scan_wait_handle_update;
extern bool more_data_left;
extern bool data_relay_in_process;
extern int send_missing_request_counter;
extern bool is_sending_to_phone;
extern bool large_received;
extern int large_counter;
extern bool large_requested;
extern bool data_gather_relay_on;
extern bool is_sending_on_gather;
extern bool sending_rssi;

// Sending Flags
extern bool sending_tlv_chunks;
extern bool sending_own_periodic;
extern bool sending_stored_tlv;
extern bool sending_missed_tlv;
extern bool sending_tlv_spec;
extern bool sending_relay_tlv;
extern bool sending_periodic_tlv;
extern bool pause_chunks;
extern bool waiting_to_send_phone;
extern bool updated_phone_conn;
extern bool updating_relay_conns;
extern bool periodic_timer_started;
extern bool waiting_conn_update_next_test;
extern int periodic_bytes_done;
ret_code_t change_gap_params_test_rssi(void);
// Sending types
#define CENT_TO_STOPPED -4
#define CENT_TO_REQUEST -3
#define CENT_TO_PERIODIC -2
#define CENT_TO_ALL -1
#define CONN_PHONE_TIMERS true

extern int max_peris_to_relay;

// System configuration
extern int SYSTEM_COMM_MODE; // 0=Nonsensor relay, 1=One sensor relay, 2=Two sensor relays, ??0=RELAY_REQ_ON_QUEUE, 1=RELAY_PERIS_CHOOSE
extern int IS_SENSOR_NODE;
extern int RELAYING_MODE; // 0=All only relay, 1=Periodic relay On-req choose, 2=All choose, 3=Star to phone 
extern uint8_t SEND_TYPE; // 0 = relay, 1 = request, 2 = periodic, 3 = rare
extern bool IS_RELAY;
extern bool IS_TESTING;
extern uint8_t last_val_rssi;
uint32_t rssi_timer_start();
void rssi_timer_handler(void * p_context);

void adv_timer_start(int time);
void adv_timer_handler(void * p_context);
void adv_timer_stop(void);


extern int first_packet_timer_handle;

extern int cent_min_rssi;
extern int cent_min_rssi_og;
extern bool doing_const_tp;
extern int curr_tp_idx;
extern uint8_t tlv_test[244];
extern uint16_t test_len;
//extern int peri_tlv_head;
//extern int peri_tlv_tail;

//extern int peri_tlv_max_size;
extern int tlv_strg_size;
extern int curr_window_index;
extern int window_list[8];
extern int max_trssi_packets;//was 1600
extern int trssi_counter;
extern bool show_rssi;
void beep_handler(void * p_context);
extern float ema10;

extern bool isPhone;

//extern peri_tlv_storage_t peri_tlv_storage[];
#endif // COMMON_H


