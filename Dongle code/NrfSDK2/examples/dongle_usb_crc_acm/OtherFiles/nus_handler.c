#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include "ble_gap.h"
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_srv_common.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "app_timer.h"
#include "bsp_btn_ble.h"
#include "nrf_pwr_mgmt.h"
#include "nus_handler.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "peer_manager_handler.h"
#include "sensorsim.h"
#include "common.h"
#include "peer_manager.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_lesc.h"
#include "nrf_ble_qwr.h"

#include "boards.h"
#include "bsp.h"

#include "ble_conn_state.h"
#include "app_usbd.h"
#include "app_usbd_cdc_acm.h"
//#include "usb_handler.h"
#include "ble_nus2.h"
#include "ble_nus_c2.h"
#include "app_uart.h"
#include "ble_db_discovery.h"
#include "nrf_ble_scan.h"

#include "device_conn.h"
#include "devices_conn_params.h"
#include "devices_tp.h"
#include "nrf_drv_rtc.h"

BLE_NUS_DEF(m_nus, NRF_SDH_BLE_PERIPHERAL_LINK_COUNT);         /**< BLE NUS service instance. */
ble_nus_t* mm_nus = &m_nus;
BLE_NUS_C_ARRAY_DEF(m_ble_nus_c, NRF_SDH_BLE_CENTRAL_LINK_COUNT);       /**< BLE Nordic UART Service (NUS) client instance. */
BLE_DB_DISCOVERY_ARRAY_DEF(m_db_disc, NRF_SDH_BLE_CENTRAL_LINK_COUNT);    /**< Database discovery module instances. */
NRF_BLE_SCAN_DEF(m_scan);                                           /**< Scanning module instance. */
NRF_BLE_GQ_DEF(m_ble_gatt_queue,                                    /**< BLE GATT Queue instance. */
               NRF_SDH_BLE_CENTRAL_LINK_COUNT,
               NRF_BLE_GQ_QUEUE_SIZE);

NRF_BLE_GATT_DEF(m_gatt);                         /**< GATT module instance. */
NRF_BLE_QWRS_DEF(m_qwr, NRF_SDH_BLE_TOTAL_LINK_COUNT);                 /**< Context for the Queued Write module.*/
BLE_ADVERTISING_DEF(m_advertising);


//static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID; /**< Handle of the current connection. */
//static uint16_t m_conn_handles[NRF_SDH_BLE_PERIPHERAL_LINK_COUNT];
//static int8_t current_tx_power[NRF_SDH_BLE_PERIPHERAL_LINK_COUNT]; 
//static uint16_t   m_ble_nus_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3;            /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */



/**@brief UUIDs that the central application scans for if the name above is set to an empty string,
 * and that are to be advertised by the peripherals.
 */
//static char received_prefixes[4];


//#define prefix_size 3
//typedef struct not_rec_t {
//    int offset;
//    struct not_rec_t *next;
//    struct not_rec_t *prev;
//} not_rec_t;

//typedef struct all_rec_t {
//    int chunk;
//    struct all_rec_t *next;
//} all_rec_t;


//bool scanning = false;


//#define RSSI_SCAN_COUNTER_MAX 66
//int8_t rssi_scan_counter = 0;

//uint8_t chosen_peri_number;
//uint8_t connected_peri_number;

//bool data_gather_requested = false;
uint8_t emergency_bit = 0x00;

//char all_received[1500];

//static uint16_t cnt_relay_chunks = 0;


//uint8_t peri_handle_id_array[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
//int16_t peri_relay_array[8] = {};
//char peri_id_all_sent[8] = {};

//bool ensure_central_send = false;
//bool ensure_peri_send = false;
//int ensure_peri_send_counter = 0;
//int ensure_send_counter = 0;
//bool more_data_left = false;

//APP_TIMER_DEF(tp_timers[NRF_SDH_BLE_CENTRAL_LINK_COUNT]);
APP_TIMER_DEF(tp_timer_0);
APP_TIMER_DEF(tp_timer_1);
APP_TIMER_DEF(tp_timer_2);
APP_TIMER_DEF(tp_timer_3);
APP_TIMER_DEF(tp_timer_4);
APP_TIMER_DEF(tp_timer_5);
APP_TIMER_DEF(tp_timer_6);
APP_TIMER_DEF(tp_timer_7);

APP_TIMER_DEF(first_packet_timer);
APP_TIMER_DEF(beep_timer);

APP_TIMER_DEF(m_adv_timer);

APP_TIMER_DEF(m_scan_timer);
int SCAN_TIMER_INTERVAL = 3000;
APP_TIMER_DEF(m_rssi_timer);
int RSSI_TIMER_INTERVAL = 5000;
int RSSI_TIMER_INTERVAL_LONG = 10000;//10000;
APP_TIMER_DEF(m_sys_time_check);
int SYS_TIME_INTERVAL = 60000;
APP_TIMER_DEF(m_conn_param_timer);
int CONN_PARAM_INTERVAL = 2000;
APP_TIMER_DEF(m_periodic_send_timer);
int PERIODIC_TIMER_INTERVAL = 3000; // in ms
bool scan_timer_started = false;
bool start_scan_wait_conn_param_update = false;
uint16_t scan_wait_handle_update = 20;

static ble_uuid_t m_adv_uuids[]          =                                          /**< Universally unique service identifier. */
{
    {BLE_UUID_NUS_SERVICE, BLE_UUID_TYPE_VENDOR_BEGIN}
};

static ble_uuid128_t nus_base_uuid = NUS_BASE_UUID;  // Replace with the NUS base UUID


// To shorten UUID for advertising, not working
void register_vendor_uuid(void)
{
    uint8_t uuid_type;
    ret_code_t err_code;

    // Add custom 128-bit UUID to the softdevice
    err_code = sd_ble_uuid_vs_add(&nus_base_uuid, &uuid_type);
    check_error("UUID change",err_code);

    // Update the UUID type for advertising
    m_adv_uuids[0].type = uuid_type;
}


static ble_gap_scan_params_t m_scan_param =                 /**< Scan parameters requested for scanning and connection. */
{
    .active        = 0x01,
    //.interval      = NRF_BLE_SCAN_SCAN_INTERVAL,
    //.window        = NRF_BLE_SCAN_SCAN_WINDOW,
    .interval      = SCAN_INTERVAL,
    .window        = SCAN_WINDOW,
    .filter_policy = BLE_GAP_SCAN_FP_ACCEPT_ALL,
    .timeout       = NRF_BLE_SCAN_SCAN_DURATION,
    .scan_phys     = BLE_GAP_PHY_1MBPS,
    .extended      = true,
};

void stHandles(void){
    //for (uint8_t i=0; i<NRF_SDH_BLE_PERIPHERAL_LINK_COUNT; i++){
    //  m_conn_handles[i] = BLE_CONN_HANDLE_INVALID;
    //  current_tx_power[i] = -1;
    //  //central_info[i].handle = 99;
    //}
    //OPTIONAL: TOGETHER P and C info
    
    for (uint8_t i=0; i<NRF_SDH_BLE_TOTAL_LINK_COUNT; i++){
        reset_device_base(i);
        reset_device_cent(i);
        reset_device_peri(i, true);
        reset_device_tp(i);
    }
    //ble_gap_conn_params_t conn_update_timer_params = initial_conn_params;
    //if(SEND_TYPE==1){
    //    large_counter = 0;
    //}else{
    //    large_counter = -1;
    //}
    set_this_device_sim_params();
}




/**@brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyse
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    //APP_ERROR_HANDLER(nrf_error);
    send_usb_data(true, "nrf_qwr_error_handler : %d", nrf_error);
}

/**@brief Function for handling the Nordic UART Service Client errors.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nus_error_handler(uint32_t nrf_error)
{
    //APP_ERROR_HANDLER(nrf_error);
    send_usb_data(true, "nus_error_handler : %d", nrf_error);
}


static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {   
        send_usb_data(false, "Failed to update conn Params with %s; h=%d", devices[p_evt->conn_handle].name, p_evt->conn_handle);
        //if(send_test_rssi){
            update_conn_params(p_evt->conn_handle, devices[p_evt->conn_handle].conn_params);
        //}else{
        //    err_code = sd_ble_gap_disconnect(p_evt->conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        //    //APP_ERROR_CHECK(err_code);
        //    if(err_code != 0){
        //        send_usb_data(false, "on conn param evt ERROR:",err_code);
        //    }
        //}

        ///
        //////err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        ////
        //for (uint8_t i = 0; i < NRF_SDH_BLE_PERIPHERAL_LINK_COUNT; i++)
        //{
        //    //if (m_conn_handles[i] != BLE_CONN_HANDLE_INVALID)
        //    if (m_conn_handles[p_evt->conn_handle] != BLE_CONN_HANDLE_INVALID)
        //    {
        //        ret_code_t err_code = sd_ble_gap_disconnect(m_conn_handles[i], BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        //        APP_ERROR_CHECK(err_code);
        //    }
        //}
        ////APP_ERROR_CHECK(err_code);
    }
}

static void conn_params_error_handler(uint32_t nrf_error)
{
    //APP_ERROR_HANDLERAPP_ERROR_HANDLER(nrf_error);
    send_usb_data(true, "conn_params_error_handler : %d", nrf_error);
}

ret_code_t conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    //ble_gap_conn_params_t conn_params;
    //memset(&conn_params, 0, sizeof(conn_params));
    
    //if(IS_RELAY){//DEVICE_ID == 'C' || DEVICE_ID == 'D'){
        //conn_params.min_conn_interval = conn_params_conn_phone_wait.min_conn_interval;//MIN_CONN_INTERVAL;//phone_with_request_conn_params.min_conn_interval;
        //conn_params.max_conn_interval = conn_params_conn_phone_wait.max_conn_interval;//MAX_CONN_INTERVAL;//phone_with_request_conn_params.max_interval;
        //conn_params.slave_latency = conn_params_conn_phone_wait.slave_latency;//SLAVE_LATENCY;//phone_with_request_conn_params.slave_latency;
        //conn_params.conn_sup_timeout = conn_params_conn_phone_wait.conn_sup_timeout;//CONN_SUP_TIMEOUT;//phone_with_request_conn_params.timeout;
        //cp_init.p_conn_params                  = &conn_params_conn_phone_wait;//conn_params_relay_default;//&phone_with_request_conn_params;//&conn_params;
    //}
    //else{
        //conn_params.min_conn_interval = initial_conn_params.min_interval;
        //conn_params.max_conn_interval = initial_conn_params.max_interval;
        //conn_params.slave_latency = initial_conn_params.slave_latency;
        //conn_params.conn_sup_timeout = initial_conn_params.timeout;
    //}

    cp_init.p_conn_params                  = &conn_params_phone_init;//NULL;//&conn_params;//NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    //APP_ERROR_CHECK(err_code);
    check_error("ble_conn_params_init err: ",err_code);
    return err_code;
}



void periodic_timer_handler(void * p_context)
{
    if(sending_stored_tlv){//!sending_own_periodic){
        //if(sending_stored_tlv && finished_stored){
        //    periodic_waiting_bytes += bytes_per_period;
        ////}else if(sending_stored_tlv){
            stored_bytes += bytes_per_period;
        //}else{
            //periodic_waiting_bytes += bytes_per_period;
        //}
        //return;
    }else if(sending_periodic_tlv){
        if(sending_own_periodic){
            //sending_periodic_tlv = true;
            add_periodic_bytes(bytes_per_period);
            send_periodic_tlv();
        }else{
            periodic_waiting_bytes += bytes_per_period;
        }
    }
    //periodic_timer_start();
    //sending_periodic_tlv = true;
    //package_storage[package_head].data[0] = large_counter;
    //package_storage[package_head].data[1] = package_own_counter;
    //package_storage[package_head].data[2] = DEVICE_ID;
    //memset(package_storage[package_head].data+3, 'a', package_max_size-3);
    //package_storage[package_head].length = package_max_size;
    //send_tlv_usb(false, "Added Periodic pckg: ", package_storage[package_head].data, 2, 20,-1);
    //package_head = (package_head+1)%package_storage_size;
    //package_own_counter = (package_own_counter+1)%256;
    //if(package_head == 0){
    //    large_counter = (large_counter+1)%256;
    //}
    //package_count++;
    //if(sending_periodic_tlv && CHOSEN_HANDLE != 255 && devices[CHOSEN_HANDLE].cent.isNotPaused){
    //    send_periodic_tlv();//(m_nus, CHOSEN_HANDLE);
    //}else if(connectedToPhone){
    //    CHOSEN_HANDLE = PHONE_HANDLE;
    //    send_relay_tlv();//(m_nus, PHONE_HANDLE);
    //}
}


bool scan_timer_finished = false;
void scan_timer_handler(void * p_context)
{
    scan_timer_finished = true;
    scan_start();
    sd_ble_gap_ppcp_get(&conn_params_relay_default);
    send_usb_data(false, "250initialy BASE PREFERED PARAMS from stack: min=%d, max=%d, latency=%d, timeout=%d", 
                                conn_params_relay_default.min_conn_interval * UNIT_1_25_MS / 1000,
                                conn_params_relay_default.max_conn_interval * UNIT_1_25_MS / 1000,
                                conn_params_relay_default.slave_latency,
                                conn_params_relay_default.conn_sup_timeout * UNIT_10_MS / 1000);

}

ret_code_t scan_on_timer()
{
    scan_timer_started = true;
    scan_timer_finished = false;
    ret_code_t err_code = app_timer_start(m_scan_timer, APP_TIMER_TICKS(300), NULL);//SCAN_TIMER_INTERVAL), NULL);
    send_usb_data(false, "Scan started timer e:%d", err_code);
    return  err_code;
}


ret_code_t scan_quick(void)
{
    if(count_peri_from_scan_list()==0 || no_scan){
        return 0;
    }
    ret_code_t err_code = nrf_ble_scan_start(&m_scan);
    show_scan_list();
    send_usb_data(false, "Scan quick started"); 
    //remove_peri_from_scan_list(DEVICE_NAME);
    //APP_ERROR_CHECK(err_code);
    check_error("scan start ERROR:",err_code);
    scan_timer_started = false;
    scan_timer_finished = false;
    return  err_code;
}

bool no_scan = false;
ret_code_t scan_start(void)
{   
    //#if IS_RELAY
    ret_code_t err_code = 0;
    if(start_scan_wait_conn_param_update){
        send_usb_data(false, "Scan requested !!!HAVE TO WAIT FOR HANDLE %d!!!", conn_update_timer_handle);
        }
    //    return 1;
    //}else 
    if(count_peri_from_scan_list()>0 && !no_scan){//&&!no_scan && ){

        if(scan_timer_started){
            if(!scan_timer_finished){
                return 1;
            }
            err_code = nrf_ble_scan_start(&m_scan);
            show_scan_list();
            //remove_peri_from_scan_list(DEVICE_NAME);
            //APP_ERROR_CHECK(err_code);
            check_error("scan start ERROR:",err_code);
            scan_timer_started = false;
            scan_timer_finished = false;
            if(err_code == 0){
                send_usb_data(false, "Scan started normally, isP:%d, isR:%d", isPhone, IS_RELAY);
                
            }else{
                send_usb_data(false, "Issue starting scanning, err=%d", err_code);
            }
        }else{
            scan_on_timer();
        }
    }
    
    return err_code;
    //#endif
}

void scan_stop(void)
{   
    nrf_ble_scan_stop();
}

void sys_timer_handler(void * p_context)
{
    send_usb_data(true, "TIME CHECK");
}

void conn_params_handler(void * p_context)
{
    if(devices[conn_update_timer_handle].connected){
        if(conn_update_timer_params.do_tlv){
            send_tlv_usb(true, "!!! Update Conn timer with TLV: ", conn_update_timer_params.tlv, conn_update_timer_params.len, conn_update_timer_params.len, conn_update_timer_handle);
            update_conn_params_send_tlv(conn_update_timer_handle, conn_update_timer_params.params, conn_update_timer_params.tlv, conn_update_timer_params.len, true);
        }else{
            send_usb_data(true, "!!! Update Conn timer %s NO TLV, tlv?=%d, tlvlen=%d",devices[conn_update_timer_handle].name, conn_update_timer_params.do_tlv, conn_update_timer_params.len);
            update_conn_params(conn_update_timer_handle, conn_update_timer_params.params);
        }
        //if(ble_conn_state_central_conn_count() < NRF_SDH_BLE_CENTRAL_LINK_COUNT)
        //{   
        //    scan_start();
        //}
        //if(ble_conn_state_central_conn_count() == NRF_SDH_BLE_CENTRAL_LINK_COUNT && isPhone){
        //    updated_phone_conn = true;
        //    updating_relay_conns = update_all_relay_conn();
        //}
    }
}

uint32_t conn_update_timer_start(int time)
{   
    // Start the timer with the specified interval
    start_scan_wait_conn_param_update = true;
    uint32_t err_code = app_timer_start(m_conn_param_timer, APP_TIMER_TICKS(time), NULL);
    send_usb_data(true, "Conn update timer start for h:%d, time=%d, e:%d", conn_update_timer_handle, time, err_code);
    return  err_code;
}

int first_packet_timer_handle = -1;

void first_packet_timer_start(void){
    ret_code_t err_code = app_timer_start(first_packet_timer, APP_TIMER_TICKS(60000*2), NULL);
    //device->peri.miss_ready = true;
    //devices[h].peri.miss_ctr_extra = 1;
    if(err_code == NRF_SUCCESS){
        send_usb_data(true, "Started first packet timer");
    }
}

void first_packet_handler(void * p_context){
    ret_code_t ret;
    //devices[h].peri.miss_ctr_extra = 1;
    //device_info_t *device = (device_info_t *) p_context;
    ////if(first_packet_timer_handle != 1){
    //device->peri.miss_ready = false;
    //device->peri.miss_sent = true;
    //    send_tlv_from_c(device->relay_h, device->peri.missed, devices->peri.miss_idx + 2, "sending missed prefs");
        //send_tlv_from_p(first_packet_timer_handle, tlv_nameParams, 5);
    //    first_packet_timer_handle = -1;
    //}
    uint8_t tlv_end [2] = {199,199};
    send_tlv_from_p(PHONE_HANDLE, tlv_end, 2);
    send_tlv_usb(true, "END EXPERIMENT", tlv_end, 2,2,PHONE_HANDLE);
    for(int i = 0; i < NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
        if(devices[i].connected){
            disconnect_handle(i);
            send_usb_data(false, "ENDDDDDDDDDDDDDDDDDDDDD");
            bsp_board_led_on(led_BLUE);
        }
    }
}

void timers_init(void)
{
    ret_code_t err_code;

    // Initialize the timer module
    err_code = app_timer_init();
    check_error("App timer init",err_code);
    if(err_code != NRF_SUCCESS){
        bsp_board_led_on(led_BLUE);
    }

    err_code = app_timer_create(&first_packet_timer, APP_TIMER_MODE_SINGLE_SHOT, first_packet_handler);
    check_error("App ADV timer create",err_code);
    // Create a timer
    err_code = app_timer_create(&m_adv_timer, APP_TIMER_MODE_SINGLE_SHOT, adv_timer_handler);
    check_error("App ADV timer create",err_code);
    if(err_code != NRF_SUCCESS){
        bsp_board_led_on(led_BLUE);
    }
    // Create a timer
    err_code = app_timer_create(&m_scan_timer, APP_TIMER_MODE_SINGLE_SHOT, scan_timer_handler);
    check_error("App SCAN timer create",err_code);
    if(err_code != NRF_SUCCESS){
        bsp_board_led_on(led_BLUE);
    }    
    // Create a timer
    //err_code = app_timer_create(&m_rssi_timer, APP_TIMER_MODE_SINGLE_SHOT, rssi_timer_handler);
    //check_error("App RSSI timer create",err_code);
    //if(err_code != NRF_SUCCESS){
    //    bsp_board_led_on(led_BLUE);
    //}
    // Create a timer
    err_code = app_timer_create(&m_periodic_send_timer, APP_TIMER_MODE_REPEATED, periodic_timer_handler);
    check_error("App Periodic timer create",err_code);
    if(err_code != NRF_SUCCESS){
        bsp_board_led_on(led_BLUE);
    }
    err_code = app_timer_create(&m_sys_time_check, APP_TIMER_MODE_REPEATED, sys_timer_handler);
    check_error("App Sys timer create",err_code);
    if(err_code != NRF_SUCCESS){
        bsp_board_led_on(led_BLUE);
    }
    err_code = app_timer_create(&m_conn_param_timer, APP_TIMER_MODE_SINGLE_SHOT, conn_params_handler);
    check_error("App Conn param timer create",err_code);
    if(err_code != NRF_SUCCESS){
        bsp_board_led_on(led_BLUE);
    }
    err_code = app_timer_create(&tp_timer_0, APP_TIMER_MODE_SINGLE_SHOT, tp_timer_handler);
    check_error("TP timer create",err_code);
    devices[0].tp_timer = tp_timer_0;
    if(err_code != NRF_SUCCESS){
        bsp_board_led_on(led_BLUE);
    }
    err_code = app_timer_create(&tp_timer_1, APP_TIMER_MODE_SINGLE_SHOT, tp_timer_handler);
    check_error("TP timer create",err_code);
    devices[1].tp_timer = tp_timer_1;
    if(err_code != NRF_SUCCESS){
        bsp_board_led_on(led_BLUE);
    }
    err_code = app_timer_create(&tp_timer_2, APP_TIMER_MODE_SINGLE_SHOT, tp_timer_handler);
    check_error("TP timer create",err_code);
    devices[2].tp_timer = tp_timer_2;
    if(err_code != NRF_SUCCESS){
        bsp_board_led_on(led_BLUE);
    }
    err_code = app_timer_create(&tp_timer_3, APP_TIMER_MODE_SINGLE_SHOT, tp_timer_handler);
    check_error("TP timer create",err_code);
    devices[3].tp_timer = tp_timer_3;
    if(err_code != NRF_SUCCESS){
        bsp_board_led_on(led_BLUE);
    }
    err_code = app_timer_create(&tp_timer_4, APP_TIMER_MODE_SINGLE_SHOT, tp_timer_handler);
    check_error("TP timer create",err_code);
    devices[4].tp_timer = tp_timer_4;
    if(err_code != NRF_SUCCESS){
        bsp_board_led_on(led_BLUE);
    }
    err_code = app_timer_create(&tp_timer_5, APP_TIMER_MODE_SINGLE_SHOT, tp_timer_handler);
    check_error("TP timer create",err_code);
    devices[5].tp_timer = tp_timer_5;
    if(err_code != NRF_SUCCESS){
        bsp_board_led_on(led_BLUE);
    }    
    //if(NRF_SDH_BLE_CENTRAL_LINK_COUNT > 6){

        err_code = app_timer_create(&tp_timer_6, APP_TIMER_MODE_SINGLE_SHOT, tp_timer_handler);
        check_error("TP timer create",err_code);
        devices[6].tp_timer = tp_timer_6;
    if(err_code != NRF_SUCCESS){
        bsp_board_led_on(led_BLUE);
    }
        err_code = app_timer_create(&tp_timer_7, APP_TIMER_MODE_SINGLE_SHOT, tp_timer_handler);
        check_error("TP timer create",err_code);
        devices[7].tp_timer = tp_timer_7;
    ////}
    //if(err_code != NRF_SUCCESS){
    //    bsp_board_led_on(led_BLUE);
    //}
    //err_code = app_timer_create(&beep_timer, APP_TIMER_MODE_SINGLE_SHOT, beep_handler);
    //check_error("TP timer create",err_code);
    //if(err_code != NRF_SUCCESS){
    //    bsp_board_led_on(led_BLUE);
    //}

    //if(IS_RELAY){
    //    for (uint32_t i = 0; i < NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++) {
    //        err_code = app_timer_create(&tp_timers[i], 
    //                                               APP_TIMER_MODE_SINGLE_SHOT, 
    //                                               tp_timer_handler);
    //        check_error("TP timer create",err_code);
    //    }
    //}
}
int adv_phoning = false;
int adv_ph_time = 5000;
void adv_timer_start(int time){
    ret_code_t err_code;
    if(!adv_phoning){
        if(DEVICE_ID == 'M'){
            //adv_ph_time = 100;
            adv_start(false);
            adv_phoning = true;
            return;
        }else if(DEVICE_ID == 'E'){
            adv_ph_time = 2000;
        }else if(DEVICE_ID == 'L'){
            adv_ph_time = 2000;//*2;
        }else if(DEVICE_ID == 'R'){
            adv_ph_time = 2000;//*3;
        }
    }
    
    if(!adv_phoning){
        err_code = app_timer_start(m_adv_timer, APP_TIMER_TICKS(adv_ph_time), NULL);
    }else{
        err_code = app_timer_start(m_adv_timer, APP_TIMER_TICKS(10000), NULL);
        adv_phoning = false;
    }
    if(err_code == NRF_SUCCESS){
        send_usb_data(true, "Started advertising for Phone");
    }
}

void adv_timer_handler(void * p_context){
    if(!adv_phoning){
        adv_start(false);
        adv_phoning = true;
    }else{
        ret_code_t err = sd_ble_gap_adv_stop(m_advertising.adv_handle);
        if(err == NRF_SUCCESS && !devices[connecting_handle].cent.connecting){
            send_usb_data(true, "Timer stopped advertising for Phone, not connected, connPh=%d", connectedToPhone);
            uint8_t filed_conn_phone[2] = {72, 0};
            send_tlv_from_p(RELAY_HANDLE, filed_conn_phone, 2);
        }else{
            send_usb_data(true, "Timer couldnt stopp advertising, err=%d, connPh=%d", err, connectedToPhone);
        }
    }
    adv_phoning = false;
}

void adv_timer_stop(void){
    app_timer_stop(m_adv_timer);
}

void tp_timer_start(device_info_t *device){
    if(!device->connected){
        send_usb_data(true, "ERROR starting TP timer not connected to dev");
        return;
    }
    //uint32_t app_timer_stop(tp_timers[h]);
     doing_const_tp = false;
    send_test_rssi = false;
    //err_code = app_timer_start(tp_timers[h], APP_TIMER_TICKS((int) devices[h].curr_interval*devices[h].curr_sl*2.5), h);
    uint32_t err_code = app_timer_stop(device->tp_timer);
    
    //if(TIMEOUT_TPC && device->handle >= NRF_SDH_BLE_CENTRAL_LINK_COUNT){//IS_RELAY){
    //    if(err_code != NRF_SUCCESS){
    //        send_usb_data(true, "ERROR stopping TP timer for %s, e=%d", device->name, err_code);
    //    }
    //    int extra = 0;
    //    if(device->curr_sl == 0){
    //        extra = 1;
    //    }
    //    if(send_test_rssi){
    //        //err_code = app_timer_start(device->tp_timer, APP_TIMER_TICKS(device->curr_interval*(device->curr_sl + extra)*3), device);
    //        err_code = app_timer_start(device->tp_timer, APP_TIMER_TICKS(device->curr_interval*3 + device->curr_interval*device->curr_sl), device);
    //        //send_usb_data(true, "TP timer started send_test %s, e=%d", device->name, err_code);
    //    }
    //    else{
    //        //err_code = app_timer_start(device->tp_timer, APP_TIMER_TICKS(device->curr_interval*(device->curr_sl + extra)*3), device);
    //        err_code = app_timer_start(device->tp_timer, APP_TIMER_TICKS(device->curr_interval*3 + device->curr_interval*device->curr_sl), device);
    //        //send_usb_data(true, "TP timer started norml %s, e=%d", device->name, err_code);
    //    }
    //}else{
    if(device->handle < NRF_SDH_BLE_CENTRAL_LINK_COUNT){
        err_code = app_timer_start(device->tp_timer, APP_TIMER_TICKS(device->curr_interval), device);
        if(err_code != NRF_SUCCESS){
            send_usb_data(true, "ERROR starting TP timer for %s, e=%d", device->name, err_code);
        }
        else{
            send_usb_data(true, "starting PDR TP timer for %s", device->name);
        }
    }else if(device->curr_interval != 0 && !doStar){
        if(sending_own_periodic || finished_stored){
            err_code = app_timer_start(device->tp_timer, APP_TIMER_TICKS(PERIODIC_TIMER_INTERVAL + device->curr_interval*3),device);
        }else if(sending_stored_tlv){
            err_code = app_timer_start(device->tp_timer, APP_TIMER_TICKS(device->curr_interval*3),device);//(device->curr_sl + 1 )*3), device);
        }
        //send_usb_data(true, "starting TIMEOUT TP timer for %s, handle=%d, int*3=%d,err=%d", device->name, device->handle, device->curr_interval*3,err_code);
    }
    
}

void tp_timer_stop(device_info_t *device){
    //send_usb_data(true, "stop TP timer for %s", device->name);
    uint32_t err_code = app_timer_stop(device->tp_timer);
    if(err_code != NRF_SUCCESS){
        send_usb_data(true, "ERROR starting TP timer for %s, e=%d", device->name, err_code);
    }else if(device->handle < NRF_SDH_BLE_CENTRAL_LINK_COUNT){//IS_RELAY){
        send_usb_data(true, "success stop TP timer for %s", device->name);
    }
    if(device->handle >= NRF_SDH_BLE_CENTRAL_LINK_COUNT){
        send_usb_data(true, "STOPPEDD TIMEOUT TP timer for %s, handle=%d, int*3=%d,err=%d", device->name, device->handle, device->curr_interval*3,err_code);
    }
}

void tp_timer_handler(void *p_context){
    ret_code_t ret;
    device_info_t *device = (device_info_t *) p_context;
    //if(device->handle < NRF_SDH_BLE_CENTRAL_LINK_COUNT){//IS_RELAY){
        if(!device->connected || (send_test_rssi && doing_const_tp)){// || !sending_rssi){
            send_usb_data(true, "TP timer bad return !conn = %d, PerisDoingConst = %d, Peri Not Sending = %d", !device->connected, (PERIS_DO_TP && doing_const_tp), !sending_rssi);
            return;
        }
        if(device->handle < NRF_SDH_BLE_CENTRAL_LINK_COUNT){
            if(device->tp.get_new_rssi && !doStar){
                device->cnt_no_rssi++;
                if(device->cnt_no_rssi > device->curr_sl+3 && device->tp.skip == 0){
                    int tx_idx = device->tp.tx_idx;
                    if(tx_idx == device->tp.max_tx_idx){
                        
                    }else{
                        send_usb_data(true, "TIMEOUT CENT RSSI %s:%d to ME to %d dBm from %d dBm", device->name, device->handle, tx_range[tx_idx+1], tx_range[tx_idx]);
                        //devices[h].tp.tx_idx = received_data[1];
                        if(PERIS_DO_TP){
                            device->tp.other_tx_idx = tx_idx+1;
                        }
                        device->tp.requested_tx_idx = -1;
                        if(!device->tp.received_counter == -1){
                            device->tp.received_counter = 3;
                            device->tp.wait_avg_cnt = 3;
                        }
                        instruct_tp_change(device->handle, tx_idx+1);//devices[h].tp.tx_idx);
                        device->tp.tx_idx = tx_idx+1;
                        //reset_pdr_timer(h);
                        reset_tpc(device->handle);
                        device->cnt_no_rssi = 0;
                         ret = app_timer_start(device->tp_timer, APP_TIMER_TICKS(device->curr_interval), device);
                         return;
                    }
                }
            }else{
                device->cnt_no_rssi = 0;
            }
            device->tp.get_new_rssi = true;
            //if(device->tp.first_evt_skip){
            //    device->tp.first_evt_skip = false;
            //    device->tp.docountpack = true;
            //}else{
            if(device->tp.pdr_wait_count > -1 && device->tp.skip == 0 && device->tp.use_pdr){
                device->tp.evt_head = (device->tp.evt_head + 1)% (PDR_WINDOW + 1);
                device->tp.cnt_pack[device->tp.evt_head] = 0;

                if(device->tp.pdr_wait_count < PDR_WINDOW){
                   device->tp.pdr_wait_count++;
                }
            
                if(device->tp.pdr_wait_count > 0){
                   do_tpc_pdr(device->handle);
                }
            }
            
            
            
                // Sample PDR calculation
                //int max = MAX_EVENT_WINDOW;
                //if(device->tp.window + 1 < MAX_EVENT_WINDOW){
                //    max = device->tp.window + 1;
                //}
                //int retr = (int8_t) 0;
                //int j = (device->tp.evt_head + MAX_EVENT_WINDOW - 1)% MAX_EVENT_WINDOW;
                //for(int i = 0; i <  max - 1; i++){
                //    retr += device->tp.cnt_pack[j];
                //    j = (j + MAX_EVENT_WINDOW - 1)% MAX_EVENT_WINDOW;
                //}
                //retr = (int8_t) 100*retr/((max - 1) * device->tp.evt_max_p);
                //char ema[20] ="";
                //sprintf(ema, "%d.%d", (int8_t)  device->tp.ema_rssi, abs((int8_t)(device->tp.ema_rssi*10))%10);
                //send_usb_data(true, "last_P = %d, HEAD = %d, EMA = %s, P sent = %d /100, PS 2 = %d /100, EVT_PCK = [%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d]", 
                //      device->peri.last, device->tp.evt_head, ema,
                //      retr, retr + 10,
                //      device->tp.cnt_pack[0], device->tp.cnt_pack[1],
                //      device->tp.cnt_pack[2],  device->tp.cnt_pack[3], device->tp.cnt_pack[4], device->tp.cnt_pack[5], device->tp.cnt_pack[6], 
                //      device->tp.cnt_pack[7], device->tp.cnt_pack[8], device->tp.cnt_pack[9], device->tp.cnt_pack[10]);
            //}
            if(device->curr_interval > 0){
                device->time_passed += device->curr_interval;
                if(device->time_passed >= 120000 && device->connected){
                    update_conn_params(device->handle, device->conn_params);
                    device->time_passed = 0;
                }
                ret = app_timer_start(device->tp_timer, APP_TIMER_TICKS(device->curr_interval), device);
            //send_usb_data(true, "starting PDR TP timer for %s", device->name);
            }

        }else{
            app_timer_stop(device->tp_timer);
             send_usb_data(true, "ACTIVATED TIMEOUT TP timer for %s, handle=%d, int*3=%d", device->name, device->handle, device->curr_interval*3);
            int i = device->tp.own_idx;
            int tx_idx = device->tp.own_idx;
            if(i < max_tx_idx_own && !doStar){
                i++;
              
                int8_t expected_rssi = (int8_t) (device->tp.ema_rssi + abs(device->tp.tx_range[i] - device->tp.tx_range[tx_idx]));
                //if(PERIS_DO_TP || PERI_TP_TIMER){
                    uint8_t tlv_timeoutrssi[3] = {91, i, devices->tp.own_idx};
                    //int h = NRF_SDH_BLE_CENTRAL_LINK_COUNT;
                    int h = device->handle;

                    ret = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_CONN, h, tx_range[i]);

                    if(ret == NRF_SUCCESS){
                        devices[h].tp.own_idx = i;
                        devices[h].tp.requested_tx_idx = i;
                        if(PERIS_DO_TP){
                            devices[h].tp.tx_idx = i;
                        }
            
                        send_usb_data(true, "Increased TIMEOUT TP for %s:%d to %d dBm from %d dBm __ ema=%d, min_limit=%d, last_rssi=%d, exp_ema=%d", devices[h].name, h, devices[h].tp.tx_range[i],(int8_t) tx_range[tx_idx],(int8_t) devices[h].tp.ema_rssi, devices[h].tp.min_rssi_limit, (int8_t) last_val_rssi, (int8_t)expected_rssi);
                        devices[h].tp.ema_rssi = expected_rssi;
                        //if(h < NRF_SDH_BLE_CENTRAL_LINK_COUNT){
                        //    send_tlv_from_c(h, tlv_rssiChange, 4, "Incr TX pow");
                        //}else{
                        //    send_tlv_from_p(h, tlv_rssiChange, 4);
                        //}
                        //devices[h].tp.skip = 3;
                        add_to_device_tlv_strg(NRF_SDH_BLE_CENTRAL_LINK_COUNT, tlv_timeoutrssi, 3);
                         if(sending_own_periodic || finished_stored){
                              app_timer_start(device->tp_timer, APP_TIMER_TICKS(PERIODIC_TIMER_INTERVAL + device->curr_interval*3),device);
                          }else if(sending_stored_tlv){
                              app_timer_start(device->tp_timer, APP_TIMER_TICKS(device->curr_interval*3),device);//(device->curr_sl + 1 )*3), device);
                          }
                    }else{
                        send_usb_data(false, "ERROR:%d, increase TIMEOUT Own TP to %s:%d to %d dBm from %d dBm, ema=%d, rssiLast=%d", ret, devices[h].name, h, devices[h].tp.tx_range[i], devices[h].tp.tx_range[devices[h].tp.own_idx], (int8_t) devices[h].tp.ema_rssi, (int8_t) last_val_rssi);
                        //devices[h].tp.incr_counter++;
                    }
                    //device->tp.ema_rssi = device->tp.ideal;//expected_rssi;
                    ////device->tp.do_first_ema = true;
                    //device->tp.skip = 3;
                    //if(PERIS_DO_TP){
                    //    device->tp.received_counter = 2;//0;
                    //}
                //}else{
                //    tlv_rssiChange[0] = 66;
                //    tlv_rssiChange[1] = i;
                //    tlv_rssiChange[2] = (int8_t) device->tp.ema_rssi;
                //    tlv_rssiChange[3] = (int8_t) device->tp.last_rssi;
                //    int8_t expected_rssi = (int8_t) (device->tp.ema_rssi + abs(device->tp.tx_range[i] - device->tp.tx_range[tx_idx]));
                //    if(WAIT_FOR_TP_CONFIRMATION){
                //        if(i != device->tp.requested_tx_idx && device->tp.requested_tx_idx == -1){
                //            device->tp.requested_tx_idx = i;
                //            send_tlv_from_c(device->handle, tlv_rssiChange, 4, "Incr TX pow");
            
                //            //device->tp.ema_rssi = ema_rssi;
                //            //send_usb_data(true, "New TPC2: ema=%d.%d, newRssi=%d, minRSSI=%d, maxRSSI=%d, coeff=%d.%d, otherTP=%d, ownTP=%d", (int8_t) device->tp.ema_rssi, (int8_t) -1* ((int8_t)(device->tp.ema_rssi*10))%10, new_rssi, (int8_t) device->tp.min_rssi_limit, (int8_t) device->tp.max_rssi_limit, (int8_t)  ema_coefficient, (int8_t) ((int8_t)(ema_coefficient*10))%10, device->tp.tx_idx, device->tp.own_idx);


                //            send_usb_data(true, "Request TIMEOUT %s:%d to Increase TP to %d dBm from %d dBm __ ema=%d, min_limit=%d, last_rssi=%d, exp_ema=%d", device->name, device->handle, device->tp.tx_range[i],(int8_t) device->tp.tx_range[tx_idx],(int8_t)  device->tp.ema_rssi, device->tp.min_rssi_limit, (int8_t) device->tp.last_rssi, (int8_t)expected_rssi);
                //            ////device->tp.ema_rssi = expected_rssi;
                //            ////device->tp.incr_counter = 0;
                //            ////device->tp.tx_idx = i;

                //        }
                //    }else{
                //        send_tlv_from_c(device->handle, tlv_rssiChange, 4, "Incr TX pow");
                //        device->tp.tx_idx = i;
                //        //send_usb_data(true, "New TPC2: ema=%d.%d, newRssi=%d, minRSSI=%d, maxRSSI=%d, coeff=%d.%d, otherTP=%d, ownTP=%d", (int8_t) device->tp.ema_rssi, (int8_t) -1* ((int8_t)(device->tp.ema_rssi*10))%10, new_rssi, (int8_t) device->tp.min_rssi_limit, (int8_t) device->tp.max_rssi_limit, (int8_t)  ema_coefficient, (int8_t) ((int8_t)(ema_coefficient*10))%10, device->tp.tx_idx, device->tp.own_idx);


                //        send_usb_data(true, "Request TIMEOUT %s:%d to Increase TP to %d dBm from %d dBm __ ema=%d, min_limit=%d, last_rssi=%d, exp_ema (ideal)=%d", device->name, device->handle, device->tp.tx_range[i],(int8_t) device->tp.tx_range[tx_idx],(int8_t) device->tp.ema_rssi, device->tp.min_rssi_limit, (int8_t) device->tp.last_rssi, (int8_t)device->tp.ideal);
                //        device->tp.ema_rssi = device->tp.ideal;//expected_rssi;
                //        //device->tp.do_first_ema = true;
                //        device->tp.received_counter = 2;//0;
                //    }
            
                //}
                ////if(device->tp.received_counter >= 3){
                ////    device->tp.received_counter = device->tp.received_counter - 3;
                ////}else{
        
                //}
                //device->tp.skip = 3;
            }
        }
        return;
}

void tp_timer_handler2(void *p_context){
    ret_code_t ret;
    device_info_t *device = (device_info_t *) p_context;
    if(device->handle < NRF_SDH_BLE_CENTRAL_LINK_COUNT){//IS_RELAY){
        if(!device->connected || (send_test_rssi && doing_const_tp)){// || !sending_rssi){
            send_usb_data(true, "TP timer bad return !conn = %d, PerisDoingConst = %d, Peri Not Sending = %d", !device->connected, (PERIS_DO_TP && doing_const_tp), !sending_rssi);
            return;
        }
        device->tp.get_new_rssi = true;
        if(device->tp.first_evt_skip){
            device->tp.first_evt_skip = false;
            device->tp.docountpack = true;
        }else{
            if(device->tp.cnt_evt < MAX_EVENT_WINDOW){
                device->tp.cnt_evt++;
            }
            device->tp.evt_head = (device->tp.evt_head + 1)% MAX_EVENT_WINDOW;
            device->tp.cnt_pack[device->tp.evt_head] = 0;
            
            int max = MAX_EVENT_WINDOW;
            if(device->tp.window + 1 < MAX_EVENT_WINDOW){
                max = device->tp.window + 1;
            }
            
            // Sample PDR calculation
            int retr = (int8_t) 0;
            int j = (device->tp.evt_head + MAX_EVENT_WINDOW - 1)% MAX_EVENT_WINDOW;
            for(int i = 0; i <  max - 1; i++){
                retr += device->tp.cnt_pack[j];
                j = (j + MAX_EVENT_WINDOW - 1)% MAX_EVENT_WINDOW;
            }
            retr = (int8_t) 100*retr/((max - 1) * device->tp.evt_max_p);
            char ema[20] ="";
            sprintf(ema, "%d.%d", (int8_t)  device->tp.ema_rssi, abs((int8_t)(device->tp.ema_rssi*10))%10);
            send_usb_data(true, "last_P = %d, HEAD = %d, EMA = %s, P sent = %d /100, PS 2 = %d /100, EVT_PCK = [%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d]", 
                  device->peri.last, device->tp.evt_head, ema,
                  retr, retr + 10,
                  device->tp.cnt_pack[0], device->tp.cnt_pack[1],
                  device->tp.cnt_pack[2],  device->tp.cnt_pack[3], device->tp.cnt_pack[4], device->tp.cnt_pack[5], device->tp.cnt_pack[6], 
                  device->tp.cnt_pack[7], device->tp.cnt_pack[8], device->tp.cnt_pack[9], device->tp.cnt_pack[10]);
        }

        ret = app_timer_start(device->tp_timer, APP_TIMER_TICKS(device->curr_interval), device);
        //send_usb_data(true, "starting PDR TP timer for %s", device->name);
        return;
    }else if(TIMEOUT_TPC){
        if(!device->connected || (send_test_rssi && doing_const_tp) || !sending_rssi){
            send_usb_data(true, "TP timer bad return !conn = %d, PerisDoingConst = %d, Peri Not Sending = %d", !device->connected, (PERIS_DO_TP && doing_const_tp), !sending_rssi);
            return;
        }
        int tx_idx = device->tp.own_idx;
        int i = device->tp.own_idx;
        if(i < device->tp.own_max_tx_idx){
            i++;
    
            int8_t expected_rssi = (int8_t) (device->tp.ema_rssi + abs(device->tp.tx_range[i] - device->tp.tx_range[tx_idx]));
            if(PERIS_DO_TP || PERI_TP_TIMER){
                uint8_t tlv_timeoutrssi[3] = {91, i, devices->tp.tx_idx};
                //int h = NRF_SDH_BLE_CENTRAL_LINK_COUNT;
                int h = device->handle;

                ret = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_CONN, h, tx_range[i]);

                if(ret == NRF_SUCCESS){
                    devices[h].tp.own_idx = i;
                    if(PERIS_DO_TP){
                        devices[h].tp.tx_idx = i;
                    }
            
                    send_usb_data(true, "Increased TIMEOUT TP for %s:%d to %d dBm from %d dBm __ ema=%d, min_limit=%d, last_rssi=%d, exp_ema=%d", devices[h].name, h, devices[h].tp.tx_range[i],(int8_t) tx_range[tx_idx],(int8_t) devices[h].tp.ema_rssi, devices[h].tp.min_rssi_limit, (int8_t) last_val_rssi, (int8_t)expected_rssi);
                    devices[h].tp.ema_rssi = expected_rssi;
                    //if(h < NRF_SDH_BLE_CENTRAL_LINK_COUNT){
                    //    send_tlv_from_c(h, tlv_rssiChange, 4, "Incr TX pow");
                    //}else{
                    //    send_tlv_from_p(h, tlv_rssiChange, 4);
                    //}
                    devices[h].tp.skip = 3;
                    add_to_device_tlv_strg(NRF_SDH_BLE_CENTRAL_LINK_COUNT, tlv_timeoutrssi, 3);
                }else{
                    send_usb_data(false, "ERROR:%d, increase TIMEOUT Own TP to %s:%d to %d dBm from %d dBm, ema=%d, rssiLast=%d", ret, devices[h].name, h, devices[h].tp.tx_range[i], devices[h].tp.tx_range[devices[h].tp.own_idx], (int8_t) devices[h].tp.ema_rssi, (int8_t) last_val_rssi);
                    //devices[h].tp.incr_counter++;
                }
                device->tp.ema_rssi = device->tp.ideal;//expected_rssi;
                //device->tp.do_first_ema = true;
                device->tp.skip = 3;
                if(PERIS_DO_TP){
                    device->tp.received_counter = 2;//0;
                }
            }else{
                tlv_rssiChange[0] = 66;
                tlv_rssiChange[1] = i;
                tlv_rssiChange[2] = (int8_t) device->tp.ema_rssi;
                tlv_rssiChange[3] = (int8_t) device->tp.last_rssi;
                int8_t expected_rssi = (int8_t) (device->tp.ema_rssi + abs(device->tp.tx_range[i] - device->tp.tx_range[tx_idx]));
                if(WAIT_FOR_TP_CONFIRMATION){
                    if(i != device->tp.requested_tx_idx && device->tp.requested_tx_idx == -1){
                        device->tp.requested_tx_idx = i;
                        send_tlv_from_c(device->handle, tlv_rssiChange, 4, "Incr TX pow");
            
                        //device->tp.ema_rssi = ema_rssi;
                        //send_usb_data(true, "New TPC2: ema=%d.%d, newRssi=%d, minRSSI=%d, maxRSSI=%d, coeff=%d.%d, otherTP=%d, ownTP=%d", (int8_t) device->tp.ema_rssi, (int8_t) -1* ((int8_t)(device->tp.ema_rssi*10))%10, new_rssi, (int8_t) device->tp.min_rssi_limit, (int8_t) device->tp.max_rssi_limit, (int8_t)  ema_coefficient, (int8_t) ((int8_t)(ema_coefficient*10))%10, device->tp.tx_idx, device->tp.own_idx);


                        send_usb_data(true, "Request TIMEOUT %s:%d to Increase TP to %d dBm from %d dBm __ ema=%d, min_limit=%d, last_rssi=%d, exp_ema=%d", device->name, device->handle, device->tp.tx_range[i],(int8_t) device->tp.tx_range[tx_idx],(int8_t)  device->tp.ema_rssi, device->tp.min_rssi_limit, (int8_t) device->tp.last_rssi, (int8_t)expected_rssi);
                        ////device->tp.ema_rssi = expected_rssi;
                        ////device->tp.incr_counter = 0;
                        ////device->tp.tx_idx = i;

                    }
                }else{
                    send_tlv_from_c(device->handle, tlv_rssiChange, 4, "Incr TX pow");
                    device->tp.tx_idx = i;
                    //send_usb_data(true, "New TPC2: ema=%d.%d, newRssi=%d, minRSSI=%d, maxRSSI=%d, coeff=%d.%d, otherTP=%d, ownTP=%d", (int8_t) device->tp.ema_rssi, (int8_t) -1* ((int8_t)(device->tp.ema_rssi*10))%10, new_rssi, (int8_t) device->tp.min_rssi_limit, (int8_t) device->tp.max_rssi_limit, (int8_t)  ema_coefficient, (int8_t) ((int8_t)(ema_coefficient*10))%10, device->tp.tx_idx, device->tp.own_idx);


                    send_usb_data(true, "Request TIMEOUT %s:%d to Increase TP to %d dBm from %d dBm __ ema=%d, min_limit=%d, last_rssi=%d, exp_ema (ideal)=%d", device->name, device->handle, device->tp.tx_range[i],(int8_t) device->tp.tx_range[tx_idx],(int8_t) device->tp.ema_rssi, device->tp.min_rssi_limit, (int8_t) device->tp.last_rssi, (int8_t)device->tp.ideal);
                    device->tp.ema_rssi = device->tp.ideal;//expected_rssi;
                    //device->tp.do_first_ema = true;
                    device->tp.received_counter = 2;//0;
                }
            
            }
            //if(device->tp.received_counter >= 3){
            //    device->tp.received_counter = device->tp.received_counter - 3;
            //}else{
        
            //}
            device->tp.skip = 3;
        }
        uint32_t err_code = app_timer_stop(device->tp_timer);
        if(err_code != NRF_SUCCESS){
            send_usb_data(true, "ERROR stopping TP timer for %s, e=%d", device->name, err_code);
        }
        int extra = 0;
        if(device->curr_sl == 0){
            extra = 1;
        }
        if(send_test_rssi){
            //err_code = app_timer_start(device->tp_timer, APP_TIMER_TICKS(device->curr_interval*(device->curr_sl + extra)*3), device);
            err_code = app_timer_start(device->tp_timer, APP_TIMER_TICKS(device->curr_interval*4 + device->curr_interval*device->curr_sl), device);
            //send_usb_data(true, "TP timer started send_test %s, e=%d", device->name, err_code);
        }
        else{
            //err_code = app_timer_start(device->tp_timer, APP_TIMER_TICKS(device->curr_interval*(device->curr_sl + extra)*3), device);
            err_code = app_timer_start(device->tp_timer, APP_TIMER_TICKS(device->curr_interval*4 + device->curr_interval*device->curr_sl), device);
            //send_usb_data(true, "TP timer started norml %s, e=%d", device->name, err_code);
        }
    }
}



bool blue_on = true;

void beep_timer_start(){
    app_timer_stop(beep_timer);
    uint32_t err_code = app_timer_start(beep_timer, APP_TIMER_TICKS(2000), NULL);
    check_error("beep start bad %e", err_code);
}

void beep_timer_stop(){
    app_timer_stop(beep_timer);
}

void beep_handler(void * p_context){
    send_usb_data(true, "beep");
    uint32_t err_code = app_timer_start(beep_timer, APP_TIMER_TICKS(2000), NULL);
    check_error("beep handler bad %e", err_code);
}

uint32_t rssi_timer_start()
{   
    
    uint32_t err_code = 0 ;
    if(tlv_test[10] == 'A'){//devices[0].tp.min_rssi_limit == -71+2){
        //change_rssi_margins_peri(devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.min_rssi_limit, devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.max_rssi_limit, devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.ideal);
        change_rssi_margins_cent(cent_min_rssi, cent_min_rssi+7, cent_min_rssi+3);
        change_rssi_margins_peri(cent_min_rssi, cent_min_rssi+7, cent_min_rssi+3);
        uint32_t err_code = app_timer_start(m_rssi_timer, APP_TIMER_TICKS(RSSI_TIMER_INTERVAL_LONG), NULL);
        waiting_conn_update_next_test = false;
        //devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.received_counter = 0;
        change_conn_tx(0, NRF_SDH_BLE_CENTRAL_LINK_COUNT, devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.ema_rssi, devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.last_rssi);
        uint8_t reset_tlv[3] = {4, (uint8_t) window_list[curr_window_index], (int8_t) cent_min_rssi};
        send_tlv_from_p(NRF_SDH_BLE_CENTRAL_LINK_COUNT, reset_tlv, 3);
        send_usb_data(false, "RSSI timer start FIRST e:%d", err_code);
    //}else if(waiting_conn_update_next_test){
    //    uint32_t err_code = app_timer_start(m_rssi_timer, APP_TIMER_TICKS(3000), NULL);
    //    send_usb_data(false, "RSSI timer for updating connParams start e:%d", err_code);
    }else{
        uint32_t err_code = app_timer_start(m_rssi_timer, APP_TIMER_TICKS(RSSI_TIMER_INTERVAL_LONG), NULL);
        waiting_conn_update_next_test = false;
        send_usb_data(false, "RSSI timer start e:%d", err_code);
    }
    send_usb_data(false, "XXXXX");
    beep_timer_stop();
    bsp_board_leds_off();
    if(!doing_const_tp){
        bsp_board_led_on(led_green);
    }
    bsp_board_led_on(led_BLUE);
       
    
    return  err_code;
}

void rssi_timer_handler(void * p_context)
{
    //if(waiting_conn_update_next_test){
    //    uint8_t reset_tlv[3] = {4,4, (int8_t) (devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.min_rssi_limit)};
    //    send_tlv_from_p(NRF_SDH_BLE_CENTRAL_LINK_COUNT, reset_tlv, 3);
    //    update_conn_params(NRF_SDH_BLE_CENTRAL_LINK_COUNT, conn_params_test_rssi);
    //}
    //else 
    if(!sending_rssi){
        sending_rssi = true;
        bsp_board_leds_off();
        bsp_board_led_on(led_green);
        bsp_board_led_on(led_GREEN);
        if(!doing_const_tp){
            if(PERIS_DO_TP){
                devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.received_counter = devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.window;
                devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.do_first_ema = true;
                do_rssi_ema_self(NRF_SDH_BLE_CENTRAL_LINK_COUNT, 100);
            }else{
                devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.received_counter = -1;
            }
        }else{
            devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.received_counter = -1;
            change_conn_tx(tx_range[curr_tp_idx], NRF_SDH_BLE_CENTRAL_LINK_COUNT, tx_range[curr_tp_idx], tx_range[curr_tp_idx]);
        }
        send_usb_data(true, "Starting to send test char = %c", tlv_test[10]);
        send_usb_data(false, "beep");
        beep_timer_stop();
        beep_timer_start();
        send_test_rssi_tlv();
    }
}

uint32_t periodic_timer_start()
{
    periodic_timer_stop();
    uint32_t err_code = app_timer_start(m_periodic_send_timer, APP_TIMER_TICKS(PERIODIC_TIMER_INTERVAL), NULL);
    send_usb_data(false, "Periodic start e:%d", err_code);
    return  err_code;
}

uint32_t periodic_timer_stop()
{
    uint32_t err_code = app_timer_stop(m_periodic_send_timer);
    send_usb_data(false, "Periodic stop e:%d", err_code);
    return  err_code;
}

uint32_t sys_timer_start()
{
    // Start the timer with the specified interval

    uint32_t err_code = app_timer_start(m_sys_time_check, APP_TIMER_TICKS(SYS_TIME_INTERVAL), NULL);
    //send_usb_data(false, "Periodic start e:%d", err_code);
    return  err_code;
}

///**@brief Function for assigning new connection handle to available instance of QWR module.
// *
// * @param[in] conn_handle New connection handle.
// */
static void multi_qwr_conn_handle_assign(uint16_t conn_handle)
{
    //for (uint32_t i = 0; i < NRF_SDH_BLE_TOTAL_LINK_COUNT; i++)
    //{
    //    if (m_qwr[i].conn_handle == BLE_CONN_HANDLE_INVALID)
    //    {
    //        ret_code_t err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr[i], conn_handle);
    //        //APP_ERROR_CHECK(err_code);
    //        if(err_code != 0){
    //            send_usb_data(false, "Multi qwr assign ERROR%d:",err_code);
    //        }
    //        break;
    //    }
    //}
    ret_code_t err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr[conn_handle], conn_handle);
    send_usb_data(false, "Multi qwr assigned to idx=%d, was invalid?=%d; err:%d",conn_handle, m_qwr[conn_handle].conn_handle == BLE_CONN_HANDLE_INVALID, err_code);
}

/**@brief Function for handling database discovery events.
 *
 * @details This function is a callback function to handle events from the database discovery module.
 *          Depending on the UUIDs that are discovered, this function forwards the events
 *          to their respective services.
 *
 * @param[in] p_event  Pointer to the database discovery event.
 */
static void db_disc_handler(ble_db_discovery_evt_t * p_evt)
{
    ble_db_discovery_t const * p_db = (ble_db_discovery_t *)p_evt->params.p_db_instance;

    ble_nus_c_on_db_disc_evt(&m_ble_nus_c[p_evt->conn_handle], p_evt);

    if (p_evt->evt_type == BLE_DB_DISCOVERY_AVAILABLE) {
        //NRF_LOG_INFO("DB Discovery instance %p available on conn handle: %d",
        //             p_db,
        //             p_evt->conn_handle);
        //NRF_LOG_INFO("Found %d services on conn_handle: %d",
        //             p_db->srv_count,
        //             p_evt->conn_handle);
        send_usb_data(false, "DB disc inst:%d, serv:%d, handle:%d", p_db, p_db->srv_count, p_evt->conn_handle);
        
        //uint16_t i = p_evt->conn_handle;
        //for (uint8_t i=0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
            
        //if(peri_relay_info[i].handle != 99){
        //    send_usb_data(false, "ERROR: HighestPref; Idx %d was:%d is now conn_handle: %d: IF:%d",i,peri_relay_info[i].handle, p_evt->conn_handle, peri_relay_info[i].handle == 99);
        //}
        //send_usb_data(false, "HandlesIdx %d was:%d is now conn_handle: %d: IF:%d",i,peri_relay_info[i].handle, p_evt->conn_handle, peri_relay_info[i].handle == 99);
        //peri_relay_info[i].handle = p_evt->conn_handle;
        //peri_relay_info[i].finished = false;
        //strcpy(peri_relay_info[i].non_received, "Not Rec ");
        //strcpy(peri_relay_info[i].retrans, "Not Rex ");
        //peri_relay_info[i].head = NULL;
        //peri_relay_info[i].curr = NULL;

        //ret_code_t err_code = sd_ble_gap_rssi_start(p_evt->conn_handle, 1, 0);
        //APP_ERROR_CHECK(err_code);
        //send_usb_data(false, "Started RSSI for Peri handle:%d ,e:%d", p_evt->conn_handle, err_code);
            //peri_relay_info[i].head_rec = NULL;
            //peri_relay_info[i].curr_rec = NULL;
        //    break;
        //}
        //}
    }
}


/**
 * @brief Database discovery initialization.
 */
ret_code_t db_discovery_init(void)
{
    ble_db_discovery_init_t db_init;

    memset(&db_init, 0, sizeof(ble_db_discovery_init_t));

    db_init.evt_handler  = db_disc_handler;
    db_init.p_gatt_queue = &m_ble_gatt_queue;

    ret_code_t err_code = ble_db_discovery_init(&db_init);
    APP_ERROR_CHECK(err_code);
    return err_code;
}


/**@snippet [Handling events from the ble_nus_c module] */

/**@brief Function for initializing the Nordic UART Service (NUS) client. */
ret_code_t nus_c_init(void)
{
    ret_code_t       err_code;
    ble_nus_c_init_t init;

    init.evt_handler   = ble_nus_c_evt_handler;
    init.error_handler = nus_error_handler;
    init.p_gatt_queue  = &m_ble_gatt_queue;
    for (uint32_t i = 0; i < NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++)
    {
        err_code = ble_nus_c_init(&m_ble_nus_c[i], &init);
        APP_ERROR_CHECK(err_code);
    }
    return err_code;
}

void start_rssi(uint16_t handle){
  ret_code_t ret = sd_ble_gap_rssi_start(handle, 1, 0);
  send_usb_data(false, "Started RSSI for Peri handle:%d ,e:%d", handle, ret);
}


/**@brief   Function for handling BLE events from the central application.
 *
 * @details This function parses scanning reports and initiates a connection to peripherals when a
 *          target UUID is found. It updates the status of LEDs used to report the central application
 *          activity.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void on_ble_central_evt(ble_evt_t const * p_ble_evt)
{//ble_evt_handler
    ret_code_t            err_code;
    ble_gap_evt_t const * p_gap_evt = &p_ble_evt->evt.gap_evt;
    uint16_t handle = p_gap_evt->conn_handle;

    switch (p_ble_evt->header.evt_id)
    {
        // Upon connection, check which peripheral is connected (HR or RSC), initiate DB
        // discovery, update LEDs status, and resume scanning, if necessary.
        
        case BLE_GAP_EVT_CONNECTED:
        {
            if(p_gap_evt->conn_handle >= NRF_SDH_BLE_CENTRAL_LINK_COUNT){
                send_usb_data(true, "BAD central_evt CONNECTED, handle=%d too big", handle);
                return;
            }
            scan_stop();
            send_usb_data(false, "Connected to Peri target %d, with IDX:%d", p_ble_evt->evt.gap_evt.conn_handle, p_gap_evt->conn_handle);
            err_code = ble_nus_c_handles_assign(&m_ble_nus_c[p_gap_evt->conn_handle], 
                                                p_ble_evt->evt.gap_evt.conn_handle, NULL);
            //APP_ERROR_CHECK(err_code);
            check_error("ble_nus_c_handles_assign ERROR:",err_code);
            devices[p_gap_evt->conn_handle].peri.nus = &m_ble_nus_c[p_gap_evt->conn_handle];
            send_usb_data(false, "Devices[%d], assigned m_ble_nus_c", p_gap_evt->conn_handle);

            // start discovery of services. The NUS Client waits for a discovery result
            err_code = ble_db_discovery_start(&m_db_disc[p_gap_evt->conn_handle], p_ble_evt->evt.gap_evt.conn_handle);
            //APP_ERROR_CHECK(err_code);
            check_error("ble_db_discovery_start ERROR:",err_code);
            //break;

            //err_code = sd_ble_gap_rssi_start(p_ble_evt->evt.gap_evt.conn_handle, 0, 0);
            ////APP_ERROR_CHECK(err_code);
            //send_usb_data(false, "Started RSSI for Peri handle:%d ,e:%d", p_ble_evt->evt.gap_evt.conn_handle, err_code);
            
            // Assign connection handle to the QWR module.
            multi_qwr_conn_handle_assign(p_gap_evt->conn_handle);
            //for (uint8_t i=0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
            //    if(peri_relay_info[i].handle == BLE_CONN_HANDLE_INVALID){
            //        peri_relay_info[i].handle = p_gap_evt->conn_handle;
            //    }
            //}
            // Update LEDs status, and check whether to look for more peripherals to connect to.
            //bsp_board_led_on(CENTRAL_CONNECTED_LED);
            //ALLCONN
            if(spec_target_peri[0] != 0){
                //memset(spec_target_peri, 0, sizeof(spec_target_peri));
            }
            //devices[handle].connected = true;
            send_usb_data(false, "Connected to %d peris", ble_conn_state_central_conn_count());
            //if(ble_conn_state_central_conn_count() < NRF_SDH_BLE_CENTRAL_LINK_COUNT)
            //{
            //    // Resume scanning.
            //    bsp_board_led_on(CENTRAL_SCANNING_LED);
            //    scan_start();
            //}
        } break;


        // Upon disconnection, reset the connection handle of the peer that disconnected,
        // update the LEDs status and start scanning again.
        case BLE_GAP_EVT_DISCONNECTED:
        {//Central
            
            if(p_gap_evt->conn_handle >= NRF_SDH_BLE_CENTRAL_LINK_COUNT){
                send_usb_data(true, "BAD central_evt DISCONNECTED, handle=%d too big", handle);
                return;
            }
            
            
                if(start_scan_wait_conn_param_update || conn_update_timer_handle == handle || !devices[handle].peri.connToPhone){
                    start_scan_wait_conn_param_update = false;
                    conn_update_timer_handle = 20;
                    if(!isPhone || doStar){
                        add_peri_to_scan_list(devices[handle].name);
                    }
                    scan_start();
                }
            
            if(devices[p_gap_evt->conn_handle].id != 255){
                send_usb_data(true, "DISCONNECTED from peripheral %s (reason: 0x%x), conn = %d",
                             devices[p_gap_evt->conn_handle].name,
                             p_gap_evt->params.disconnected.reason, devices[handle].connected);
            }else{
                send_usb_data(true, "DISCONNECTED EARLY! handle %d (reason: 0x%x)",
                             p_gap_evt->conn_handle,
                             p_gap_evt->params.disconnected.reason);
                if(!data_gather_relay_on && !connectedToPhone){
                    //if(!isPhone){
                        scan_start();
                    //}
                }
            }
            devices[handle].connected = false;
            if(devices[handle].peri.last > -1 && devices[handle].peri.batch_number > -1){
                uint8_t t[4] = {12, devices[handle].id, devices[handle].peri.batch_number, devices[handle].peri.last};
                if(devices[RELAY_HANDLE].connected){
                    send_tlv_from_c(RELAY_HANDLE, t, 4, "Tell rel starting prefs, batches");
                }
            }
            err_code = sd_ble_gap_rssi_stop(p_ble_evt->evt.gap_evt.conn_handle);
            send_usb_data(false, "Stopped RSSI for Peri handle:%d e:%d", p_ble_evt->evt.gap_evt.conn_handle, err_code);
            reset_device_tp(p_ble_evt->evt.gap_evt.conn_handle);
            
            if(IS_RELAY && devices[handle].id != 255){
                if(devices[handle].peri.type == 1){
                    count_onreq--;
                    relay_info.disc_type1++;
                    if(devices[handle].peri.askedAdvPhone){
                        count_onreq_not_phone--;
                        relay_info.disc_type1_sink++;
                    }
                }else if(devices[handle].peri.type == 2){
                    count_periodic--;
                    relay_info.disc_type2++;
                }else if(devices[handle].peri.type == 3){
                    count_periodic--;
                    relay_info.disc_type3++;
                }
                //char name[6];
                //memcpy(name, devices[handle].name, 5);
                ////connected_peris[devices[p_gap_evt->conn_handle].nameNum-1] = false;
                //devices[handle].connected = false;
                //m_target_periph_names[devices[handle].nameNum-1] = name;

                //strcpy(m_target_periph_names[devices[handle].nameNum-1], devices[handle].name);
                //add_peri_to_scan_list(devices[handle].name);
                //m_target_periph_names[devices[handle].nameNum-1] = devices[handle].name;//was namenum FIX THIS
                //send_usb_data(false, "New scanning list is: %s %s %s %s %s %s", m_target_periph_names[0], m_target_periph_names[1], m_target_periph_names[2], m_target_periph_names[3], m_target_periph_names[4], m_target_periph_names[5]);
                //send_usb_data(false, "Erased handle %d info, name:%s", p_gap_evt->conn_handle, devices[p_gap_evt->conn_handle].name);
                //reset_device_full(p_gap_evt->conn_handle);
                //if(SEND_TYPE){
                //    find_best_cent();
                //}else{
                //    package_tail = (package_tail+255)%package_storage_size;
                //}
                //CHOSEN_HANDLE = 255;
                //if(SEND_TYPE && p_gap_evt->conn_handle == CHOSEN_HANDLE && ble_conn_state_peripheral_conn_count() > 0){
                //    for (uint8_t i=NRF_SDH_BLE_CENTRAL_LINK_COUNT; i<NRF_SDH_BLE_TOTAL_LINK_COUNT; i++){
                //        if(i != p_gap_evt->conn_handle && devices[i].nameNum != 255){
                //            if(CHOSEN_HANDLE == p_gap_evt->conn_handle || devices[CHOSEN_HANDLE].tp.tx_idx > devices[i].tp.tx_idx){
                //                CHOSEN_HANDLE = i;
                //            }
                //        }
                //    }
                //    if(CHOSEN_HANDLE == p_gap_evt->conn_handle){
                //        CHOSEN_HANDLE = 255;
                //    }else{
                //        if(SENDING_PERIODIC_TLV){
                //            send_periodic_tlv(m_nus, CHOSEN_HANDLE);
                //        }
                //    }
                //}
            }

            // check on_req_queue to change connection status of disconnected handle
            #if RELAY_REQ_ON_QUEUE
              if(on_req_queue != NULL && devices[handle].peri.chosenRelay){
                  on_req_queue_t *temp_req = on_req_queue;
                  int j = 0;
                  while(temp_req != NULL){
                      send_usb_data(false,"On_req_queue[%d] = %s", j, devices[temp_req->handle].name);
                      if(temp_req->next == NULL){
                          send_usb_data(false, "On_req_queue[%d] = NULL", j+1);
                      }
                      temp_req = temp_req->next;
                      j++;
                  }

                  if(on_req_queue->next == NULL){
                      if(devices[handle].peri.finished){
                          scan_start();
                      }else{
                          if(!scanning_for_on_req){
                              strcpy(m_target_periph_name, devices[handle].name);
                              scanning_for_on_req = true;
                              send_usb_data(true, "Start to scan for missing On_REQ name = %s", m_target_periph_name);
                              scan_start();
                          }
                      }
                  }else if(devices[on_req_queue->next->handle].connected){
                      on_req_queue_t *disc_req = (on_req_queue_t *)malloc(sizeof(on_req_queue_t));
                    
                      disc_req->handle = on_req_queue->handle;
                      disc_req->id = on_req_queue->id;
                      disc_req->next = NULL;
                    
                      last_on_req_queue->next = disc_req;
                      last_on_req_queue = disc_req;

                      on_req_queue = on_req_queue->next;
                      uint16_t new_h = on_req_queue->handle;

                      if(devices[new_h].peri.is_sending_on_req){
                          if(devices[new_h].peri.batch_number == -1){
                              tlv_resumeData[2] = 0;
                          }else{
                              tlv_resumeData[2] = (uint8_t) devices[new_h].peri.batch_number;
                          }
                          if(devices[new_h].peri.last == -1){
                              tlv_resumeData[3] = 0;
                          }else{
                              tlv_resumeData[3] = (uint8_t) devices[new_h].peri.last;
                          }
                          char type[30];
                          scanning_for_on_req = false;
                          sprintf(type, "Restart on b:%d, p:%d", devices[new_h].peri.batch_number, devices[new_h].peri.last);
                          send_tlv_from_c(new_h, tlv_resumeData, 1, type);
                      }else{
                          send_tlv_from_c(new_h, tlv_reqData, 2, "Sent start Sending req data");
                      }

                      devices[new_h].peri.is_sending_on_req = true;
                      update_conn_params(new_h, on_request_conn_params);
                      calibrate_conn_params(new_h, 10);
                  }else{
                      if(!scanning_for_on_req){
                          strcpy(m_target_periph_name, devices[on_req_queue->handle].name);
                          scanning_for_on_req = true;
                          send_usb_data(true, "Start to scan for missing On_REQ name = %s", m_target_periph_name);
                          scan_start();
                      }
                  }
              }
            #else
              //scan_start();
            #endif
        } break;
        case BLE_GAP_EVT_RSSI_CHANGED :
         {//Central   
            if(devices[handle].tp.ideal > -50 || devices[handle].curr_interval <= 40){
                //send_usb_data(true, "%s, BAD for RSSI idealTP=%d, interval=%d", devices[handle].name, devices[handle].tp.ideal, devices[handle].curr_interval);
                return;
            }
            if(p_gap_evt->conn_handle >= NRF_SDH_BLE_CENTRAL_LINK_COUNT){
                //send_usb_data(true, "BAD central_evt RSSI, handle=%d too big", handle);
                return;
            }
            int8_t rssi = p_ble_evt->evt.gap_evt.params.rssi_changed.rssi;
            uint8_t channel = p_ble_evt->evt.gap_evt.params.rssi_changed.ch_index;
            //if(PERIS_DO_TP){// && !doing_const_tp){
            //    uint8_t new_rssi[3] = {99, (uint8_t) rssi, devices[handle].tp.own_idx};
            //    add_to_device_tlv_strg(handle, new_rssi, 3);
            //}else{
            
                if(devices[handle].tp.get_new_rssi){
                //    //do_rssi_ema2(handle, rssi);
                    //if(devices[handle].tp.skip > 0){
                    //    devices[handle].tp.skip--;
                    //}else{
                        do_tpc_rssi(handle, rssi);

                //    //send_usb_data(false, "new rssi for %c = %d dBm", (char)devices[handle].id, rssi);
                        devices[handle].tp.get_new_rssi = false;
                    //}
                }
                
            //}



            //if(send_test_rssi){
            //    if(rssi < 0){
            //        last_rssi_test = (uint8_t) (rssi*(-1));
            //    }
            //    return;
            //}
            
            ////uint16_t handle = p_gap_evt->conn_handle;
            ////send_usb_data(false, "Got RSSI %d from handle %d", rssi, handle);
            //////bool kot=true;
            
            ////if(rssi > -65 && peri_relay_info[p_ble_evt->evt.gattc_evt.conn_handle].rssi != -40){
                
            ////    send_usb_data(false, "Got RSSI %d from handle %d", rssi, handle);
            ////    tlv_rssiChange[2] = (uint8_t) -40;
            ////    send_tlv_from_c(handle,  tlv_rssiChange, 3, "Change RSSI to -40");
            ////}
            ///////
            ////uint8_t tx_id = devices[handle].tp.tx_idx;
            ////int8_t eq_rssi_from_cent = rssi - devices[handle].tp.rssi + devices[handle].tp.tx_range[tx_id];
            ////ret_code_t ret;

            //if(rssi < -85 && devices[handle].tp.rssi == -40){
            ////if(rssi < -85 && devices[handle].tp.rssi == -40){//tx_id == devices[handle].tp.max_tx_idx){
            //    if(devices[handle].tp.signal_to_lower_counter == 0){
            //        uint8_t tlv_incrTx[2] = {7,(uint8_t)  devices[handle].tp.tx_range[devices[handle].tp.tx_idx]};
            //        send_usb_data(false, "NOTICE %s:%d received with RSSI LOW %d", devices[handle].name, handle, rssi);
            //        send_tlv_from_c(handle,  tlv_incrTx, 2, "MUST INCR TX POW");
            //    }
            //    devices[handle].tp.signal_to_lower_counter = (devices[handle].tp.signal_to_lower_counter + 1)%5;
            //}else{
            //    do_rssi_ema2(handle, rssi);
            //}

            //////ALLCONN: Update Peri avg rssi
            ////int8_t old_rssi_val = devices[handle].tp.rssi_window[devices[handle].tp.head_rssi];
            ////devices[handle].tp.rssi_window[devices[handle].tp.head_rssi] = rssi - devices[handle].tp.rssi;//eq_rssi_from_cent;
            ////devices[handle].tp.head_rssi = (devices[handle].tp.head_rssi + 1) % RSSI_WINDOW_SIZE;
            ////devices[handle].tp.sum_rssi = devices[handle].tp.sum_rssi + rssi - devices[handle].tp.rssi - old_rssi_val;
            ////devices[handle].tp.avg_rssi = devices[handle].tp.sum_rssi / RSSI_WINDOW_SIZE;
            //////ALLCONN: END

            ////if(tx_id > 0 && -80 >= eq_rssi_from_cent){// raise tx power to peripheral
            ////    if(devices[handle].tp.incr_counter >= 5){
            ////        tx_id--;
            ////        ret = change_conn_tx(devices[handle].tp.tx_range[tx_id], handle);
            ////        if(ret == NRF_SUCCESS){
            ////            tlv_rssiChange[1] = (uint8_t) devices[handle].tp.tx_range[tx_id];
            ////            send_tlv_from_c(handle, tlv_rssiChange, 2, "Incr TX pow");
            ////            send_usb_data(true, "Increased TX power to %s:%d to %d dBm __ avg=%d", devices[handle].name, handle, devices[handle].tp.tx_range[tx_id], devices[handle].tp.avg_rssi);
            ////            devices[handle].tp.incr_counter = 0;
            ////            devices[handle].tp.tx_idx = tx_id;
            ////        }else{
            ////            tx_id++;
            ////            send_usb_data(false, "ERROR:%d, Incr TX power to %s:%d to %d dBm", ret, devices[handle].name, handle, devices[handle].tp.tx_range[tx_id-1]);
            ////            devices[handle].tp.incr_counter++;
            ////        }
                    
            ////    }else{
            ////        devices[handle].tp.incr_counter++;
            ////        //send_usb_data(false, "Got RSSI %d from Peri:%d, Count:%d, EqRSSI:%d", rssi,handle, devices[handle].tp.incr_counter, eq_rssi_from_cent);
            ////    }
            ////}
            ////else if(tx_id < devices[handle].tp.max_tx_idx){ 
            ////    if(-75 < eq_rssi_from_cent + devices[handle].tp.tx_range[tx_id+1] - devices[handle].tp.tx_range[tx_id]
            ////            && -75 < devices[handle].tp.avg_rssi + devices[handle].tp.tx_range[tx_id+1]){// lower peripheral tx power
            ////        if(devices[handle].tp.decr_counter >= 10){
            ////            tx_id++;
            ////            ret = change_conn_tx(devices[handle].tp.tx_range[tx_id], handle);
            ////            if(ret == NRF_SUCCESS){
            ////                tlv_rssiChange[1] = (uint8_t) devices[handle].tp.tx_range[tx_id];
            ////                send_tlv_from_c(handle, tlv_rssiChange, 2, "Decr TX pow");
            ////                send_usb_data(true, "Decreased TX power to %s:%d to %d dBm __ avg=%d", devices[handle].name, handle, devices[handle].tp.tx_range[tx_id], devices[handle].tp.avg_rssi);
            ////                devices[handle].tp.decr_counter = 0;
            ////                devices[handle].tp.tx_idx = tx_id;
            ////            }else{
            ////                tx_id--;
            ////                send_usb_data(false, "ERROR:%d, Decr TX power to %s:%d to %d dBm", ret, devices[handle].name, handle, devices[handle].tp.tx_range[tx_id+1]);
            ////                devices[handle].tp.decr_counter++;
            ////            }
                    
            ////        }else{
            ////            devices[handle].tp.decr_counter++;
            ////            //send_usb_data(false, "Got RSSI %d from Peri:%d, Count:%d, EqRSSI:%d", rssi,handle, devices[handle].tp.decr_counter,eq_rssi_from_cent);
            ////        }
            ////    }
            ////}
            ////for (uint8_t i = 0; i < NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++) {
            ////    if (rssi_trackers[i].handle == handle) {
            ////        rssi_trackers[i].rssi_history[rssi_trackers[i].rssi_index] = rssi;
            ////        send_usb_data(false, "RSSI in table hndl:%d, tabix:%d, rssi:%d",handle,i, rssi_trackers[i].rssi_history[rssi_trackers[i].rssi_index]);
            ////        rssi_trackers[i].rssi_index = (rssi_trackers[i].rssi_index + 1) % RSSI_HISTORY_SIZE;
            ////        kot = false;
            ////        break;
            ////    }
            ////}
            ////send_usb_data(false, "RSSI changed hndl:%d  , rssi:%d",handle, rssi);
            ////if(kot){
            ////  // If handle not found, find an empty slot
            ////  for (uint8_t i = 0; i < NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++) {
            ////      if (rssi_trackers[i].handle == -1) {
            ////          rssi_trackers[i].handle = (int16_t) handle;
            ////          rssi_trackers[i].rssi_history[0] = rssi;
            ////          send_usb_data(false, "RSSI in table hndl:%d, tabix:%d, rssi:%d",handle,i, rssi_trackers[i].rssi_history[rssi_trackers[i].rssi_index]);
            ////          rssi_trackers[i].rssi_index = 1;
            ////          break;
            ////      }
            ////  }
            ////}
        }break; 

        case BLE_GAP_EVT_TIMEOUT:
        {//Central
            if(p_gap_evt->conn_handle >= NRF_SDH_BLE_CENTRAL_LINK_COUNT){
                send_usb_data(true, "BAD central_evt TIMEOUT, handle=%d too big", handle);
                return;
            }
            send_usb_data(false, "Connection Request timed out.");
            // No timeout for scanning is specified, so only connection attemps can timeout.
            if (p_gap_evt->params.timeout.src == BLE_GAP_TIMEOUT_SRC_CONN)
            {
                send_usb_data(false, "Connection Request timed out.");
            }
        } break;

        case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
        {//Central   
            if(p_gap_evt->conn_handle >= NRF_SDH_BLE_CENTRAL_LINK_COUNT){
                send_usb_data(true, "BAD central_evt PARAM_UPDATE_REQ, handle=%d too big", handle);
                return;
            }

            //if(send_test_rssi){
            //  send_usb_data(false, "Got connParam Request from %s: min=%d, max=%d, latency=%d, timeout=%d", 
            //                    devices[handle].name,
            //                    p_gap_evt->params.conn_param_update_request.conn_params.min_conn_interval * UNIT_1_25_MS / 1000,
            //                    p_gap_evt->params.conn_param_update_request.conn_params.max_conn_interval * UNIT_1_25_MS / 1000,
            //                    p_gap_evt->params.conn_param_update_request.conn_params.slave_latency,
            //                    p_gap_evt->params.conn_param_update_request.conn_params.conn_sup_timeout * UNIT_10_MS / 1000);
            //  if(p_gap_evt->params.conn_param_update_request.conn_params.min_conn_interval * UNIT_1_25_MS / 1000 <= conn_params_test_rssi.max_conn_interval * UNIT_1_25_MS / 1000 ||
            //            p_gap_evt->params.conn_param_update_request.conn_params.min_conn_interval * UNIT_1_25_MS / 1000 <= 150){
            //    //sending_rssi = true;
            //    //send_test_rssi_tlv();
            //    update_conn_params(handle, conn_params_test_rssi);
            //    //rssi_timer_start();
            //    return;
            //  }else{
                  
            //      err_code = sd_ble_gap_conn_param_update(p_gap_evt->conn_handle,
            //                                &p_gap_evt->params.conn_param_update.conn_params);
            //      return;
            //  }
            //}

            // Accept parameters requested by peer.
            //if(p_gap_evt->params.conn_param_update_request.conn_params.min_conn_interval * UNIT_1_25_MS / 1000 > conn_params_test_rssi.max_conn_interval * UNIT_1_25_MS / 1000 ||
            //            p_gap_evt->params.conn_param_update_request.conn_params.min_conn_interval * UNIT_1_25_MS / 1000 > 220){
            //      send_usb_data(true, "Got BAD connParam Request from %s: min=%d, max=%d, latency=%d, timeout=%d", 
            //              devices[handle].name,
            //              p_gap_evt->params.conn_param_update_request.conn_params.min_conn_interval * UNIT_1_25_MS / 1000,
            //              p_gap_evt->params.conn_param_update_request.conn_params.max_conn_interval * UNIT_1_25_MS / 1000,
            //              p_gap_evt->params.conn_param_update_request.conn_params.slave_latency,
            //              p_gap_evt->params.conn_param_update_request.conn_params.conn_sup_timeout * UNIT_10_MS / 1000);
            //      //update_conn_params(handle, conn_params_test_rssi);
            //      return;
            //}
            if(p_gap_evt->params.conn_param_update_request.conn_params.min_conn_interval == devices[handle].conn_params.min_conn_interval){
                err_code = sd_ble_gap_conn_param_update(p_gap_evt->conn_handle,
                                        &p_gap_evt->params.conn_param_update_request.conn_params);
            }
            //devices[handle].conn_params.min_conn_interval = p_gap_evt->params.conn_param_update.conn_params.min_conn_interval;
            //devices[handle].conn_params.max_conn_interval = p_gap_evt->params.conn_param_update.conn_params.max_conn_interval;
            //devices[handle].conn_params.slave_latency = p_gap_evt->params.conn_param_update.conn_params.slave_latency;
            //devices[handle].conn_params.conn_sup_timeout = p_gap_evt->params.conn_param_update.conn_params.conn_sup_timeout;
            //APP_ERROR_CHECK(err_code);
            check_error("BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST CENT",err_code);

            send_usb_data(false, "Got connParam Request from %s: min=%d, max=%d, latency=%d, timeout=%d", 
                                devices[handle].name,
                                p_gap_evt->params.conn_param_update_request.conn_params.min_conn_interval * UNIT_1_25_MS / 1000,
                                p_gap_evt->params.conn_param_update_request.conn_params.max_conn_interval * UNIT_1_25_MS / 1000,
                                p_gap_evt->params.conn_param_update_request.conn_params.slave_latency,
                                p_gap_evt->params.conn_param_update_request.conn_params.conn_sup_timeout * UNIT_10_MS / 1000);

            if(devices[handle].connected){
                send_usb_data(false, "Got connParam Request from %s: min=%d, max=%d, latency=%d, timeout=%d", 
                                devices[handle].name,
                                p_gap_evt->params.conn_param_update_request.conn_params.min_conn_interval * UNIT_1_25_MS / 1000,
                                p_gap_evt->params.conn_param_update_request.conn_params.max_conn_interval * UNIT_1_25_MS / 1000,
                                p_gap_evt->params.conn_param_update_request.conn_params.slave_latency,
                                p_gap_evt->params.conn_param_update_request.conn_params.conn_sup_timeout * UNIT_10_MS / 1000);
                
                devices[handle].conn_params = p_gap_evt->params.conn_param_update_request.conn_params;
            }else{
                send_usb_data(false, "Got connParam Request from NEW Peripheral: min=%d, max=%d, latency=%d, timeout=%d",
                                p_gap_evt->params.conn_param_update_request.conn_params.min_conn_interval * UNIT_1_25_MS / 1000,
                                p_gap_evt->params.conn_param_update_request.conn_params.max_conn_interval * UNIT_1_25_MS / 1000,
                                p_gap_evt->params.conn_param_update_request.conn_params.slave_latency,
                                p_gap_evt->params.conn_param_update_request.conn_params.conn_sup_timeout * UNIT_10_MS / 1000);
            }

        } break;
        
        case BLE_GAP_EVT_CONN_PARAM_UPDATE:{//Central
            //err_code = sd_ble_gap_conn_param_get(p_gap_evt->conn_handle, &conn_params);
            //if (err_code == NRF_SUCCESS) {
            ble_gap_conn_params_t new_params = p_gap_evt->params.conn_param_update.conn_params;

                //if(send_test_rssi){
                //  if(p_gap_evt->params.conn_param_update_request.conn_params.min_conn_interval * UNIT_1_25_MS / 1000 <= conn_params_test_rssi.max_conn_interval * UNIT_1_25_MS / 1000){
                //    //sending_rssi = true;
                //    //send_test_rssi_tlv();
                //    err_code = sd_ble_gap_conn_param_update(p_gap_evt->conn_handle,
                //                                &p_gap_evt->params.conn_param_update.conn_params);
                //    //rssi_timer_start();
                //    return;
                //  }else{
                //      update_conn_params(handle, conn_params_test_rssi);
                //      return;
                //  }
                //}
                send_usb_data(true, "Current connection interval for %s:%d: MIN %d units, %d in ms; MAX %d units, %d in ms; latency=%d, timeout=%d", 
                        devices[p_gap_evt->conn_handle].name, p_gap_evt->conn_handle,
                        new_params.min_conn_interval * UNIT_1_25_MS / 1000, 
                        new_params.min_conn_interval + new_params.min_conn_interval/4, 
                        new_params.max_conn_interval * UNIT_1_25_MS / 1000, 
                        new_params.max_conn_interval + new_params.max_conn_interval/4,
                        p_gap_evt->params.conn_param_update_request.conn_params.slave_latency,
                        p_gap_evt->params.conn_param_update_request.conn_params.conn_sup_timeout * UNIT_10_MS / 1000);

                devices[handle].curr_interval = new_params.max_conn_interval * UNIT_1_25_MS / 1000;
                devices[handle].curr_sl = new_params.slave_latency;
                if(devices[handle].curr_interval >= 50 && devices[handle].tp.ideal != 1){
                //if(conn_update_timer_handle == handle){
                    if(devices[handle].tp.stopped_pdr){
                        devices[handle].tp.stopped_pdr = false ;
                        devices[handle].tp.use_pdr = true;
                    }
                    tp_timer_start(&devices[handle]);
                    reset_tpc(handle);
                    
                }
                //}
                int h = handle;
                //if(!(isPhone && doStar)){
                //if(conn_update_timer_handle == h && devices[conn_update_timer_handle].peri.type == 3 && devices[handle].curr_interval <=400){
                    //if(check_update(h)){
                    //    // second update to be done
                    //    return;
                    //}
                    //check_update(h);
                    //devices[h].waiting_conn_update = false;
                    //update_conn_params_send_tlv(h, devices[h].conn_params, tlv_start_data, 2, true);
                    //return;
                //}else{
                devices[h].time_passed = 0;
                if(check_update(h)){
                    
                    return;
                }
                send_usb_data(false, "conn_update_timer_handle = %d, hand=%d, start_scan_wait_conn_param_update=%d",conn_update_timer_handle, handle, start_scan_wait_conn_param_update); 
                if(conn_update_timer_handle == handle && start_scan_wait_conn_param_update){
                    start_scan_wait_conn_param_update = false;
                    conn_update_timer_handle = 20;
                //}
                    show_scan_list();
                    if(count_peri_from_scan_list()>0){
                        if(isPhone){
                            scan_quick();
                        }else{
                            scan_start();
                        }
                        send_usb_data(false, "return to scan!"); 
                        return;
                    }else{
                        send_usb_data(false, "ELSE  doStart=%d, ble_cnt= %d, doStCnt=%d, !updatin=%d",doStar, ble_conn_state_central_conn_count(), doStarCount, !updating_relay_conns); 
                        if(doStar && ble_conn_state_central_conn_count() == doStarCount && isPhone && !updating_relay_conns){
                            updated_phone_conn = true;
                            updating_relay_conns = update_all_relay_conn_tlv(fast_tlv, 1);//(tlv_start_data, 2);//(fast_tlv, 1);
                            send_usb_data(false, "!!!doSTARRR!!!"); 
                            //if(updating_relay_conns){
                            bsp_board_led_on(led_GREEN);
                            return;
                        }else if(isPhone && relay_sent_peris && count_peri_from_scan_list()==0 && !doStar){
                                //uint8_t tt[1] = {11};//fast tlv was 13 before instead
                                no_scan = true;
                                //send_tlv_from_c(RELAY_HANDLE, tt, 1, "conned to peris");
                                updated_phone_conn = true;
                                updating_relay_conns = update_all_relay_conn_tlv(fast_tlv, 1);
                                send_usb_data(false, "conn all sent peris"); 
                                //bsp_board_led_on(led_GREEN);
                                return;
                          }
                    }
                }
                show_scan_list();
                if(count_peri_from_scan_list()>0){
                    if(isPhone){
                        scan_quick();
                    }else{
                        scan_start();
                    }
                    send_usb_data(false, "return to scan!"); 
                    return;
                }
                if(updating_relay_conns){
                    updating_relay_conns = update_all_relay_conn_tlv(devices[h].relay_all_upd_params.tlv, devices[h].relay_all_upd_params.len);
                    return;
                }
                //}
                //if(conn_update_timer_handle == handle && start_scan_wait_conn_param_update){//conn param update first then scan for other device
                //    //if((devices[handle].conn_params.min_conn_interval * UNIT_1_25_MS / 1000 <= 
                //    //                                p_gap_evt->params.conn_param_update.conn_params.min_conn_interval * UNIT_1_25_MS / 1000) 
                //    //                && (devices[handle].conn_params.max_conn_interval * UNIT_1_25_MS / 1000 >= 
                //    //                                    p_gap_evt->params.conn_param_update.conn_params.min_conn_interval * UNIT_1_25_MS / 1000))
                //    //    {
                //            start_scan_wait_conn_param_update = false;
                //            conn_update_timer_handle = 20;
                            
                //            if(count_peri_from_scan_list()>0 && (!(doStar && ble_conn_state_central_conn_count()>=doStarCount && isPhone) || IS_RELAY)){
                //                send_usb_data(false, "!!!NOW YOU CAN SCAN!!!"); 
                //                scan_quick();
                //                return;
                //            }else if(isPhone && relay_sent_peris && count_peri_from_scan_list()==0 && !doStar){
                //                uint8_t tt[1] = {13};
                //                no_scan = true;
                //                send_tlv_from_c(RELAY_HANDLE, tt, 1, "conned to peris");
                //                send_usb_data(false, "conn all sent peris"); 
                //                bsp_board_led_on(led_GREEN);
                //                return;
                //            }
                //            else if(isPhone && !updating_relay_conns && doStar && ble_conn_state_central_conn_count()==doStarCount){// only star
                //                no_scan = true;
                //                if(check_update(h)){
                //                    updated_phone_conn = true;
                //                    updating_relay_conns = update_all_relay_conn_tlv(fast_tlv, 1);
                //                    send_usb_data(false, "!!!doSTARRR!!!"); 
                //                    //if(updating_relay_conns){
                //                    bsp_board_led_on(led_GREEN);
                //                    //    updating_relay_conns = update_all_relay_conn_tlv(devices[h].relay_all_upd_params.tlv, devices[h].relay_all_upd_params.len);
                //                    //}
                //                    return;
                //                }
                                
                //            }
                //        //}
                //}
                //if(updating_relay_conns){
                //    updating_relay_conns = update_all_relay_conn_tlv(devices[h].relay_all_upd_params.tlv, devices[h].relay_all_upd_params.len);
                //}
                

                //if(devices[handle].tp.received_counter != -1 && !PERIS_DO_TP && !PERI_TP_TIMER && sending_rssi){
                //    tp_timer_start(&devices[handle]);
                //}else if(PDR_TCP){
                    
                //}
                
                //if(devices[handle].peri.mode == 4 && devices[handle].curr_interval == devices[handle].conn_params.min_conn_interval){
                //    if(!devices[handle].peri.sent_start_send){
                //        //uint8_t tlv_start_data[2] = {0, DEVICE_ID};
                //        if(handle == RELAY_HANDLE){
                //            send_tlv_from_c(h, tlv_start_data, 2, "Rel start send data");
                //        }else{
                //            tlv_start_data[0] = 1;
                //            send_tlv_from_c(h, tlv_start_data, 2, "Periph start send data");
                //        }
                //        devices[handle].peri.sent_start_send = true;
                //    }
                //}
                //if(devices[handle].peri.mode == 1 && devices[handle].curr_interval <= devices[handle].conn_params.min_conn_interval){
                //    if(!devices[handle].peri.sent_send_fast){
                //        //uint8_t fast_tlv[1] = {11};
                //        if(handle == RELAY_HANDLE){
                //            send_tlv_from_c(h, fast_tlv, 1, "Rel start send fast data");
                //        }else{
                //            send_tlv_from_c(h, fast_tlv, 1, "Periph start send fast data");
                //        }
                //        devices[handle].peri.sent_send_fast = true;
                //    }
                //}
                
                //if(devices[handle].curr_interval > devices[handle].conn_params.min_conn_interval && devices[handle].peri.mode == 1){
                //    set_other_device_op_mode(handle, 1);
                //    update_conn_params(handle, devices[handle].conn_params);
                //    return;
                //}
                //if(updating_relay_conns){//used
                //    updating_relay_conns = update_all_relay_conn_tlv();
                //}

                //if(TEST_RSSI && !devices[handle].tp.doing_const_test && !PERIS_DO_TP){
                //    devices[handle].tp.received_counter = 4;
                //    devices[handle].tp.do_first_ema = true;
                //    send_usb_data(true, "1: RecCounter for %s = 0", devices[handle].name);
                //    //devices[handle].tp.do_first_ema = true;
                //}
                //send_usb_data(true, "ON UPDATE: WaitingConnUpdate = %d, if Min=%d, if Max=%d, if Latency=%d, if Timeout=%d, waiting timeout not 0 = %d", 
                //                devices[handle].waiting_conn_update,
                //                devices[handle].conn_params.max_conn_interval >= new_params.max_conn_interval,
                //                devices[handle].conn_params.min_conn_interval <= new_params.min_conn_interval,
                //                devices[handle].conn_params.slave_latency == new_params.slave_latency,
                //                devices[handle].conn_params.conn_sup_timeout == new_params.conn_sup_timeout,
                //                devices[handle].waiting_list_conn_params.conn_sup_timeout != MSEC_TO_UNITS(0, UNIT_10_MS));

                //if(devices[handle].waiting_conn_update &&
                //    devices[handle].conn_params.max_conn_interval >= new_params.max_conn_interval &&
                //    devices[handle].conn_params.min_conn_interval <= new_params.min_conn_interval &&
                //    devices[handle].conn_params.slave_latency == new_params.slave_latency &&
                //    devices[handle].conn_params.conn_sup_timeout == new_params.conn_sup_timeout){

                //if(devices[handle].waiting_conn_update &&
                //    devices[handle].conn_params.max_conn_interval >= new_params.max_conn_interval &&
                //    devices[handle].conn_params.min_conn_interval <= new_params.min_conn_interval &&
                //    devices[handle].conn_params.slave_latency == new_params.slave_latency &&
                //    devices[handle].conn_params.conn_sup_timeout == new_params.conn_sup_timeout &&
                //    devices[handle].waiting_list_conn_params.conn_sup_timeout != MSEC_TO_UNITS(0, UNIT_10_MS)){
                    
                //    devices[handle].waiting_conn_update = false;

                //    send_usb_data(true, "Followed by WAITING PARAMS, Current connection interval for %s:%d: MIN %d; MAX %d; latency=%d, timeout=%d", 
                //            devices[p_gap_evt->conn_handle].name, p_gap_evt->conn_handle,
                //            new_params.min_conn_interval * UNIT_1_25_MS / 1000, 
                //            new_params.max_conn_interval * UNIT_1_25_MS / 1000, 
                //            p_gap_evt->params.conn_param_update_request.conn_params.slave_latency,
                //            p_gap_evt->params.conn_param_update_request.conn_params.conn_sup_timeout * UNIT_10_MS / 1000);

                //    devices[handle].waiting_conn_update = false;
                //    update_conn_params(handle, devices[handle].waiting_list_conn_params);
                //    return;
                //}
                
                //if(devices[handle].peri.sending_stored && devices[handle].peri.type != 3){
                //    devices[handle].tp.use_pdr = true;
                //}
                

                
                //if(p_gap_evt->conn_handle >= NRF_SDH_BLE_CENTRAL_LINK_COUNT){
                //    send_usb_data(true, "BAD central_evt PARAM_UPDATE, handle=%d too big", handle);
                //    return;
                //}
                //send_usb_data(true, "Current connection interval for %s:%d: MIN %d units, %d in ms; MAX %d units, %d in ms; latency=%d, timeout=%d", 
                //        devices[p_gap_evt->conn_handle].name, p_gap_evt->conn_handle,
                //        new_params.min_conn_interval * UNIT_1_25_MS / 1000, 
                //        new_params.min_conn_interval + new_params.min_conn_interval/4, 
                //        new_params.max_conn_interval * UNIT_1_25_MS / 1000, 
                //        new_params.max_conn_interval + new_params.max_conn_interval/4,
                //        p_gap_evt->params.conn_param_update_request.conn_params.slave_latency,
                //        p_gap_evt->params.conn_param_update_request.conn_params.conn_sup_timeout * UNIT_10_MS / 1000);
                 
                // if(start_scan_wait_conn_param_update){
                //    send_usb_data(false, "Requested parameters for h=%d scan_wait=%d, wait_handle=%d, MIN=%d, MAX=%d, LATENCY=%d, TIMEOUT=%d ; ",
                //        handle, start_scan_wait_conn_param_update, scan_wait_handle_update,
                //        devices[handle].conn_params.min_conn_interval * UNIT_1_25_MS / 1000, 
                //        devices[handle].conn_params.max_conn_interval * UNIT_1_25_MS / 1000, 
                //        devices[handle].conn_params.slave_latency,
                //        devices[handle].conn_params.conn_sup_timeout * UNIT_10_MS / 1000);
                //    //send_usb_data(false, "!!!HAVE TO WAIT FOR SCAN!!! on conn param update"); 
                // }
                // //if(updating_onreq_conn){
                // //   update_onreq_conn(10);
                // //}
                // if(updating_ongather_conn && new_params.min_conn_interval <= devices[handle].conn_params.max_conn_interval){//not used
                //    send_tlv_from_c(handle, tlv_reqData, 2, "Sent start Sending req data");
                //    updating_ongather_conn = update_all_conn_for_gather();
                // }else if(updating_ongather_conn_t23 && new_params.min_conn_interval <= devices[handle].conn_params.max_conn_interval){
                //    updating_ongather_conn_t23 = update_all_conn_for_gather_t23();
                // }


            //}else{
            //    send_usb_data(false, "Failed to get connection parameters for %s:%d, error code: %d", devices[p_gap_evt->conn_handle].name,p_gap_evt->conn_handle, err_code);
            //}
            //if(scan_wait_handle_update == handle && start_scan_wait_conn_param_update){//conn param update first then scan for other device
                
            //    if((devices[handle].conn_params.min_conn_interval * UNIT_1_25_MS / 1000 <= 
            //                                    p_gap_evt->params.conn_param_update.conn_params.min_conn_interval * UNIT_1_25_MS / 1000) 
            //        && (devices[handle].conn_params.max_conn_interval * UNIT_1_25_MS / 1000 >= 
            //                                    p_gap_evt->params.conn_param_update.conn_params.min_conn_interval * UNIT_1_25_MS / 1000))
            //    {
            //        start_scan_wait_conn_param_update = false;
            //        scan_wait_handle_update = 20;
            //        //send_usb_data(false, "!!!NOW YOU CAN SCAN!!!"); 
            //        scan_start();
            //    }
            //}
        }  break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {//Central
            //append_log_to_flash("PHY update request.");
            if(p_gap_evt->conn_handle >= NRF_SDH_BLE_CENTRAL_LINK_COUNT){
                send_usb_data(true, "BAD central_evt PHY_REQUEST, handle=%d too big", handle);
                return;
            }
            ble_gap_phys_t const phys =
            {
                //.rx_phys = BLE_GAP_PHY_AUTO,
                //.tx_phys = BLE_GAP_PHY_AUTO,
                .rx_phys = BLE_GAP_PHY_2MBPS,
                .tx_phys = BLE_GAP_PHY_2MBPS,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            //APP_ERROR_CHECK(err_code);
            check_error("BLE_GAP_EVT_PHY_UPDATE_REQUEST CEN",err_code);
        } break;
        
        case BLE_GAP_EVT_PHY_UPDATE:
            if(p_gap_evt->conn_handle >= NRF_SDH_BLE_CENTRAL_LINK_COUNT){
                send_usb_data(true, "BAD central_evt PHY_UPDATE, handle=%d too big", handle);
                return;
            }
            // PHY update complete
            if (p_ble_evt->evt.gap_evt.params.phy_update.status == BLE_HCI_STATUS_CODE_SUCCESS)
            {
                send_usb_data(false, "PHY updated to TX: %d, RX: %d; for handle:%d, name:%s.",
                             p_ble_evt->evt.gap_evt.params.phy_update.tx_phy,
                             p_ble_evt->evt.gap_evt.params.phy_update.rx_phy,
                             p_gap_evt->conn_handle, devices[p_gap_evt->conn_handle].name);
            }
            else
            {
                send_usb_data(false, "PHY update failed with handle:%d, name:%s.", p_gap_evt->conn_handle, devices[p_gap_evt->conn_handle].name);
            }
            break;

        case BLE_GATTC_EVT_TIMEOUT://Central
            if(p_gap_evt->conn_handle >= NRF_SDH_BLE_CENTRAL_LINK_COUNT){
                send_usb_data(true, "BAD central_evt GATTC_TIMEOUT, handle=%d too big", handle);
                return;
            }
            // Disconnect on GATT Client timeout event.
            send_usb_data(false, "Time out from handle: %d, Peri tp:%d", p_ble_evt->evt.gattc_evt.conn_handle, tx_range[devices[handle].tp.tx_idx]);
            //if(peri_relay_info[p_ble_evt->evt.gattc_evt.conn_handle].rssi == -40){
            //    if(peri_relay_info[p_ble_evt->evt.gattc_evt.conn_handle].timeoutCtr == 3){
            //        send_usb_data(false, "Timeout for handle %d ",p_ble_evt->evt.gattc_evt.conn_handle);
            //        peri_relay_info[p_ble_evt->evt.gattc_evt.conn_handle].timeoutCtr = 0;
            //        err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
            //                                         BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            //        //APP_ERROR_CHECK(err_code);
            //        check_error("BLE_GATTC_EVT_TIMEOUT CEN",err_code);
            //    }else{
            //        peri_relay_info[p_ble_evt->evt.gattc_evt.conn_handle].timeoutCtr++;
            //        tlv_rssiChange[2] = (uint8_t) -20;
            //        send_usb_data(false, "RSSI -40 for handle %d timeout is %d", p_ble_evt->evt.gattc_evt.conn_handle, peri_relay_info[p_ble_evt->evt.gattc_evt.conn_handle].timeoutCtr);
            //        send_tlv_from_c(p_ble_evt->evt.gattc_evt.conn_handle, false, tlv_rssiChange, 3, "RSSI change to -20");
            //    }
            //    break;
            //}
            send_usb_data(false, "GATT Client Timeout. Handle %d", p_ble_evt->evt.gatts_evt.conn_handle);
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            //APP_ERROR_CHECK(err_code);
            //reset_device_full(p_ble_evt->evt.gatts_evt.conn_handle);
            check_error("BLE_GATTC_EVT_TIMEOUT CEN",err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT://Central
            if(p_gap_evt->conn_handle >= NRF_SDH_BLE_CENTRAL_LINK_COUNT){
                send_usb_data(true, "BAD central_evt GATTS_TIMEOUT, handle=%d too big", handle);
                return;
            }
            // Disconnect on GATT Server timeout event.
            send_usb_data(false, "GAT SERVER Time out from handle: %d, Peri tp:%d", p_ble_evt->evt.gattc_evt.conn_handle, tx_range[devices[handle].tp.tx_idx]);
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            //APP_ERROR_CHECK(err_code);
            //reset_device_full(p_ble_evt->evt.gatts_evt.conn_handle);
            check_error("BLE_GATTS_EVT_TIMEOUT CEN",err_code);
            break;

        default:
            // No implementation needed.
            break;
    }
}

ret_code_t gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *) DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    //if(IS_RELAY || (SYSTEM_COMM_MODE==3 && !SEND_TYPE)){
    //    gap_conn_params.min_conn_interval = MSEC_TO_UNITS(20, UNIT_1_25_MS);
    //    //if(!IS_RELAY){
    //    //    gap_conn_params.max_conn_interval = MSEC_TO_UNITS(NRF_SDH_BLE_GAP_EVENT_LENGTH*16, UNIT_1_25_MS);
    //    //}else{
    //    //    gap_conn_params.max_conn_interval = MSEC_TO_UNITS(600, UNIT_1_25_MS);
    //    //}
    //    //gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    //    gap_conn_params.max_conn_interval = conn_params_periodic.max_conn_interval;//MAX_CONN_INTERVAL;
    //    gap_conn_params.slave_latency     = 0;
    //    gap_conn_params.conn_sup_timeout  =  MSEC_TO_UNITS(4000, UNIT_10_MS);
    //}else{
    //    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    //    gap_conn_params.max_conn_interval = conn_params_periodic.max_conn_interval;
    //    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    //    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;
    //}
    if(IS_RELAY){
        gap_conn_params.min_conn_interval = conn_params_conn_phone_wait.min_conn_interval;
        gap_conn_params.max_conn_interval = conn_params_conn_phone_wait.max_conn_interval;
        gap_conn_params.slave_latency     = conn_params_conn_phone_wait.slave_latency;
        gap_conn_params.conn_sup_timeout  = conn_params_conn_phone_wait.conn_sup_timeout;
    }else{
        gap_conn_params.min_conn_interval = conn_params_conn_phone_wait.min_conn_interval;
        gap_conn_params.max_conn_interval = conn_params_conn_phone_wait.max_conn_interval;
        gap_conn_params.slave_latency     = conn_params_conn_phone_wait.slave_latency;
        gap_conn_params.conn_sup_timeout  = conn_params_conn_phone_wait.conn_sup_timeout;
        //gap_conn_params.min_conn_interval = conn_params_idle.min_conn_interval;   // used conn_params_periodic before
        //gap_conn_params.max_conn_interval = conn_params_idle.max_conn_interval;
        //gap_conn_params.slave_latency     = conn_params_idle.slave_latency;
        //gap_conn_params.conn_sup_timeout  = conn_params_idle.conn_sup_timeout;
    }
    //gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    //gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    //////gap_conn_params.min_conn_interval = MSEC_TO_UNITS(NRF_SDH_BLE_GAP_EVENT_LENGTH*10, UNIT_1_25_MS);
    //////gap_conn_params.max_conn_interval = MSEC_TO_UNITS(NRF_SDH_BLE_GAP_EVENT_LENGTH*13, UNIT_1_25_MS);
    //gap_conn_params.slave_latency     = SLAVE_LATENCY;
    ////gap_conn_params.slave_latency     = 0;
    //gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;
    //gap_conn_params.conn_sup_timeout  =  MSEC_TO_UNITS(4000, UNIT_10_MS);

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    //APP_ERROR_CHECK(err_code);
    check_error("Gap Param Init error:%d", err_code);
    return err_code;
}

ret_code_t change_gap_params_test_rssi(void){
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    //if(forPhone){
    //    gap_conn_params.min_conn_interval = conn_params_on_gather_sink.min_conn_interval;//MSEC_TO_UNITS(20, UNIT_1_25_MS);
    //    gap_conn_params.max_conn_interval = conn_params_periodic.max_conn_interval;//MSEC_TO_UNITS(NRF_SDH_BLE_GAP_EVENT_LENGTH*16, UNIT_1_25_MS);
    //    gap_conn_params.slave_latency     = 2;
    //    gap_conn_params.conn_sup_timeout  =  MSEC_TO_UNITS(10000, UNIT_10_MS);
    //}else{
    //    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    //    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    //    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    //    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;
    //}
    gap_conn_params = conn_params_test_rssi;
    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    send_usb_data(false, "Change Gap Conn params for TEST RSSI err=%d",err_code);
    //send_usb_data(true, "Changed GAP CONN params forPhone=%d, err=%d", forPhone, err_code);
    send_usb_data(false, "Changed GAP CONN params: min=%d, max=%d, latency=%d, timeout=%d",
                                gap_conn_params.min_conn_interval * UNIT_1_25_MS / 1000,
                                gap_conn_params.max_conn_interval * UNIT_1_25_MS / 1000,
                                gap_conn_params.slave_latency,
                                gap_conn_params.conn_sup_timeout * UNIT_10_MS / 1000);
    return err_code;
}

ret_code_t change_gap_params(bool forPhone){
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    if(forPhone){
        if(DEVICE_ID != 'V' && DEVICE_ID != 'G'){
            gap_conn_params.min_conn_interval = conn_params_conn_phone_wait.min_conn_interval;
            gap_conn_params.max_conn_interval = conn_params_conn_phone_wait.max_conn_interval;
            gap_conn_params.slave_latency     = conn_params_conn_phone_wait.slave_latency;
            gap_conn_params.conn_sup_timeout  = conn_params_conn_phone_wait.conn_sup_timeout;
        }else{
            gap_conn_params.min_conn_interval = conn_params_idle.min_conn_interval;
            gap_conn_params.max_conn_interval = conn_params_idle.max_conn_interval;
            gap_conn_params.slave_latency     = conn_params_idle.slave_latency;
            gap_conn_params.conn_sup_timeout  = conn_params_idle.conn_sup_timeout;
            //gap_conn_params.min_conn_interval = conn_params_phone.min_conn_interval;//MSEC_TO_UNITS(20, UNIT_1_25_MS);
            //gap_conn_params.max_conn_interval = conn_params_phone.max_conn_interval;//MSEC_TO_UNITS(NRF_SDH_BLE_GAP_EVENT_LENGTH*16, UNIT_1_25_MS);
            //gap_conn_params.slave_latency     = conn_params_phone.slave_latency;
            //gap_conn_params.conn_sup_timeout  =  MSEC_TO_UNITS(5000, UNIT_10_MS);
        }
    }else{
        //gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
        //gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
        //gap_conn_params.slave_latency     = SLAVE_LATENCY;
        //gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;
        gap_conn_params.min_conn_interval = conn_params_conn_phone_wait.min_conn_interval;
        gap_conn_params.max_conn_interval = conn_params_conn_phone_wait.max_conn_interval;
        gap_conn_params.slave_latency     = conn_params_conn_phone_wait.slave_latency;
        gap_conn_params.conn_sup_timeout  = conn_params_conn_phone_wait.conn_sup_timeout;
    }
    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    check_error("Change Gap Conn params",err_code);
    //send_usb_data(true, "Changed GAP CONN params forPhone=%d, err=%d", forPhone, err_code);
    //send_usb_data(false, "Changed GAP CONN params: min=%d, max=%d, latency=%d, timeout=%d",
    //                            gap_conn_params.min_conn_interval * UNIT_1_25_MS / 1000,
    //                            gap_conn_params.max_conn_interval * UNIT_1_25_MS / 1000,
    //                            gap_conn_params.slave_latency,
    //                            gap_conn_params.conn_sup_timeout * UNIT_10_MS / 1000);
    return err_code;
}

ret_code_t services_init(void)
{
    ret_code_t err_code;
    ble_nus_init_t nus_init;
    nrf_ble_qwr_init_t qwr_init = {0};

    // Initialize Queued Write Module.
    qwr_init.error_handler = nrf_qwr_error_handler;
    
    for (uint32_t i = 0; i < NRF_SDH_BLE_TOTAL_LINK_COUNT; i++)
    {
        err_code = nrf_ble_qwr_init(&m_qwr[i], &qwr_init);
        APP_ERROR_CHECK(err_code);
    }

    // Initialize NUS.
    memset(&nus_init, 0, sizeof(nus_init));

    nus_init.data_handler = nus_data_handler;

    err_code = ble_nus_init(&m_nus, &nus_init);
    APP_ERROR_CHECK(err_code);
    return err_code;
}

/**@brief Function for the SoftDevice initialization.
 *
 * @details This function initializes the SoftDevice and the BLE event interrupt.
 */
ret_code_t ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    if(err_code != NRF_SUCCESS){
        bsp_board_led_on(led_GREEN);
    }
    APP_ERROR_CHECK(err_code);

    //err_code = sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);
    //send_usb_data(false, "sd_power_dcdc_mode_set enabled=%d; err=%d", (NRF_POWER->DCDCEN || NRF_POWER->DCDCEN0), err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    if(err_code != NRF_SUCCESS){
        bsp_board_led_on(led_GREEN);
    }
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    if(err_code != NRF_SUCCESS){
        bsp_board_led_on(led_GREEN);
    }
    APP_ERROR_CHECK(err_code);
    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler2, NULL);
    return err_code;
}

/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    uint32_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            //APP_ERROR_CHECK(err_code);
            check_error("on_adv_evt",err_code);
            break;
        case BLE_ADV_EVT_IDLE:
            //sleep_mode_enter();
            break;
        default:
            break;
    }
}

ret_code_t advertising_init(void)
{
    ret_code_t err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance      = true;
    err_code = sd_ble_gap_appearance_set(2);//BLE_APPEARANCE_UNKNOWN);

    init.advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    //init.advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    //init.advdata.uuids_complete.p_uuids  = m_adv_uuids;

    //ADV: NEW
    //ble_advdata_manuf_data_t manuf_data;
    ////manuf_data.company_identifier = 0x0059;
    ////uint8_t data_arr[1] = {emergency_bit};
    ////manuf_data.data.p_data = data_arr[0];
    ////manuf_data.data.size = sizeof(data_arr);
    ////init.advdata.p_manuf_specific_data = &manuf_data;

    //if(RELAY){//NRF_SDH_BLE_CENTRAL_LINK_COUNT
    //    uint8_t data_arr[NRF_SDH_BLE_CENTRAL_LINK_COUNT] = {0};
    //    for(int i=0; i< NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
    //        data_arr[i] = (uint8_t) (peri_setup[i].avg_rssi +128);
    //    }
    //    manuf_data.company_identifier = 0x0059;  // Replace with your company identifier
    //    manuf_data.data.p_data = data_arr;
    //    manuf_data.data.size = sizeof(data_arr);
    //}else{
    //    uint8_t data_arr[1] = {emergency_bit};
    //    manuf_data.company_identifier = 0x0059;  // Replace with your company identifier
    //    manuf_data.data.p_data = data_arr;
    //    manuf_data.data.size = sizeof(data_arr);
    //}
    //init.advdata.p_manuf_specific_data = &manuf_data;
    //

    init.config.ble_adv_fast_enabled  = true;
    if(doStar){
        init.config.ble_adv_fast_interval = 1600;
    }else{
        if(DEVICE_ID == 'C'){
            init.config.ble_adv_fast_interval = 240   /**< The advertising interval (in units of 0.625 ms. This value corresponds to 187.5 ms). */;
        }else{
            init.config.ble_adv_fast_interval = 240;//192;//120s, swin=50, sint=100  //512 = 320s //scan int 160, window 80/**< The advertising interval (in units of 0.625 ms. This value corresponds to 30 ms). */;
        }
    }
    init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;
    init.evt_handler = on_adv_evt;


    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);
    check_error("Advertising init Error ",err_code);
    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
    check_error("Advertising init SET Error ",err_code);
    //change_adv_tx(default_tx_idx);
    return err_code;
}

void delete_bonds(void)
{
    ret_code_t err_code;

    send_usb_data(false, "Erase bonds!");

    err_code = pm_peers_delete();
    //APP_ERROR_CHECK(err_code);
    check_error("delete_bonds",err_code);
}

uint32_t adv_start(bool erase_bonds)
{
    uint32_t err_code;
    //if (erase_bonds == true)
    //{
    //    //delete_bonds();
    //    // Advertising is started by PM_EVT_PEERS_DELETE_SUCCEEDED event.
    //    return 10;
    //}
    
    err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
    send_usb_data(false, "advertising start e:%d", err_code);
    //APP_ERROR_CHECK(err_code);
    return err_code;
    //}
}

void stop_adv(void){
    ret_code_t ret;
    ret = sd_ble_gap_adv_stop(m_advertising.adv_handle);
    //APP_ERROR_CHECK(ret);
    send_usb_data(false, "advertising stop e:%d", ret);
}

ret_code_t update_adv(){
    ret_code_t err_code;
    ble_advdata_t advdata;
    //ADV
    //ble_advdata_manuf_data_t manuf_data;
    
    //if(RELAY){
    //    uint8_t data_arr[6] = {0};
    //    for(int i=0; i< NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
    //        data_arr[i] = (uint8_t) (peri_setup[i].avg_rssi+128);
    //    }
    //    manuf_data.company_identifier = 0x0059;  // Replace with your company identifier
    //    manuf_data.data.p_data = data_arr;
    //    manuf_data.data.size = sizeof(data_arr);
    //}else{
    //    uint8_t data_arr[1] = {emergency_bit};
    //    manuf_data.company_identifier = 0x0059;  // Replace with your company identifier
    //    manuf_data.data.p_data = data_arr;
    //    manuf_data.data.size = sizeof(data_arr);
    //}

    // Prepare updated advertising data
    memset(&advdata, 0, sizeof(advdata));
    advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance      = false;
    advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    //ADV
    //advdata.p_manuf_specific_data   = &manuf_data;

    // Update advertising data while advertising
    err_code = ble_advertising_advdata_update(&m_advertising, &advdata, NULL);
    check_error("Adv update", err_code);
}

ret_code_t update_emergency_bit(bool isEmerg){
    //ret_code_t ret;
    //stop_adv();
    if(isEmerg){
        emergency_bit = 0x01;
    }else{
        emergency_bit = 0x00;
    }
    //check_error("Update adv init",advertising_init());
    //check_error("Update adv start",adv_start());
}

void change_adv_tx(int8_t power){
    ret_code_t ret;
    //ret = sd_ble_gap_adv_stop(m_advertising.adv_handle);
    //APP_ERROR_CHECK(err_code);
    //send_usb_data(false, "advertising stop e:%d", ret);
    ret = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV, m_advertising.adv_handle, power);
    //APP_ERROR_CHECK(ret);
    send_usb_data(false, "advertising pow set to %d, e:%d", power, ret);
    //adv_start(false);
}
int connecting_handle = 0;
static void on_ble_peripheral_evt(ble_evt_t const * p_ble_evt)
{
    uint32_t err_code;
    uint16_t handle = p_ble_evt->evt.gap_evt.conn_handle;
    ble_gap_evt_t const * p_gap_evt = &p_ble_evt->evt.gap_evt;
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            //send_usb_data(false, "Connected");
            err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);
            send_usb_data(false, "Connected to handle:%d", handle);
            
            //m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            //err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            // Assign connection handle to the QWR module.
            multi_qwr_conn_handle_assign(handle);
            //for (uint8_t i = 0; i < NRF_SDH_BLE_PERIPHERAL_LINK_COUNT; i++)
            //{
            //    if (m_conn_handles[i] == BLE_CONN_HANDLE_INVALID)
            //    {
            //        adv_start(false);
            //        break;
            //    }
            //}
            //APP_ERROR_CHECK(err_code);
            check_error("BLE_GAP_EVT_CONNECTED PER",err_code);
            devices[handle].cent.connecting = true;
            connecting_handle = handle;
            ble_gap_phys_t phys;
            phys.rx_phys = BLE_GAP_PHY_2MBPS; // Set RX to 2M PHY
            phys.tx_phys = BLE_GAP_PHY_2MBPS; // Set TX to 2M PHY

            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            send_usb_data(false, "Requested 2MBPS PHY, err:%d", err_code);

            //if(ble_conn_state_peripheral_conn_count() < NRF_SDH_BLE_PERIPHERAL_LINK_COUNT){
            //    adv_start(false);
            //}
            break;

        case BLE_GAP_EVT_DISCONNECTED://Peripheral
             //send_usb_data(false, "Disconnected");
             send_usb_data(true, "DISCONNECTED from central %s (reason: 0x%x)",
                             devices[handle].name,
                             p_gap_evt->params.disconnected.reason);
            devices[connecting_handle].cent.connecting = false;
            //m_conn_handle = BLE_CONN_HANDLE_INVALID;
            //Propose change
            //uint16_t handle = p_ble_evt->evt.gap_evt.conn_handle;
            //m_conn_handles[handle] = BLE_CONN_HANDLE_INVALID;
            err_code = sd_ble_gap_rssi_stop(handle);
            //send_usb_data(false, "Stopped RSSI for Cent handle:%d e:%d",handle, err_code);
            /////
            devices[handle].connected = false;
            if(devices[handle].id!=NRF_SDH_BLE_TOTAL_LINK_COUNT){
                if(devices[handle].cent.isPhone &&  IS_RELAY){
                    
                    PHONE_HANDLE = NRF_SDH_BLE_TOTAL_LINK_COUNT;
                    connectedToPhone = false;
                    if(sending_relay_tlv){
                        sending_relay_tlv = false;
                        for(int i=0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
                            if(!devices[i].peri.is_sending_on_req && devices[i].connected){
                                if(devices[i].peri.last >= 0 && devices[i].peri.last <= 255){
                                    tlv_stopData[2] = (uint8_t) devices[i].peri.last;
                                    tlv_stopData[1] = (uint8_t) devices[i].peri.batch_number;
                                }else if(devices[i].peri.last < 0){
                                    tlv_stopData[2] = 0;
                                    if(devices[i].peri.batch_number < 0){
                                        tlv_stopData[1] = 0;
                                    }
                                }
                                //send_tlv_usb(false, "STOP TLV with last", tlv_stopData, 3,3,i);
                                tlv_stopData[0] = 4;
                                //tlv_stopData[1] = 0;
                                //send_tlv_usb(false, "STOP TLV final", tlv_stopData, 3,3,i);
                                devices[i].peri.stopped = true;
                                send_tlv_from_c(i,tlv_stopData,(uint16_t) 3, "STOP");
                            }
                        }
                    }
                    //if(!IS_RELAY && sending_tlv_chunks){
                    //    uint8_t tlv_discPhone[2] = {3, 0};
                    //    for(int i=NRF_SDH_BLE_CENTRAL_LINK_COUNT; i<NRF_SDH_BLE_TOTAL_LINK_COUNT; i++){
                    //        if(devices[i].id == 'C' || devices[i].id == 'D'){
                    //            send_tlv_from_p(i, tlv_discPhone, 2);
                    //            CHOSEN_HANDLE = i;
                    //        }
                    //    }
                    //}
                    //reset_not_used_devices_conn_update();
                    //reset_data_gather_ble_nus();
                }
                
                //if(SEND_TYPE){
                //    //find_best_cent();
                //}else{
                //    package_tail = (package_tail+255)%package_storage_size;
                //}
                if(CHOSEN_HANDLE == p_gap_evt->conn_handle){
                    bsp_board_leds_off();
                    bsp_board_led_on(led_RED);
                }
            }

            if(send_test_rssi && handle == NRF_SDH_BLE_CENTRAL_LINK_COUNT){
                
            }
            
            if(handle == CHOSEN_HANDLE){
                CHOSEN_HANDLE = 255;
                sending_tlv_chunks = false;
                //send_usb_data(false, "Stopped sending req data to handle:%d %s",handle, devices[handle].name);
            }
            //reset_device_full(handle);
            if(!connectedToPhone && handle == RELAY_HANDLE){
                adv_start(false);
            }
            send_usb_data(false, "Disconnected thread %s", devices[handle].name);
            break;

        case BLE_GAP_EVT_RSSI_CHANGED:
        {//Peripheral
            ble_gap_evt_t const * p_gap_evt = &p_ble_evt->evt.gap_evt;
            int8_t rssi = p_ble_evt->evt.gap_evt.params.rssi_changed.rssi;
            //uint8_t channel = p_ble_evt->evt.gap_evt.params.rssi_changed.ch_index;

            //uint16_t handle = p_gap_evt->conn_handle;
            //if(devices[handle].tp.received_counter != -1 && PERIS_DO_TP){
            //    uint8_t new_rssi[3] = {99, (uint8_t) rssi, channel};
            //    add_to_device_tlv_strg(handle, new_rssi, 3);
            //}

            //if(devices[handle].tp.received_counter != -1 && handle != PHONE_HANDLE && !PERIS_DO_TP){
                //do_rssi_ema2(handle, rssi);
            //}
            devices[handle].tp.ema_rssi = (float) (2/6) * (float)rssi + (1.0-2/6) * devices[handle].tp.ema_rssi;
        }
        break;

        case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
        {   //Peripheral
            ble_gap_evt_t const * p_gap_evt = &p_ble_evt->evt.gap_evt;

            //if( p_gap_evt->params.conn_param_update_request.conn_params.min_conn_interval * UNIT_1_25_MS / 1000 < 50 && !sending_stored_tlv){
            //    err_code = sd_ble_gap_conn_param_update(p_gap_evt->conn_handle,
            //                                &conn_params_conn_phone_wait);
            //    send_usb_data(true, "got param req, suggest phone wait params");
            //}
            //// Accept parameters requested by peer.
            //err_code = sd_ble_gap_conn_param_update(p_gap_evt->conn_handle,
            //                            &p_gap_evt->params.conn_param_update.conn_params);
            ////APP_ERROR_CHECK(err_code);
            //check_error("BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST PERI",err_code);
            if(devices[handle].connected){
                send_usb_data(false, "Got connParam Request from %s: min=%d, max=%d, latency=%d, timeout=%d", 
                                devices[handle].name,
                                p_gap_evt->params.conn_param_update_request.conn_params.min_conn_interval * UNIT_1_25_MS / 1000,
                                p_gap_evt->params.conn_param_update_request.conn_params.max_conn_interval * UNIT_1_25_MS / 1000,
                                p_gap_evt->params.conn_param_update_request.conn_params.slave_latency,
                                p_gap_evt->params.conn_param_update_request.conn_params.conn_sup_timeout * UNIT_10_MS / 1000);
            }else{
                send_usb_data(false, "Got connParam Request from NEW Central: min=%d, max=%d, latency=%d, timeout=%d",
                                p_gap_evt->params.conn_param_update_request.conn_params.min_conn_interval * UNIT_1_25_MS / 1000,
                                p_gap_evt->params.conn_param_update_request.conn_params.max_conn_interval * UNIT_1_25_MS / 1000,
                                p_gap_evt->params.conn_param_update_request.conn_params.slave_latency,
                                p_gap_evt->params.conn_param_update_request.conn_params.conn_sup_timeout * UNIT_10_MS / 1000);
            }
            //if(send_test_rssi){
            //  if(p_gap_evt->params.conn_param_update_request.conn_params.min_conn_interval * UNIT_1_25_MS / 1000 <= conn_params_test_rssi.max_conn_interval * UNIT_1_25_MS / 1000 ||
            //            p_gap_evt->params.conn_param_update_request.conn_params.min_conn_interval * UNIT_1_25_MS / 1000 <= 220){
            //    //sending_rssi = true;
            //    //send_test_rssi_tlv();
            //    err_code = sd_ble_gap_conn_param_update(p_gap_evt->conn_handle,
            //                                &p_gap_evt->params.conn_param_update.conn_params);
            //    //devices[handle].conn_params.min_conn_interval = p_gap_evt->params.conn_param_update.conn_params.min_conn_interval;
            //    //devices[handle].conn_params.max_conn_interval = p_gap_evt->params.conn_param_update.conn_params.max_conn_interval;
            //    //devices[handle].conn_params.slave_latency = p_gap_evt->params.conn_param_update.conn_params.slave_latency;
            //    //devices[handle].conn_params.conn_sup_timeout = p_gap_evt->params.conn_param_update.conn_params.conn_sup_timeout;
            //    send_usb_data(true, "Accept params");
            //    //rssi_timer_start();
            //    return;
            //  }else{
            //      send_usb_data(true, "Request new params");
            //      update_conn_params(handle, conn_params_test_rssi);
            //      return;
            //  }
            //}
            //if(devices[p_gap_evt->conn_handle].id == 'P' &&
            //    //devices[handle].updated_conn && 
            //    p_gap_evt->params.conn_param_update_request.conn_params.min_conn_interval * UNIT_1_25_MS / 1000 > devices[handle].conn_params.max_conn_interval * UNIT_1_25_MS / 1000){
                   
            //        send_usb_data(true, "Update Conn param IF:0, request_min=%d, response_max=%d", 
            //              p_gap_evt->params.conn_param_update_request.conn_params.min_conn_interval * UNIT_1_25_MS / 1000, 
            //              devices[handle].conn_params.max_conn_interval * UNIT_1_25_MS / 1000);
            //        update_conn_params(handle, devices[handle].conn_params);
            //}else{
                // Accept parameters requested by peer.
                err_code = sd_ble_gap_conn_param_update(p_gap_evt->conn_handle,
                                            &p_gap_evt->params.conn_param_update.conn_params);
                //devices[handle].conn_params.min_conn_interval = p_gap_evt->params.conn_param_update.conn_params.min_conn_interval;
                //devices[handle].conn_params.max_conn_interval = p_gap_evt->params.conn_param_update.conn_params.max_conn_interval;
                //devices[handle].conn_params.slave_latency = p_gap_evt->params.conn_param_update.conn_params.slave_latency;
                //devices[handle].conn_params.conn_sup_timeout = p_gap_evt->params.conn_param_update.conn_params.conn_sup_timeout;
                //APP_ERROR_CHECK(err_code);
                check_error("BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST PERI",err_code);
            //}
        } break;
        case BLE_GAP_EVT_CONN_PARAM_UPDATE:
        {//Peripheral
            //err_code = sd_ble_gap_conn_param_get(p_gap_evt->conn_handle, &conn_params);
            ble_gap_evt_t const * p_gap_evt = &p_ble_evt->evt.gap_evt;
            ble_gap_conn_params_t new_params = p_gap_evt->params.conn_param_update.conn_params;
            //if (err_code == NRF_SUCCESS) {
            
            devices[handle].curr_interval = new_params.max_conn_interval * UNIT_1_25_MS / 1000;
            devices[handle].curr_sl = new_params.slave_latency;

            //if(devices[handle].tp.received_counter != -1 && PERIS_DO_TP){
            //if(PERIS_DO_TP){
                //tp_timer_start(&devices[handle]);
                //send_usb_data(true, "ON UPDATE: TP timer is %d ms", devices[handle].curr_interval*(devices[handle].curr_sl + 1 + 1)*3);
                //device_info_t *dev = &devices[handle];
                //send_usb_data(true, "ON UPDATE: TP timer pointer for %s is %d ms", dev->name, dev->curr_interval*(dev->curr_sl + 1 + 1)*3);
            //} 

            send_usb_data(true, "Current connection interval for %s:%d: MIN %d ms, SL %d, PDR?=%d", 
                    devices[p_gap_evt->conn_handle].name, p_gap_evt->conn_handle,
                    new_params.min_conn_interval * UNIT_1_25_MS / 1000,
                    new_params.slave_latency,
                    devices[handle].tp.use_pdr);
            //if(check_update(h)){
            //    if(updating_relay_conns){
            //        updating_relay_conns = update_all_relay_conn_tlv(devices[h].relay_all_upd_params.tlv, devices[h].relay_all_upd_params.len);
            //    }
            //    return;
            //}
            
            //if(false){
            //    send_usb_data(true, "ON UPDATE: WaitingConnUpdate = %d, if Min=%d, if Max=%d, if Latency=%d, if Timeout=%d", 
            //                    devices[handle].waiting_conn_update,
            //                    devices[handle].conn_params.max_conn_interval >= new_params.max_conn_interval,
            //                    devices[handle].conn_params.min_conn_interval <= new_params.min_conn_interval,
            //                    devices[handle].conn_params.slave_latency == new_params.slave_latency,
            //                    devices[handle].conn_params.conn_sup_timeout == new_params.conn_sup_timeout);

            //    if(devices[handle].waiting_conn_update &&
            //        devices[handle].conn_params.max_conn_interval >= new_params.max_conn_interval &&
            //        devices[handle].conn_params.min_conn_interval <= new_params.min_conn_interval &&
            //        devices[handle].conn_params.slave_latency == new_params.slave_latency &&
            //        devices[handle].conn_params.conn_sup_timeout == new_params.conn_sup_timeout){

            //        devices[handle].waiting_conn_update = false;
                
            //        if(package_storage[0].data[10] == 'A' && waiting_conn_update_next_test && send_test_rssi){
            //            waiting_conn_update_next_test = false;
            //            rssi_timer_start();
            //        }

            //        if(devices[handle].waiting_list_conn_params.conn_sup_timeout != MSEC_TO_UNITS(0, UNIT_10_MS)){
            //            send_usb_data(true, "Followed by WAITING PARAMS, Current connection interval for %s:%d: MIN %d; MAX %d; latency=%d, timeout=%d", 
            //                devices[p_gap_evt->conn_handle].name, p_gap_evt->conn_handle,
            //                new_params.min_conn_interval * UNIT_1_25_MS / 1000, 
            //                new_params.max_conn_interval * UNIT_1_25_MS / 1000, 
            //                new_params.slave_latency,
            //                new_params.conn_sup_timeout * UNIT_10_MS / 1000);

                
            //                update_conn_params(handle, devices[handle].waiting_list_conn_params);
            //                return;
            //        }
            //    }
            //}

            
            
            if(devices[handle].id == 'P'){// && !sending_periodic_tlv){
                      //&& (count_onreq != 0 && !SEND_TYPE && !IS_RELAY &&
                      //      p_gap_evt->params.conn_param_update.conn_params.min_conn_interval * UNIT_1_25_MS / 1000 > on_request_conn_params.max_conn_interval * UNIT_1_25_MS / 1000)
                      //&& (count_onreq != 0 && !SEND_TYPE && !IS_RELAY && p_gap_evt->params.conn_param_update.conn_params.min_conn_interval > on_request_conn_params.max_conn_interval)
                      //&& (count_onreq == 0 && p_gap_evt->params.conn_param_update.conn_params.min_conn_interval * UNIT_1_25_MS / 1000 > peri_request_to_phone_conn_params.max_conn_interval * UNIT_1_25_MS / 1000)){
                
                //if(new_params.min_conn_interval * UNIT_1_25_MS / 1000 > devices[handle].conn_params.max_conn_interval * UNIT_1_25_MS / 1000 && sending_stored_tlv){
                //   send_usb_data(true, "Ketujaaaaa");
                //    //send_usb_data(true, "Update Conn param IF:1, update_min=%d, should_be_max=%d", 
                //    //      new_params.min_conn_interval * UNIT_1_25_MS / 1000, 
                //    //      devices[handle].conn_params.max_conn_interval * UNIT_1_25_MS / 1000);
                //    update_conn_params(handle, devices[handle].conn_params);
                //}else 
                if(waiting_to_send_phone){
                    //if(devices[handle].curr_interval > conn_params_phone.max_conn_interval){
                    //    update_conn_params(handle, conn_params_phone);
                    //    return;
                    //}
                    //if(IS_RELAY && updating_relay_conns){
                    //    send_usb_data(true, "IS_RELAY && updating_relay_conns");
                    //    updating_relay_conns = update_all_relay_conn();
                    //}
                    //if(!waiting_to_send_phone && sending_stored_tlv && devices[handle].curr_interval == devices[handle].phone_params.max_conn_interval){
                    //    send_usb_data(true, "tart[3] = {0,0,0};");
                    //    uint8_t tlv_phoneReadyToStart[3] = {0,0,0};
                    //    send_tlv_from_p(handle, tlv_phoneReadyToStart, 3);
                    //}
                }else if(devices[handle].cent.start_send_after_conn_update){
                    //send_usb_data(true, "vices[handle].cent.start_send_after_conn_updat");
                    //uint8_t tlv_phoneReadyToStart[2] = {0,0};
                    //if(IS_RELAY){
                    //    devices[handle].cent.start_send_after_conn_update = false;
                    //    send_tlv_from_p(handle, tlv_phoneReadyToStart, 1);
                    //    uint8_t tlv_phoneScanPeris[10] = {8};
                    //        int count_phoneScanPeris = 1;
                    //        for(int i = 0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
                    //            if(devices[i].connected && 
                    //                (devices[i].peri.type == 1 || devices[i].peri.type == 2)
                    //                && !devices[i].peri.askedAdvPhone)
                    //                //devices[handle].tp.own_idx <= devices[i].tp.max_tx_idx ||
                    //                //devices[handle].tp.ema_rssi + tx_range[devices[handle].tp.own_idx] - tx_range[devices[i].tp.max_tx_idx] > -70 )//||
                    //                ////devices[handle].tp.ema_rssi> -72 ||
                    //                ////devices[handle].tp.last_rssi > -72)// TP check
                    //                //&& (devices[i].peri.type == 1 || devices[i].peri.type == 2)
                    //                //&& !devices[i].peri.askedAdvPhone)  // allowed types for direct sending
                    //            {
                    //                //if(!devices[i].peri.isPeriodic){
                    //                send_tlv_from_c(i, tlv_advertiseSink, 4, "Type 1 start advertising");
                    //                send_usb_data(false, "%s Start adv for phone", devices[i].name);
                    //                //count_onreq_not_phone--;
                    //                tlv_phoneScanPeris[count_phoneScanPeris] = devices[i].id;
                    //                count_phoneScanPeris++;
                    //                //relay_info.count_type1_sink++;
                    //                send_usb_data(false, "Sent adv for Phone to %s, count phone scan peris = %d", devices[i].name, relay_info.count_type1_sink);
                    //                devices[i].peri.chosenRelay = false;
                    //                devices[i].peri.askedAdvPhone = true;
                    //            }
                    //        }

                    //        send_usb_data(false, "Send PhoneScanPeris, count phone scan peris = %d, t1_REL=%d, t1_Sink", count_phoneScanPeris, relay_info.count_type1, relay_info.count_type1_sink);
                    //        if(count_phoneScanPeris > 1){
                    //        //update_conn_params(handle, phone_scan_for_peris_conn_params);
                    //            send_tlv_from_p(handle, tlv_phoneScanPeris, count_phoneScanPeris);
                    //            //devices[handle].conn_params = conn_params_conn_phone_wait;
                            
                    //        }else{
                    //            send_tlv_from_p(handle, tlv_phoneScanPeris, 1);
                    //            //devices[handle].conn_params = conn_params_phone;
                    //            //update_conn_params(handle, conn_params_phone);
                    //            //devices[handle].cent.start_send_after_conn_update = true;
                    //        }
                    //}else{
                    //    devices[handle].cent.start_send_after_conn_update = false;
                    //    send_tlv_from_p(handle, tlv_phoneReadyToStart, 2);
                    //}
                    //waiting_to_send_phone = true;
                    ////ble_gap_phys_t phys;
                    ////phys.rx_phys = BLE_GAP_PHY_2MBPS; // Set RX to 2M PHY
                    ////phys.tx_phys = BLE_GAP_PHY_2MBPS; // Set TX to 2M PHY

                    ////err_code = sd_ble_gap_phy_update(handle, &phys);
                    ////send_usb_data(false, "Requested 2MBPS PHY, err:%d", err_code);
                }
                //else if(devices[handle].cent.start_send_after_conn_update && !sending_periodic_tlv && !sending_relay_tlv){
                    //uint8_t tlv_phoneReadyToStart[2] = {0,0};
                    //if(IS_RELAY){
                    //    devices[handle].cent.start_send_after_conn_update = false;
                    //    send_tlv_from_p(handle, tlv_phoneReadyToStart, 1);
                    //}else{
                    //    devices[handle].cent.start_send_after_conn_update = false;
                    //    send_tlv_from_p(handle, tlv_phoneReadyToStart, 2);
                    //}
                //}
                
            }

            //if(send_test_rssi){
            //    if(new_params.min_conn_interval * UNIT_1_25_MS / 1000 <= conn_params_test_rssi.max_conn_interval * UNIT_1_25_MS / 1000
            //        //|| new_params.min_conn_interval * UNIT_1_25_MS / 1000 <= 220
            //        ){
            //        //sending_rssi = true;
            //        //send_test_rssi_tlv();
            //        send_usb_data(false, "Correct params, if1=%d, if2=%d", 
            //            new_params.min_conn_interval * UNIT_1_25_MS / 1000 <= conn_params_test_rssi.max_conn_interval * UNIT_1_25_MS / 1000,
            //            new_params.min_conn_interval * UNIT_1_25_MS / 1000 <= 220);
            //        if(package_storage[0].data[10] == 'A' && waiting_conn_update_next_test && send_test_rssi){
            //            waiting_conn_update_next_test = false;
            //            rssi_timer_start();
            //        }
            //    }else{
            //        send_usb_data(true, "Request new params for %s:%d: MIN %d units, %d in ms; MAX %d units, %d in ms; latency=%d, timeout=%d", 
            //            devices[p_gap_evt->conn_handle].name, p_gap_evt->conn_handle,
            //            conn_params_test_rssi.min_conn_interval * UNIT_1_25_MS / 1000, 
            //            conn_params_test_rssi.min_conn_interval + conn_params_test_rssi.min_conn_interval/4, 
            //            conn_params_test_rssi.max_conn_interval * UNIT_1_25_MS / 1000, 
            //            conn_params_test_rssi.max_conn_interval + conn_params_test_rssi.max_conn_interval/4,
            //            conn_params_test_rssi.slave_latency,
            //            conn_params_test_rssi.conn_sup_timeout * UNIT_10_MS / 1000);
            //        update_conn_params(handle, conn_params_test_rssi);
            //    }
            //    return;
            //}
            //if(devices[p_gap_evt->conn_handle].id == 'P' && updating_ongather_conn){
            //    updating_ongather_conn = update_all_conn_for_gather();
            //}
            
            //else if(is_sending_to_phone){
            //    update_conn_params(PHONE_HANDLE, peri_request_to_phone_conn_params);
            //}
            //}else{
            //    send_usb_data(false, "Failed to get connection parameters for %s:%d, error code: %d", devices[p_gap_evt->conn_handle].name,p_gap_evt->conn_handle, err_code);
            //}
        }  
        break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {//Peripheral
            //send_usb_data(false, "PHY update request.");
            ble_gap_phys_t const phys =
            {
                //.rx_phys = BLE_GAP_PHY_AUTO,
                //.tx_phys = BLE_GAP_PHY_AUTO,
                .rx_phys = BLE_GAP_PHY_2MBPS,
                .tx_phys = BLE_GAP_PHY_2MBPS,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            //APP_ERROR_CHECK(err_code);
            check_error("BLE_GAP_EVT_PHY_UPDATE_REQUEST PER",err_code);
            
        } 
        break;
        
        case BLE_GAP_EVT_PHY_UPDATE:
            // PHY update complete
            if (p_ble_evt->evt.gap_evt.params.phy_update.status == BLE_HCI_STATUS_CODE_SUCCESS)
            {
                //send_usb_data(false, "PHY updated to TX: %d, RX: %d; for handle:%d, name:%s.",
                //             p_ble_evt->evt.gap_evt.params.phy_update.tx_phy,
                //             p_ble_evt->evt.gap_evt.params.phy_update.rx_phy,
                //             handle, devices[handle].name);
            }
            else
            {
                send_usb_data(false, "PHY update failed with handle:%d, name:%s.", handle, devices[handle].name);
            }
            break;


        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            // Pairing not supported
            //Proposed
            err_code = sd_ble_gap_sec_params_reply(p_ble_evt->evt.gap_evt.conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
            //APP_ERROR_CHECK(err_code);
            check_error("BLE_GAP_EVT_SEC_PARAMS_REQUEST PER",err_code);
            break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            // No system attributes have been stored.
            // Proposed
            err_code = sd_ble_gatts_sys_attr_set(p_ble_evt->evt.gap_evt.conn_handle, NULL, 0, 0);
            //APP_ERROR_CHECK(err_code);
            check_error("BLE_GATTS_EVT_SYS_ATTR_MISSING PER",err_code);
            break;

        case BLE_GATTC_EVT_TIMEOUT://Peripheral
            // Disconnect on GATT Client timeout event.
            //err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
            //                                 BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            ////APP_ERROR_CHECK(err_code);
            check_error("BLE_GATTC_EVT_TIMEOUT PER",err_code);
            //break;

            //if(devices[handle].tp.tx_idx == devices[handle].tp.max_tx_idx){// raise peripheral tx power
            //    err_code = change_conn_tx(devices[handle].tp.tx_range[devices[handle].tp.max_tx_idx - 1], handle);
            //    if(err_code == NRF_SUCCESS){
            //        tlv_rssiChange[2] = (uint8_t) devices[handle].tp.tx_range[devices[handle].tp.max_tx_idx - 1];
            //        send_tlv_from_p(handle, tlv_rssiChange, 3);
            //        send_usb_data(false, "TIMEOUT Increased TX power to %s:%d to %d dBm", devices[handle].name, handle, devices[handle].tp.tx_range[devices[handle].tp.max_tx_idx - 1]);
            //        devices[handle].tp.incr_counter = 0;
            //        devices[handle].tp.tx_idx = devices[handle].tp.max_tx_idx - 1;
            //        package_count++;
            //        package_tail = (package_tail+255)%package_storage_size;
            //        break;
            //    }      
            //}
            // Disconnect on GATT Client timeout event.
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
           devices[connecting_handle].cent.connecting = false;
            //APP_ERROR_CHECK(err_code);
            //reset_device_full(p_ble_evt->evt.gatts_evt.conn_handle);
            //send_usb_data(true, "BLE_GATTC_EVT_TIMEOUT PER",err_code);
            break;


        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            //err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
            //                                 BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            ////APP_ERROR_CHECK(err_code);
            //check_error("BLE_GATTS_EVT_TIMEOUT PER",err_code);
            //break;

            //if(devices[handle].tp.tx_idx == devices[handle].tp.max_tx_idx){// raise peripheral tx power
            //    err_code = change_conn_tx(devices[handle].tp.tx_range[devices[handle].tp.max_tx_idx - 1], handle);
            //    if(err_code == NRF_SUCCESS){
            //        tlv_rssiChange[2] = (uint8_t) devices[handle].tp.tx_range[devices[handle].tp.max_tx_idx - 1];
            //        send_tlv_from_p(handle, tlv_rssiChange, 3);
            //        send_usb_data(false, "TIMEOUT2: Increased TX power to Cent:%d to %d dBm", handle, devices[handle].tp.tx_range[devices[handle].tp.max_tx_idx - 1]);
            //        devices[handle].tp.incr_counter = 0;
            //        devices[handle].tp.tx_idx = devices[handle].tp.max_tx_idx  - 1;
            //        package_count++;
            //        package_tail = (package_tail+255)%package_storage_size;
            //        break;
            //    }      
            //}
            // Disconnect on GATT Server timeout event.
            devices[connecting_handle].cent.connecting = false;
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            //reset_device_full(p_ble_evt->evt.gatts_evt.conn_handle);
            //APP_ERROR_CHECK(err_code);
            //send_usb_data(true, "BLE_GATTS_EVT_TIMEOUT PER",err_code);
            break;

        case BLE_GAP_EVT_AUTH_KEY_REQUEST:
            //send_usb_data(false, "BLE_GAP_EVT_AUTH_KEY_REQUEST");
            break;

        case BLE_GAP_EVT_LESC_DHKEY_REQUEST:
            //send_usb_data(false, "BLE_GAP_EVT_LESC_DHKEY_REQUEST");
            break;

         case BLE_GAP_EVT_AUTH_STATUS:
             //send_usb_data(false, "BLE_GAP_EVT_AUTH_STATUS: status=0x%x bond=0x%x lv4: %d kdist_own:0x%x kdist_peer:0x%x",
             //             p_ble_evt->evt.gap_evt.params.auth_status.auth_status,
             //             p_ble_evt->evt.gap_evt.params.auth_status.bonded,
             //             p_ble_evt->evt.gap_evt.params.auth_status.sm1_levels.lv4,
             //             *((uint8_t *)&p_ble_evt->evt.gap_evt.params.auth_status.kdist_own),
             //             *((uint8_t *)&p_ble_evt->evt.gap_evt.params.auth_status.kdist_peer));
            break;
        default:
            // No implementation needed.
            break;
    }
}



/**@brief Function for checking whether a bluetooth stack event is an advertising timeout.
 *
 * @param[in] p_ble_evt Bluetooth stack event.
 */
static bool ble_evt_is_advertising_timeout(ble_evt_t const * p_ble_evt)
{
    //send_usb_data(false, "Advertising STOPPED timeout");
    return (p_ble_evt->header.evt_id == BLE_GAP_EVT_ADV_SET_TERMINATED);
}

void ble_evt_handler2(ble_evt_t const * p_ble_evt, void * p_context)
{
    uint16_t conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
    uint16_t role        = ble_conn_state_role(conn_handle);
    // Based on the role this device plays in the connection, dispatch to the right handler.
    if (role == BLE_GAP_ROLE_PERIPH || ble_evt_is_advertising_timeout(p_ble_evt))
    { 
        ble_nus_on_ble_evt(p_ble_evt, &m_nus);
        on_ble_peripheral_evt(p_ble_evt);
    }
    else if ((role == BLE_GAP_ROLE_CENTRAL) || (p_ble_evt->header.evt_id == BLE_GAP_EVT_ADV_REPORT))
    {
        ble_nus_c_on_ble_evt(p_ble_evt, &m_ble_nus_c);
        on_ble_central_evt(p_ble_evt);
    }
}

static bool ble_advdata_names_parse(uint8_t * adv_data, uint8_t adv_data_len, uint8_t ad_type, uint8_t * buffer, uint8_t buffer_len)
{
    uint8_t index = 0;

    while (index < adv_data_len)
    {
        uint8_t field_len = adv_data[index];
        uint8_t field_type = adv_data[index + 1];

        if (field_type == ad_type)
        {
            memcpy(buffer, &adv_data[index + 2], field_len - 1);
            buffer[field_len - 1] = '\0';
            return true;
        }

        index += field_len + 1;
    }

    return false;
}

/**@brief Function for handling Scanning Module events.
 */
static void scan_evt_handler(scan_evt_t const * p_scan_evt)
{
    ret_code_t err_code;
    ble_gap_evt_adv_report_t const * p_adv = 
                   p_scan_evt->params.filter_match.p_adv_report;

    
    ble_gap_scan_params_t    const * p_scan_param = 
                   p_scan_evt->p_scan_params;
    switch(p_scan_evt->scan_evt_id)
    {
        case NRF_BLE_SCAN_EVT_FILTER_MATCH:
        {
             int8_t rssi = p_adv->rssi;
             //ble_gap_evt_adv_report_t const * p_adv = p_scan_evt->params.filter_match.p_adv_report;
             char device_name[5];
             memset(device_name, 0, sizeof(device_name));
             
             // Extract the device name from the advertisement packet.
             bool name_found = ble_advdata_names_parse(p_adv->data.p_data, p_adv->data.len, BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, (uint8_t*)device_name, sizeof(device_name));

             //if (!name_found) {
             //    // Try extracting the short local name if the complete name is not available
             //    name_found = ble_advdata_names_parse(p_adv->data.p_data, p_adv->data.len, BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME, (uint8_t*)device_name, sizeof(device_name));
             //}
             //send_usb_data(false, "NRF_BLE_SCAN_EVT_FILTER_MATCH, %s - RSSi:%d, ID:%d, nameFnd:%d, isPeri:%d", device_name, rssi, device_name[4] - '0' -1,name_found, memcmp(device_name,"Peri",4) == 0);
             //OPTIONAL: OLD working, connect directly
             //if (name_found) {
             //    send_usb_data(false, "NRF_BLE_SCAN_EVT_FILTER_MATCH name found");
             //    // Compare the extracted name with our list of target names
             //    for (uint32_t i = 0; i < sizeof(m_target_periph_names) / sizeof(m_target_periph_names[0]); i++) {
             //        if (strcmp(device_name, m_target_periph_names[i]) == 0) {
             //            send_usb_data(false, "Matched Peri LIST device name: %s", device_name);
             //            // Initiate connection or process accordingly.
             //            err_code = sd_ble_gap_connect(&p_adv->peer_addr, &m_scan_param, &m_scan.conn_params, APP_BLE_CONN_CFG_TAG);
             //            //APP_ERROR_CHECK(err_code);
             //            check_error("SCAN sd_ble_gap_connect",err_code);
             //            send_usb_data(false, "Match %s err:%d", device_name, err_code);
             //            return;  // Stop checking further since we've matched a name.
             //        }
             //    }
             //}
            
            //ALLCONN: scan one by one
            name_found = false;
            //if(scanning_for_on_req && strncmp(device_name, m_target_periph_name, 5) == 0){
            //    send_usb_data(false, "found disconnected ON_REQ name %s", device_name);
            //    name_found = true;
            //}else{
                for (uint32_t i = 0; i < 7; i++) {
                    if (strncmp(device_name, m_target_periph_names[i],5) == 0) {
                        //send_usb_data(false, "found name %s", device_name);
                        name_found = true;
                        break;
                    }
                }
            //}
            if(name_found){ //&& (memcmp(device_name,m_target_periph_name, 5) == 0)){

                //if(spec_target_peri[0] == 0 || (spec_target_peri[0] != 0 && memcmp(device_name,spec_target_peri,5) == 0)){
                    uint8_t periId = device_name[4] - '0' - 1; //number - 1
                    //uint8_t periNum = device_name[4] - '0' + 1;
                    //bool connect = true;
                    //for(int i = 0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
                    //    if(devices[i].nameNum == periNum){
                    //        connect = false;
                    //    }
                    //}


                    //if(!connected_peris[periId]){
                        //ble_gap_conn_params_t conn_params;
                        //memset(&conn_params, 0, sizeof(conn_params));
                        //send_usb_data(false, "NEW periID=%d, connections:%d%d%d%d%d%d", periId, connected_peris[0], connected_peris[1], connected_peris[2], connected_peris[3], connected_peris[4], connected_peris[5]);

                        //conn_params.min_conn_interval = initial_conn_params.min_interval;
                        //conn_params.max_conn_interval = initial_conn_params.max_interval;
                        //conn_params.slave_latency = initial_conn_params.slave_latency;
                        //conn_params.conn_sup_timeout = initial_conn_params.timeout;

                        ////send_usb_data(false, "Scan conn params: MIN %d units, %d in ms; MAX %d units, %d in ms; latency=%d, timeout=%d", 
                        ////                m_scan.conn_params.min_conn_interval * UNIT_1_25_MS / 1000, 
                        ////                m_scan.conn_params.min_conn_interval + m_scan.conn_params.min_conn_interval /4, 
                        ////                m_scan.conn_params.max_conn_interval * UNIT_1_25_MS / 1000, 
                        ////                m_scan.conn_params.max_conn_interval + m_scan.conn_params.min_conn_interval /4,
                        ////                m_scan.conn_params.slave_latency,
                        ////                m_scan.conn_params.conn_sup_timeout * UNIT_10_MS / 1000);

                        //send_usb_data(true, "Scan conn params: MIN %d units, %d in ms; MAX %d units, %d in ms; latency=%d, timeout=%d", 
                        //                initial_conn_params.min_conn_interval * UNIT_1_25_MS / 1000, 
                        //                initial_conn_params.min_conn_interval + initial_conn_params.min_conn_interval /4, 
                        //                initial_conn_params.max_conn_interval * UNIT_1_25_MS / 1000, 
                        //                initial_conn_params.max_conn_interval + initial_conn_params.max_conn_interval /4,
                        //                initial_conn_params.slave_latency,
                        //                initial_conn_params.conn_sup_timeout * UNIT_10_MS / 1000);
                        ////err_code = sd_ble_gap_connect(&p_adv->peer_addr, &m_scan_param, &m_scan.conn_params, APP_BLE_CONN_CFG_TAG);

                        err_code = sd_ble_gap_connect(&p_adv->peer_addr, &m_scan_param, &initial_conn_params, APP_BLE_CONN_CFG_TAG);

                        check_error("SCAN sd_ble_gap_connect",err_code);
                        send_usb_data(false, "RELAY start to connect to %s, err:%d", device_name, err_code);
                    //}
                //}
            }

            //ALLCONN: all scanned at once
            //if(name_found && (memcmp(device_name,"Peri",4) == 0)){
            //    if(spec_target_peri[0] == 0 || (spec_target_peri[0] != 0 && memcmp(device_name,spec_target_peri,5) == 0)){
            //        uint8_t periId = device_name[4] - '0' - 1;
            //        //uint8_t periNum = device_name[4] - '0' + 1;
            //        //bool connect = true;
            //        //for(int i = 0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
            //        //    if(devices[i].nameNum == periNum){
            //        //        connect = false;
            //        //    }
            //        //}
            //        if(!connected_peris[periId]){
            //            ble_gap_conn_params_t conn_params;
            //            memset(&conn_params, 0, sizeof(conn_params));
            //            send_usb_data(false, "NEW periID=%d, connections:%d%d%d%d%d%d", periId, connected_peris[0], connected_peris[1], connected_peris[2], connected_peris[3], connected_peris[4], connected_peris[5]);

            //            conn_params.min_conn_interval = initial_conn_params.min_interval;
            //            conn_params.max_conn_interval = initial_conn_params.max_interval;
            //            conn_params.slave_latency = initial_conn_params.slave_latency;
            //            conn_params.conn_sup_timeout = initial_conn_params.timeout;

            //            send_usb_data(false, "Scan conn params: MIN %d units, %d in ms; MAX %d units, %d in ms; latency=%d, timeout=%d", 
            //                            m_scan.conn_params.min_conn_interval * UNIT_1_25_MS / 1000, 
            //                            m_scan.conn_params.min_conn_interval + m_scan.conn_params.min_conn_interval /4, 
            //                            m_scan.conn_params.max_conn_interval * UNIT_1_25_MS / 1000, 
            //                            m_scan.conn_params.max_conn_interval + m_scan.conn_params.min_conn_interval /4,
            //                            m_scan.conn_params.slave_latency,
            //                            m_scan.conn_params.conn_sup_timeout);

            //            send_usb_data(false, "Scan NEW conn params: MIN %d units, %d in ms; MAX %d units, %d in ms; latency=%d, timeout=%d", 
            //                            initial_conn_params.min_interval * UNIT_1_25_MS / 1000, 
            //                            initial_conn_params.min_interval + initial_conn_params.min_interval /4, 
            //                            initial_conn_params.max_interval * UNIT_1_25_MS / 1000, 
            //                            initial_conn_params.max_interval + initial_conn_params.max_interval /4,
            //                            initial_conn_params.slave_latency,
            //                            initial_conn_params.timeout);
            //            //err_code = sd_ble_gap_connect(&p_adv->peer_addr, &m_scan_param, &m_scan.conn_params, APP_BLE_CONN_CFG_TAG);
            //            err_code = sd_ble_gap_connect(&p_adv->peer_addr, &m_scan_param, &conn_params, APP_BLE_CONN_CFG_TAG);
            //            check_error("SCAN sd_ble_gap_connect",err_code);
            //            send_usb_data(false, "RELAY connect to %s, err:%d, numID-1:%d", device_name, err_code, periId);
            //        }
            //    }
            //}

            //ADV: NEW get adv data
            //if(name_found){                
            //    //if(data_gather_requested && peri_setup[periId].chosen && peri_setup[periId].notConnected){
            //        //TODO: DO CONNECTION
            //    //    err_code = sd_ble_gap_connect(&p_adv->peer_addr, &m_scan_param, &m_scan.conn_params, APP_BLE_CONN_CFG_TAG);
            //    //    check_error("SCAN sd_ble_gap_connect",err_code);
            //    //    send_usb_data(false, "RELAY connect to %s, err:%d", device_name, err_code);
            //    //    if(err_code == NRF_SUCCESS){
            //    //        peri_setup[periId].notConnected = false;
            //    //    }
            //    //}else{
            //    //send_usb_data(false, "NAMEFOUND: %s, numID:%d, isP:%d, isC:%d", device_name,device_name[4] - '0' - 1,memcmp(device_name,"Peri",4) == 0,memcmp(device_name,"Cent",4) == 0);
            //    //OPTIONAL: SCAN and do distributed routing
            //    //if(memcmp(device_name,"Cent",4) == 0){
            //    //    // Search for the Manufacturer Specific Data (0xFF)
            //    //    uint8_t *msd_data = NULL;
            //    //    uint8_t msd_len = 0;
            //    //    //send_usb_data(false, "FOUND CENT %s", device_name);
            //    //    for (uint8_t index = 0; index < p_adv->data.len; ) {
            //    //        uint8_t field_length = p_adv->data.p_data[index];  // Length of the field
            //    //        uint8_t field_type = p_adv->data.p_data[index + 1]; // Field type

            //    //        if (field_type == BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA) {
            //    //            msd_data = &p_adv->data.p_data[index + 2];  // MSD starts after length and type
            //    //            msd_len = field_length - 1;  // Subtract the type byte
            //    //            break;  // MSD found, exit loop
            //    //        }

            //    //        // Move to the next field
            //    //        index += field_length + 1;  // field_length includes the length byte itself
            //    //    }

            //    //    // If MSD was found, extract the emergency flag
            //    //    if (msd_data != NULL && msd_len >= 1) {
            //    //        for(int i=0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
                            
            //    //            peri_setup[i].cent_rssi = (int8_t) (msd_data[i] -128);
            //    //            int8_t rs = msd_data[i];
            //    //            send_usb_data(false, "Cent has P%d, rssi=%d, here=%d, rs=%d", i+1,msd_data[i], peri_setup[i].cent_rssi, rs);
            //    //        }
            //    //    }
                    
            //    //}
            //    //else if(memcmp(device_name,"Peri",4) == 0){
            //    if(memcmp(device_name,"Peri",4) == 0){
            //        uint8_t periId = device_name[4] - '0' - 1;
            //        if(data_gather_requested && peri_setup[periId].chosen && peri_setup[periId].notConnected){
            //            //TODO: DO CONNECTION
            //            err_code = sd_ble_gap_connect(&p_adv->peer_addr, &m_scan_param, &m_scan.conn_params, APP_BLE_CONN_CFG_TAG);
            //            check_error("SCAN sd_ble_gap_connect",err_code);
            //            send_usb_data(false, "RELAY connect to %s, err:%d", device_name, err_code);
            //            if(err_code == NRF_SUCCESS){
            //                peri_setup[periId].notConnected = false;
            //            }
            //        }else{
            //            peri_setup[periId].rssi_window[peri_setup[periId].head] = rssi;
            //            peri_setup[periId].head = (peri_setup[periId].head + 1) % RSSI_WINDOW_SIZE;
            //            peri_setup[periId].sum_rssi = peri_setup[periId].sum_rssi + rssi - peri_setup[periId].rssi_window[peri_setup[periId].head % RSSI_WINDOW_SIZE];
            //            peri_setup[periId].avg_rssi = peri_setup[periId].sum_rssi / RSSI_WINDOW_SIZE;
            //            //int avg2 = (int8_t) peri_setup[periId].sum_rssi / RSSI_WINDOW_SIZE;
            //            //int8_t avg3 = peri_setup[periId].sum_rssi / RSSI_WINDOW_SIZE;
            //            //int avg4 = peri_setup[periId].sum_rssi / 20;
            //            rssi_scan_counter++;
                        
            //            if(peri_setup[periId].id == 'X'){
            //                send_usb_data(false, "%s had ID=%d", device_name, peri_setup[periId].id);
            //                peri_setup[periId].id = 'I' + periId;
            //                send_usb_data(false, "%s is now ID=%c,%d", device_name, peri_setup[periId].id, peri_setup[periId].id);
            //            }
            //            //send_usb_data(false, "%s, pID:%d, has avgRSSI %d, this=%d, sum=%d, a2=%d, a3=%d, a4=%d", device_name, periId, peri_setup[periId].avg_rssi, rssi, peri_setup[periId].sum_rssi, avg2, avg3, avg4);
            //            //send_usb_data(false, "%s, pID:%d, has avgRSSI %d, head=%d", device_name, periId, peri_setup[periId].avg_rssi, peri_setup[periId].head);

            //            //peri_setup[periId].id = 'I' + periId;

            //            //OPTIONAL: Emergency behaviour
            //            uint8_t emergency_flag = 0;  // Default: no emergency
            //            // Search for the Manufacturer Specific Data (0xFF)
            //            uint8_t *msd_data = NULL;
            //            uint8_t msd_len = 0;
            //            for (uint8_t index = 0; index < p_adv->data.len; ) {
            //                uint8_t field_length = p_adv->data.p_data[index];  // Length of the field
            //                uint8_t field_type = p_adv->data.p_data[index + 1]; // Field type

            //                if (field_type == BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA) {
            //                    msd_data = &p_adv->data.p_data[index + 2];  // MSD starts after length and type
            //                    msd_len = field_length - 1;  // Subtract the type byte
            //                    break;  // MSD found, exit loop
            //                }

            //                // Move to the next field
            //                index += field_length + 1;  // field_length includes the length byte itself
            //            }

            //            // If MSD was found, extract the emergency flag
            //            if (msd_data != NULL && msd_len >= 1) {
            //                emergency_flag = msd_data[0];  // Assume the first byte contains the emergency flag
            //            }

            //            // Output the results (log or print, depending on your setup)
                        
            //            // 4. If an emergency is detected, initiate a connection (if necessary)
            //            if (emergency_flag == 0x01) {
            //                send_usb_data(false, "Device Name: %s, chID:%c, RSSI: %d, Emergency Flag: %d\n", device_name, peri_setup[periId].id, rssi, emergency_flag);
            //                send_usb_data(false, "Emergency detected from device: %s\n", device_name);
            //                err_code = sd_ble_gap_connect(&p_adv->peer_addr, &m_scan_param, &m_scan.conn_params, APP_BLE_CONN_CFG_TAG);
            //                check_error("SCAN sd_ble_gap_connect",err_code);
            //                send_usb_data(false, "EMERGECY connect to %s, err:%d", device_name, err_code);
            //            }
            
            //            if(rssi_scan_counter == RSSI_SCAN_COUNTER_MAX){
            //                update_adv();
            //                rssi_scan_counter = 0;
            //            }
            //        }
            //    }
            //    //}
            //}
        }break;
        
        case NRF_BLE_SCAN_EVT_CONNECTING_ERROR:
        {
            err_code = p_scan_evt->params.connecting_err.err_code;
            //APP_ERROR_CHECK(err_code);
            check_error("SCAN NRF_BLE_SCAN_EVT_CONNECTING_ERROR",err_code);
        } break;

        case NRF_BLE_SCAN_EVT_CONNECTED:
        {
            ble_gap_evt_connected_t const * p_connected =
                             p_scan_evt->params.connected.p_connected;
           // Scan is automatically stopped by the connection.
           send_usb_data(false, "Connecting to target %02x%02x%02x%02x%02x%02x",
                    p_connected->peer_addr.addr[0],
                    p_connected->peer_addr.addr[1],
                    p_connected->peer_addr.addr[2],
                    p_connected->peer_addr.addr[3],
                    p_connected->peer_addr.addr[4],
                    p_connected->peer_addr.addr[5]
                    );
        } break;

        case NRF_BLE_SCAN_EVT_SCAN_TIMEOUT:
        {
           send_usb_data(false, "Scan timed out.");
           scan_start();
        } break;

        default:
           break;
    }
}

/**@brief Function for initialization the scanning and setting the filters.
 */
ret_code_t scan_init(void)
{
    ret_code_t          err_code;
    nrf_ble_scan_init_t init_scan;

    memset(&init_scan, 0, sizeof(init_scan));

    init_scan.p_scan_param = &m_scan_param;

    ///////
    ble_gap_conn_params_t conn_params;
    memset(&conn_params, 0, sizeof(conn_params));

    conn_params.min_conn_interval = initial_conn_params.min_conn_interval;
    conn_params.max_conn_interval = initial_conn_params.max_conn_interval;
    conn_params.slave_latency = initial_conn_params.slave_latency;
    conn_params.conn_sup_timeout = initial_conn_params.conn_sup_timeout;

    init_scan.p_conn_param = &conn_params;
    ///////

    err_code = nrf_ble_scan_init(&m_scan, &init_scan, scan_evt_handler);
    APP_ERROR_CHECK(err_code);

    //if (strlen(m_target_periph_name) != 0)
    //{
        //err_code = nrf_ble_scan_filter_set(&m_scan, 
        //                                   SCAN_NAME_FILTER, 
        //                                   m_target_periph_name);
    //    APP_ERROR_CHECK(err_code);
    //}
    uint16_t appearance = 2;//BLE_APPEARANCE_UNKNOWN;
    err_code = nrf_ble_scan_filter_set(&m_scan, SCAN_APPEARANCE_FILTER, &appearance);
    check_error("Scan init, appearance set",err_code);
    //err_code = nrf_ble_scan_filter_set(&m_scan, 
    //                                   SCAN_UUID_FILTER, 
    //                                   &m_adv_uuids[0]);
    //check_error("UUID filter set", err_code);

    err_code = nrf_ble_scan_filters_enable(&m_scan, 
                                           NRF_BLE_SCAN_ALL_FILTER, 
                                           false);
    //APP_ERROR_CHECK(err_code);
    return err_code;

}

/**@brief Function for handling events from the GATT library. */
void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
    //
    //uint16_t conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
    //uint16_t role        = ble_conn_state_role(conn_handle);
    //append_log_to_flash("ble_evt_handler handle:%d, role:%d",p_ble_evt->evt.gap_evt.conn_handle,role);
    //// Based on the role this device plays in the connection, dispatch to the right handler.
    //if (role == BLE_GAP_ROLE_PERIPH || ble_evt_is_advertising_timeout(p_ble_evt))
    //
    if (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED)
    {
        uint16_t m_ble_nus_max_data_len = p_evt->params.att_mtu_effective - OPCODE_LENGTH - HANDLE_LENGTH;
        send_usb_data(false, "Data len is set to 0x%X(%d)", m_ble_nus_max_data_len, m_ble_nus_max_data_len);
    }
    send_usb_data(false, "ATT MTU exchange completed. central 0x%x peripheral 0x%x,hndl:%d, if:%d",
                  p_gatt->att_mtu_desired_central,
                  p_gatt->att_mtu_desired_periph,
                  p_evt->conn_handle, (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED));
}


/**@brief Function for initializing the GATT library. */
ret_code_t gatt_init(void)
{
    ret_code_t err_code;

    //err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
    err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_ble_gatt_att_mtu_periph_set(&m_gatt, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_ble_gatt_att_mtu_central_set(&m_gatt, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
    APP_ERROR_CHECK(err_code);
    return err_code;
}

void pm_evt_handler(pm_evt_t const * p_evt)
{
    pm_handler_on_pm_evt(p_evt);
    pm_handler_disconnect_on_sec_failure(p_evt);
    pm_handler_flash_clean(p_evt);

    switch (p_evt->evt_id)
    {
        case PM_EVT_PEERS_DELETE_SUCCEEDED:
            adv_start(false);
            break;

        default:
            break;
    }
}

/**@brief Function for the Peer Manager initialization.
 */
ret_code_t peer_manager_init(void)
{
    ble_gap_sec_params_t sec_param;
    ret_code_t           err_code;

    err_code = pm_init();
    APP_ERROR_CHECK(err_code);
    
    //send_usb_data(false, "pm_init e:%d",err_code);
    //if(err_code != 0){return err_code;}

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    // Security parameters to be used for all security procedures.
    sec_param.bond           = SEC_PARAM_BOND;
    sec_param.mitm           = SEC_PARAM_MITM;
    sec_param.lesc           = SEC_PARAM_LESC;
    sec_param.keypress       = SEC_PARAM_KEYPRESS;
    sec_param.io_caps        = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob            = SEC_PARAM_OOB;
    sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_own.enc  = 1;
    sec_param.kdist_own.id   = 1;
    sec_param.kdist_peer.enc = 1;
    sec_param.kdist_peer.id  = 1;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);
    //send_usb_data(false, "pm_sec_params_set e:%d",err_code);
    //if(err_code != 0){return err_code;}
    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);
    //send_usb_data(false, "pm_register e:%d",err_code);
    //if(err_code != 0){return err_code;} 
    return err_code;
}

uint32_t disconnect_handle(uint16_t handle)
{
    uint32_t err_code;
    
    if(devices[handle].id != 255 && devices[handle].peri.connToPhone){
        send_usb_data(true, "Erased handle %d info, name:%s", handle, devices[handle].name);
        //reset_device_full(handle);
        devices[handle].connected = false;
        devices[handle].cent.connecting = false;
    }else{
        if(!isPhone){
            send_usb_data(true, "Erased handle %d info, not used previously", handle);
            reset_device_full(handle);
            devices[handle].connected = false;
        }
    }
    if (handle != BLE_CONN_HANDLE_INVALID)
    {
        err_code = sd_ble_gap_disconnect(handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        if(handle < NRF_SDH_BLE_CENTRAL_LINK_COUNT){
            m_ble_nus_c[handle].conn_handle = BLE_CONN_HANDLE_INVALID;
        }
        //else{
        //    m_conn_handles[handle] = BLE_CONN_HANDLE_INVALID;
        //}
        if (err_code != NRF_SUCCESS)
        {
            send_usb_data(false, "Failed to disconnect from handle: %d, Error: 0x%x", handle, err_code);
        }
        else
        {
            send_usb_data(false, "Successfully DISCONNECTED from handle: %d", handle);
            //if(devices[handle].nameNum!=255){
            //    connected_peris[devices[handle].nameNum-1] = false;
                
            //}
            
        }
        return err_code;
    }
    else
    {
        send_usb_data(false, "Invalid handle: %d, cannot disconnect.", handle);
    }
    return 100;
}

ret_code_t power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
    return  err_code;
}

void idle_state_handle(void)
{
    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
}
