#include "sdk_common.h"
#if NRF_MODULE_ENABLED(BLE_NUS)
#include "ble.h"
#include "ble_nus2.h"
#include "ble_srv_common.h"
#include "ble_conn_state.h"

//#include "usb_handler.h"
//#include "app_uart.h"
//#define NRF_LOG_MODULE_NAME ble_nus
//#if BLE_NUS_CONFIG_LOG_ENABLED
//#define NRF_LOG_LEVEL       BLE_NUS_CONFIG_LOG_LEVEL
//#define NRF_LOG_INFO_COLOR  BLE_NUS_CONFIG_INFO_COLOR
//#define NRF_LOG_DEBUG_COLOR BLE_NUS_CONFIG_DEBUG_COLOR
//#else // BLE_NUS_CONFIG_LOG_ENABLED
//#define NRF_LOG_LEVEL       0
//#endif // BLE_NUS_CONFIG_LOG_ENABLED
//#endif // BLE_NUS_CONFIG_LOG_ENABLED
#include "nrf_log.h"
#include "common.h"

//#include "nus_handler.h"
#include "device_conn.h"
#include "devices_conn_params.h"
#include "devices_tp.h"
//void (*tlv_periodic_sender)(void) = send_periodic_tlv;
//int (*tlv_from_p_sender)(int16_t h, bool rssi, uint8_t* tlv, uint16_t length) = send_tlv_from_p;
//NRF_LOG_MODULE_REGISTER();


// Sending Flags
bool sending_tlv_chunks = false;
bool periodic_timer_started = false;
bool sending_stored_tlv = false;
bool sending_missed_tlv = false;
bool waiting_to_send_phone = false;
bool updated_phone_conn = false;
bool sending_own_periodic = false;
bool sending_tlv_spec = false;
bool sending_relay_tlv = false;
bool sending_periodic_tlv = false;
bool pause_chunks = false;


// helper variables
bool last = false;
int last_prefix = 0;
bool start_send_data_not_received = true;
bool waitingRecFromUsb = false;
uint8_t last_received_by_cent = 0;
bool is_sending_on_gather = false;
bool data_gather_relay_on = false;
int8_t perisToRelayCnt = 0;
bool data_relay_in_process = false; //dont know if needed

// reset variables
void reset_data_gather_ble_nus(){
    last = false;
    last_prefix = 0;
    start_send_data_not_received = true;
    waitingRecFromUsb = false;
    last_received_by_cent = 0;
    data_gather_relay_on = false;
    is_sending_on_gather = false;
    perisToRelayCnt = 0;
    data_relay_in_process = false;

    sending_tlv_chunks = false;
    sending_stored_tlv = false;
    sending_tlv_spec = false;
    sending_relay_tlv = false;
    sending_periodic_tlv = false;
    pause_chunks = false;
}

// Queue tlv messages to centrals
int cent_tlv_head=0;
int cent_tlv_tail=0;
int cent_tlv_count=0;
int cent_tlv_max_size = MAX_PACKAGE_SIZE; // 1 less than max 244
int cent_tlv_storage_size = 30;
cent_tlv_storage_t cent_tlv_storage[30];


/**@brief Function for handling the @ref BLE_GAP_EVT_CONNECTED event from the SoftDevice.
 *
 * @param[in] p_nus     Nordic UART Service structure.
 * @param[in] p_ble_evt Pointer to the event received from BLE stack.
 */
static void on_connect(ble_nus_t * p_nus, ble_evt_t const * p_ble_evt)
{
    ret_code_t                 err_code;
    ble_nus_evt_t              evt;
    ble_gatts_value_t          gatts_val;
    uint8_t                    cccd_value[2];
    ble_nus_client_context_t * p_client = NULL;

    err_code = blcm_link_ctx_get(p_nus->p_link_ctx_storage,
                                 p_ble_evt->evt.gap_evt.conn_handle,
                                 (void *) &p_client);
    if (err_code != NRF_SUCCESS)
    {
        send_usb_data(false, "Link context for 0x%02X connection handle could not be fetched, ERR:%d.",
                      p_ble_evt->evt.gap_evt.conn_handle, err_code);
    }

    /* Check the hosts CCCD value to inform of readiness to send data using the RX characteristic */
    memset(&gatts_val, 0, sizeof(ble_gatts_value_t));
    gatts_val.p_value = cccd_value;
    gatts_val.len     = sizeof(cccd_value);
    gatts_val.offset  = 0;

    err_code = sd_ble_gatts_value_get(p_ble_evt->evt.gap_evt.conn_handle,
                                      p_nus->tx_handles.cccd_handle,
                                      &gatts_val);

    if ((err_code == NRF_SUCCESS)     &&
        (p_nus->data_handler != NULL) &&
        ble_srv_is_notification_enabled(gatts_val.p_value))
    {
        if (p_client != NULL)
        {
            p_client->is_notification_enabled = true;
        }

        memset(&evt, 0, sizeof(ble_nus_evt_t));
        evt.type        = BLE_NUS_EVT_COMM_STARTED;
        evt.p_nus       = p_nus;
        evt.conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
        evt.p_link_ctx  = p_client;

        p_nus->data_handler(&evt);
    }
}


/**@brief Function for handling the @ref BLE_GATTS_EVT_WRITE event from the SoftDevice.
 *
 * @param[in] p_nus     Nordic UART Service structure.
 * @param[in] p_ble_evt Pointer to the event received from BLE stack.
 */
static void on_write(ble_nus_t * p_nus, ble_evt_t const * p_ble_evt)
{
    ret_code_t                    err_code;
    ble_nus_evt_t                 evt;
    ble_nus_client_context_t    * p_client;
    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    err_code = blcm_link_ctx_get(p_nus->p_link_ctx_storage,
                                 p_ble_evt->evt.gatts_evt.conn_handle,
                                 (void *) &p_client);
    if (err_code != NRF_SUCCESS)
    {
        send_usb_data(false, "Link context on_write for 0x%02X connection handle could not be fetched.",
                      p_ble_evt->evt.gatts_evt.conn_handle);
    }

    memset(&evt, 0, sizeof(ble_nus_evt_t));
    evt.p_nus       = p_nus;
    evt.conn_handle = p_ble_evt->evt.gatts_evt.conn_handle;
    evt.p_link_ctx  = p_client;

    if ((p_evt_write->handle == p_nus->tx_handles.cccd_handle) &&
        (p_evt_write->len == 2))
    {
        if (p_client != NULL)
        {
            if (ble_srv_is_notification_enabled(p_evt_write->data))
            {
                p_client->is_notification_enabled = true;
                evt.type                          = BLE_NUS_EVT_COMM_STARTED;
            }
            else
            {
                p_client->is_notification_enabled = false;
                evt.type                          = BLE_NUS_EVT_COMM_STOPPED;
            }

            if (p_nus->data_handler != NULL)
            {
                p_nus->data_handler(&evt);
            }

        }
    }
    else if ((p_evt_write->handle == p_nus->rx_handles.value_handle) &&
             (p_nus->data_handler != NULL))
    {
        evt.type                  = BLE_NUS_EVT_RX_DATA;
        evt.params.rx_data.p_data = p_evt_write->data;
        evt.params.rx_data.length = p_evt_write->len;

        p_nus->data_handler(&evt);
    }
    else
    {
        // Do Nothing. This event is not relevant for this service.
    }
}


/**@brief Function for handling the @ref BLE_GATTS_EVT_HVN_TX_COMPLETE event from the SoftDevice.
 *
 * @param[in] p_nus     Nordic UART Service structure.
 * @param[in] p_ble_evt Pointer to the event received from BLE stack.
 */
static void on_hvx_tx_complete(ble_nus_t * p_nus, ble_evt_t const * p_ble_evt)
{
    ret_code_t                 err_code;
    ble_nus_evt_t              evt;
    ble_nus_client_context_t * p_client;

    err_code = blcm_link_ctx_get(p_nus->p_link_ctx_storage,
                                 p_ble_evt->evt.gatts_evt.conn_handle,
                                 (void *) &p_client);
    if (err_code != NRF_SUCCESS)
    {
        send_usb_data(false, "Link context on_hvx_tx_complete for 0x%02X connection handle could not be fetched.",
                      p_ble_evt->evt.gatts_evt.conn_handle);
        return;
    }

    if ((p_client->is_notification_enabled) && (p_nus->data_handler != NULL))
    {
        
        memset(&evt, 0, sizeof(ble_nus_evt_t));
        evt.type        = BLE_NUS_EVT_TX_RDY;
        evt.p_nus       = p_nus;
        evt.conn_handle = p_ble_evt->evt.gatts_evt.conn_handle;
        evt.p_link_ctx  = p_client;
        //if(devices[evt.conn_handle].count == 0){
        //    tp_timer_stop(&devices[evt.conn_handle]);
        //}
        p_nus->data_handler(&evt);
    }
}


void ble_nus_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    if ((p_context == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    ble_nus_t * p_nus = (ble_nus_t *)p_context;
    
    switch (p_ble_evt->header.evt_id)
    {
        
        case BLE_GAP_EVT_CONNECTED:
            send_usb_data(false, "bleNusEvt Conn handle:%d",p_ble_evt->evt.gap_evt.conn_handle);
            on_connect(p_nus, p_ble_evt);
            break;

        case BLE_GATTS_EVT_WRITE:
            on_write(p_nus, p_ble_evt);
            break;

        case BLE_GATTS_EVT_HVN_TX_COMPLETE:
            on_hvx_tx_complete(p_nus, p_ble_evt);
            //if (sending_nus_chunks && !pause_chunks) {
                //chunk_completed += p_ble_evt->evt.gatts_evt.params.hvn_tx_complete.count;  // Increment the number of completed packets
                //if(chunk_completed%20==0){
                //    send_usb_data(false, "HVNTX CH completed %d", chunk_completed);
                //}
                
            //    send_next_chunk_nus(p_nus, p_ble_evt->evt.gap_evt.conn_handle);
            //    break;
            //}
            //if (sending_relay_chunks) {
            //    relay_completed += p_ble_evt->evt.gatts_evt.params.hvn_tx_complete.count;  // Increment the number of completed packets
            //    if(relay_completed%20==0){
            //        send_usb_data(false, "HVNTX REL completed %d", relay_completed);
            //    }
                
            //    if(0<strlen(file_data)){
            //        relay_chunks(p_nus, p_ble_evt->evt.gap_evt.conn_handle);
            //    }else if(relay_sent <= relay_completed){
            //        sending_relay_chunks = 0;
            //    }
            //}
            break;

        default:
            // No implementation needed.
            break;
    }
}


uint32_t ble_nus_init(ble_nus_t * p_nus, ble_nus_init_t const * p_nus_init)
{
    ret_code_t            err_code;
    ble_uuid_t            ble_uuid;
    ble_uuid128_t         nus_base_uuid = NUS_BASE_UUID;
    ble_add_char_params_t add_char_params;

    VERIFY_PARAM_NOT_NULL(p_nus);
    VERIFY_PARAM_NOT_NULL(p_nus_init);

    // Initialize the service structure.
    p_nus->data_handler = p_nus_init->data_handler;

    /**@snippet [Adding proprietary Service to the SoftDevice] */
    // Add a custom base UUID.
    err_code = sd_ble_uuid_vs_add(&nus_base_uuid, &p_nus->uuid_type);
    VERIFY_SUCCESS(err_code);

    ble_uuid.type = p_nus->uuid_type;
    ble_uuid.uuid = BLE_UUID_NUS_SERVICE;

    // Add the service.
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &ble_uuid,
                                        &p_nus->service_handle);
    /**@snippet [Adding proprietary Service to the SoftDevice] */
    VERIFY_SUCCESS(err_code);

    // Add the RX Characteristic.
    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid                     = BLE_UUID_NUS_RX_CHARACTERISTIC;
    add_char_params.uuid_type                = p_nus->uuid_type;
    add_char_params.max_len                  = BLE_NUS_MAX_RX_CHAR_LEN;
    add_char_params.init_len                 = sizeof(uint8_t);
    add_char_params.is_var_len               = true;
    add_char_params.char_props.write         = 1;
    add_char_params.char_props.write_wo_resp = 1;

    add_char_params.read_access  = SEC_OPEN;
    add_char_params.write_access = SEC_OPEN;

    err_code = characteristic_add(p_nus->service_handle, &add_char_params, &p_nus->rx_handles);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Add the TX Characteristic.
    /**@snippet [Adding proprietary characteristic to the SoftDevice] */
    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid              = BLE_UUID_NUS_TX_CHARACTERISTIC;
    add_char_params.uuid_type         = p_nus->uuid_type;
    add_char_params.max_len           = BLE_NUS_MAX_TX_CHAR_LEN;
    add_char_params.init_len          = sizeof(uint8_t);
    add_char_params.is_var_len        = true;
    add_char_params.char_props.notify = 1;

    add_char_params.read_access       = SEC_OPEN;
    add_char_params.write_access      = SEC_OPEN;
    add_char_params.cccd_write_access = SEC_OPEN;

    return characteristic_add(p_nus->service_handle, &add_char_params, &p_nus->tx_handles);
    /**@snippet [Adding proprietary characteristic to the SoftDevice] */
}


uint32_t ble_nus_data_send(ble_nus_t * p_nus,
                           uint8_t   * p_data,
                           uint16_t  * p_length,
                           uint16_t    conn_handle)
{
    ret_code_t                 err_code;
    ble_gatts_hvx_params_t     hvx_params;
    ble_nus_client_context_t * p_client;

    VERIFY_PARAM_NOT_NULL(p_nus);

    err_code = blcm_link_ctx_get(p_nus->p_link_ctx_storage, conn_handle, (void *) &p_client);
    VERIFY_SUCCESS(err_code);

    if ((conn_handle == BLE_CONN_HANDLE_INVALID) || (p_client == NULL))
    {
        send_usb_data(false, "ERR nus snd, connhndl:%d, pclnt:%d", conn_handle, p_client == NULL);
        return NRF_ERROR_NOT_FOUND;}

    if (!p_client->is_notification_enabled)
    {return NRF_ERROR_INVALID_STATE;}

    if (*p_length > BLE_NUS_MAX_DATA_LEN)
    {return NRF_ERROR_INVALID_PARAM;}

    memset(&hvx_params, 0, sizeof(hvx_params));

    hvx_params.handle = p_nus->tx_handles.value_handle;
    hvx_params.p_data = p_data;
    hvx_params.p_len  = p_length;
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
    
    err_code = sd_ble_gatts_hvx(conn_handle, &hvx_params);
    //if(err_code == NRF_SUCCESS && !doStar ){
    //    tp_timer_start(&devices[conn_handle]);
    //}
    return err_code;
    //return 
}

uint8_t tlv_test[244] = {};
uint16_t test_len = 244;
void start_test_rssi(void){
    max_tx_idx_own = 6;
    min_tx_idx_own = 1;

    memset(tlv_test, 0, package_max_size);
    tlv_test[0] = 0;
    tlv_test[1] = 0;
    tlv_test[2] = cent_min_rssi;//devices[6].tp.min_rssi_limit;
    tlv_test[3] = cent_min_rssi + 7;//devices[6].tp.max_rssi_limit;
    tlv_test[4] = cent_min_rssi + 3;//devices[6].tp.ideal;
    tlv_test[5] = window_list[curr_window_index];
    RSSI_starting_limit = window_list[curr_window_index];
    test_len = 244;
    
    //package_storage[0].data[2] = DEVICE_ID;
    for (int i = 6; i < 244; i++) {
        tlv_test[i] = 'A';
    }
    if(!PERIS_DO_TP){
        change_rssi_margins_peri(cent_min_rssi, cent_min_rssi+7, cent_min_rssi+3);
        change_rssi_margins_cent(cent_min_rssi, cent_min_rssi+7, cent_min_rssi+3);
    }
    if(devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].connected){
        if(devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].cent.isPhone){
            update_conn_params(NRF_SDH_BLE_CENTRAL_LINK_COUNT, conn_params_test_rssi);
            devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].conn_params = conn_params_test_rssi;
            devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].periodic_params = conn_params_test_rssi;
        }
        //ret_code_t er = change_conn_tx(-20, NRF_SDH_BLE_CENTRAL_LINK_COUNT, (int8_t) devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.ema_rssi, (int8_t) devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.last_rssi);
        //if(er == NRF_SUCCESS){
        //    send_usb_data(true, "Set TX power to %s:%d to %d dBm", devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].name, NRF_SDH_BLE_CENTRAL_LINK_COUNT,20);
        //}else{
        //    send_usb_data(true, "NOT set TX power to %s:%d to %d dBm, err:%d", devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].name, NRF_SDH_BLE_CENTRAL_LINK_COUNT,20, er);
        //}
        if(!PERIS_DO_TP){
            change_rssi_margins_peri(-100, 8, -70);
            change_rssi_margins_cent(-100, 8, -71);
            devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.received_counter = -1;
        }
        //cent_min_rssi = -69;
        //waiting_conn_update_next_test = true;
        //update_conn_params(NRF_SDH_BLE_CENTRAL_LINK_COUNT, conn_params_test_rssi);
        rssi_timer_start();
        send_test_rssi = true;
        sending_rssi = false;
        //send_test_rssi_tlv();
        send_usb_data(true, "Started test rssi with %s", devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].name);
    }else{
        send_usb_data(true, "Could not start test rssi, no central connected");
    }
}

bool sending_rssi = false;
bool constant_test_done = false;
int max_trssi_packets = 256*5;//was 1600
int trssi_counter = 0;
bool waiting_conn_update_next_test = false;
bool last_test_rssiii = false;
int cent_min_rssi = 0;
int cent_min_rssi_og = 0;
uint8_t last_val_rssi = 0; 
int curr_tp_idx = 4;//6+1
//int max_tp_idx = 5-1; // desired idx - 1

bool doing_const_tp = false;
bool new_window = false;
bool do_first_window_change = false;


int min_cent_min_rssi = -77;

int curr_window_index = 2;
int max_window_idx = 5; // desired idx - 1
int window_list[8] = {1, 2, 5, 10, 20, 50, 100, 200};
void send_test_rssi_tlv(void){
    if(!PERIS_DO_TP){
        devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.received_counter = -1;
    }
    ret_code_t err_code;
    while(trssi_counter < max_trssi_packets){
        if(devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].connected){
            //while(true){
            //package_storage[0].data[1] = package_storage[0].data[1] + 1;
            if(last_test_rssiii){
                uint8_t reset_tlv[3] = {4, window_list[curr_window_index], (int8_t) cent_min_rssi};
                if(doing_const_tp){
                    reset_tlv[1] = 0;
                    
                    reset_tlv[2] = (uint8_t) curr_tp_idx - 1;
                }
                //uint8_t reset_tlv[3] = {4,4, (int8_t) (devices[6].tp.min_rssi_limit-2)};
                uint16_t l = 3;
                err_code = ble_nus_data_send(mm_nus, reset_tlv, &l, NRF_SDH_BLE_CENTRAL_LINK_COUNT);

                //err_code = NRF_SUCCESS;
                
            }else{
                tlv_test[6] = (int8_t) devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.last_rssi;
                tlv_test[7] = (int8_t) devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.ema_rssi;
                tlv_test[8] = (int8_t) tx_range[devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.own_idx];
                if(PERIS_DO_TP){
                    tlv_test[8] = (int8_t) tx_range[devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.tx_idx];
                }
                err_code = ble_nus_data_send(mm_nus, tlv_test, &test_len, NRF_SDH_BLE_CENTRAL_LINK_COUNT);
            }
            if (err_code == NRF_ERROR_RESOURCES) {
                // Wait for BLE_NUS_EVT_TX_RDY before sending more
                return;
                //send_test_rssi_tlv();
            }else if (err_code == NRF_ERROR_INVALID_STATE){
                send_usb_data(false, "Invalid state ERROR for sending chunks. Stop sending.");
                send_test_rssi = false;
                return;
            }else if (err_code != NRF_SUCCESS)
            {
                send_usb_data(false, "Failed to send NUS message. Error: 0x%x", err_code);
                send_test_rssi = false;
                //send_test_rssi_tlv();
                return;
            }else if(TIMEOUT_TPC){
                tp_timer_start(&devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT]);
            }
            if(last_test_rssiii){
                break;
            }else{
                tlv_test[1] = tlv_test[1] + 1;
                trssi_counter++;
                if(trssi_counter % 256 == 0){
                    tlv_test[0] = tlv_test[0]+1;
                    tlv_test[1] = 0;
                    send_usb_data(true, "NEW BATCH = %d (%c), packets left = %d", tlv_test[0], tlv_test[10], max_trssi_packets - trssi_counter);
                }
                //if(trssi_counter < max_trssi_packets){
                //    return;
                //}
            }
            
            //}
        }
    }
    if(!last_test_rssiii){
        last_test_rssiii = true;
        //package_storage[0].data[1] = 4;
        //package_storage[0].data[0] = 4;
        //package_storage[0].length = 2;
        trssi_counter = max_trssi_packets-1;
        if(cent_min_rssi == min_cent_min_rssi && curr_tp_idx > 2 && curr_window_index == max_window_idx){
            doing_const_tp = true;
            
        }else if(cent_min_rssi == min_cent_min_rssi && curr_window_index < max_window_idx){
            //if(doing_const_tp || constant_test_done){
                constant_test_done = true;
                cent_min_rssi = cent_min_rssi_og;//-67;
                RSSI_starting_limit;
                curr_window_index++;
                tlv_test[0] = 0;
                tlv_test[1] = 0;
                tlv_test[2] = cent_min_rssi;//devices[6].tp.min_rssi_limit;
                tlv_test[3] = cent_min_rssi + 7;//devices[6].tp.max_rssi_limit;
                tlv_test[4] = cent_min_rssi + 3;//devices[6].tp.ideal;
                tlv_test[5] = window_list[curr_window_index];//devices[6].tp.window;

                RSSI_starting_limit = window_list[curr_window_index];;
                change_rssi_margins_peri(cent_min_rssi, cent_min_rssi+7, cent_min_rssi+3);
                change_rssi_margins_cent(cent_min_rssi, cent_min_rssi+7, cent_min_rssi+3);
                for (int i = 6; i < 244; i++) {
                    tlv_test[i] = 'A';
                }
                //do_first_window_change = true;
            //}
            last_test_rssiii = false;
            sending_rssi = false;
            doing_const_tp = false;
            trssi_counter = 0;
            rssi_timer_start();
        }else{
            cent_min_rssi = cent_min_rssi - 2;
        }

    }else{
        //app_timer_stop(devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp_timer);
        //waiting_conn_update_next_test = true;
        //if(devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.min_rssi_limit-2 >= -85){
        if(!doing_const_tp){
            if(cent_min_rssi >= min_cent_min_rssi){//87){
                last_test_rssiii = false;
                sending_rssi = false;
                //update_conn_params(h, conn_params_test_rssi);
                //uint8_t reset_tlv[2] = {4,4};
                //send_tlv_from_p(NRF_SDH_BLE_CENTRAL_LINK_COUNT, reset_tlv, 2);
                
                //cent_min_rssi = cent_min_rssi - 2;
                change_rssi_margins_peri(cent_min_rssi, cent_min_rssi+7, cent_min_rssi+3);
                change_rssi_margins_cent(cent_min_rssi, cent_min_rssi+7, cent_min_rssi+3);
                
                tlv_test[1] = 0;
                tlv_test[0] = 0;
                tlv_test[2] = cent_min_rssi;//devices[6].tp.min_rssi_limit;
                tlv_test[3] = cent_min_rssi + 7;//devices[6].tp.max_rssi_limit;
                tlv_test[4] = cent_min_rssi + 3;//devices[6].tp.ideal;
                tlv_test[5] = window_list[curr_window_index];//devices[6].tp.window;

                for (int i = 6; i < 244; i++) {
                    tlv_test[i] = tlv_test[i]+1;
                }
                test_len = 244;
                trssi_counter = 0;

                //send_usb_data(true, "New margins for %c: min=%d, max=%d ideal=%d", package_storage[0].data[10], devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.min_rssi_limit, devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.max_rssi_limit, devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.ideal);
                 send_usb_data(true, "New margins for %c: min=%d, max=%d ideal=%d",tlv_test[10], cent_min_rssi, cent_min_rssi + 7, cent_min_rssi +3);
                //for (int i = 2; i < 244; i++) {
                //    package_storage[0].data[i] = package_storage[0].data[i]+1;
                //}
                devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.received_counter = -1;
                
                if(PERIS_DO_TP){
                    change_conn_tx(0, NRF_SDH_BLE_CENTRAL_LINK_COUNT, devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.ema_rssi, devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.last_rssi);
                    update_conn_params(NRF_SDH_BLE_CENTRAL_LINK_COUNT, conn_params_test_rssi);
                }
                rssi_timer_start();
            }else{
                send_usb_data(true, "FINISHEDDDDDDDDDDDDDDDDDDDDDDDDDDDD", devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.min_rssi_limit, devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.max_rssi_limit, devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.ideal);
                sending_rssi = true;
                last_test_rssiii = false;
                max_trssi_packets = 1000000;
                send_usb_data(true, "New FINISHED margins for %c: min=%d, max=%d ideal=%d", tlv_test[10], cent_min_rssi, cent_min_rssi + 7, cent_min_rssi +3);
                tlv_test[1] = 0;
                tlv_test[0] = 0;
                tlv_test[2] = cent_min_rssi;//devices[6].tp.min_rssi_limit;
                tlv_test[3] = cent_min_rssi + 7;//devices[6].tp.max_rssi_limit;
                tlv_test[4] = cent_min_rssi + 3;//devices[6].tp.ideal;
                tlv_test[5] = window_list[curr_window_index];//devices[6].tp.window;
                devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.min_rssi_limit = cent_min_rssi;
                devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.max_rssi_limit = cent_min_rssi+7;
                devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.ideal = cent_min_rssi+3;
                for (int i = 6; i < 244; i++) {
                    tlv_test[i] = 'X';
                }
                test_len = 244;
                trssi_counter = 0;
                bsp_board_leds_off();
                bsp_board_led_on(led_green);
                bsp_board_led_on(led_RED);
            }
        }else{
            last_test_rssiii = false;
            sending_rssi = false;
            //update_conn_params(h, conn_params_test_rssi);
            //uint8_t reset_tlv[2] = {4,4};
            //send_tlv_from_p(NRF_SDH_BLE_CENTRAL_LINK_COUNT, reset_tlv, 2);

            //change_rssi_margins_peri(devices[0].tp.min_rssi_limit-2, devices[0].tp.max_rssi_limit-2, devices[0].tp.ideal-2);
            //change_rssi_margins_cent(devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.min_rssi_limit-2, devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.max_rssi_limit-2, devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.ideal-2);
            curr_tp_idx--;
            int8_t txP = tx_range[curr_tp_idx];
            //cent_min_rssi = cent_min_rssi - 2;
            tlv_test[1] = 0;
            tlv_test[0] = 0;
            tlv_test[2] = txP;//devices[6].tp.min_rssi_limit;
            tlv_test[3] = txP;//devices[6].tp.max_rssi_limit;
            tlv_test[4] = txP;//devices[6].tp.ideal;
            tlv_test[5] = 0;//devices[6].tp.window;

            for (int i = 6; i < 244; i++) {
                tlv_test[i] = tlv_test[i]+1;
            }
            test_len = 244;
            trssi_counter = 0;

            //send_usb_data(true, "New margins for %c: min=%d, max=%d ideal=%d", package_storage[0].data[10], devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.min_rssi_limit, devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.max_rssi_limit, devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.ideal);
             send_usb_data(true, "New margins for %c: CONSTANT=%d", tlv_test[10], txP);
            //for (int i = 2; i < 244; i++) {
            //    package_storage[0].data[i] = package_storage[0].data[i]+1;
            //}
            devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.received_counter = -1;
            if(PERIS_DO_TP){
                change_conn_tx(0, NRF_SDH_BLE_CENTRAL_LINK_COUNT, devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.ema_rssi, devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.last_rssi);
                //devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.tx_idx = curr_tp_idx;
                update_conn_params(NRF_SDH_BLE_CENTRAL_LINK_COUNT, conn_params_test_rssi);
            }
            rssi_timer_start();
        }
    }
}

//int periodic_bytes_done=0;
void add_periodic_bytes(int new_bytes){
    if(stored_bytes > stored_bytes_sent){
        periodic_waiting_bytes += (stored_bytes- stored_bytes_sent);
        stored_bytes = 0;
    }
    periodic_waiting_bytes += new_bytes;//0;
    //int bytes_done = 0;
    uint16_t len = 244;
    int b = 0;
    int temp_waiting = periodic_waiting_bytes;
    //while(periodic_bytes_done < periodic_waiting_bytes){
    while(0 < periodic_waiting_bytes){
        if(package_count >= package_storage_size - 125){// No memory

            //periodic_waiting_bytes = periodic_waiting_bytes - periodic_bytes_done;
            return;
        }else{

            //if(periodic_waiting_bytes - periodic_bytes_done < 241){
            //    len = periodic_waiting_bytes - periodic_bytes_done + 3;
            //}
            if(periodic_waiting_bytes < 241){
                len = periodic_waiting_bytes + 3;
            }
            temp_waiting = periodic_waiting_bytes;
            memset(package_storage[package_head].data, 0, MAX_PACKAGE_SIZE);
            package_storage[package_head].data[0] = batch_counter;
            package_storage[package_head].data[1] = pref_counter;
            package_storage[package_head].data[2] = DEVICE_ID;
            package_storage[package_head].data[3] = devices[CHOSEN_HANDLE].tp.own_idx;
        
            for(int i = 4; i < 244; i++){
                if(temp_waiting > 255){
                    b = 255;
                }else{
                    b = temp_waiting;
                }
                package_storage[package_head].data[i] = b;
                temp_waiting -= b;
                if(temp_waiting <= 0){break;}
            }
            package_storage[package_head].length = len;

            package_head = (package_head+1)%package_storage_size;
            package_count++;

            if(pref_counter == 255){
                pref_counter = 0;
                batch_counter++;
            }else{
                pref_counter ++;
            }
            periodic_waiting_bytes = periodic_waiting_bytes - (len - 3);
            if(pref_counter % 50 == 0){
                send_usb_data(true, "Periodic prefixes sent up to %d b, %d p, %d waiting bytes", batch_counter, pref_counter, periodic_waiting_bytes);
            }
        }
    }
}

void add_other_device_periodic_bytes_at_start(char id, int new_bytes){
//    //periodic_waiting_bytes += new_bytes;
//    int bytes_done = 0;
//    uint16_t len = 244;
//    uint8_t p = 0;
//    uint8_t b = 0;
//    //int storage_limit = package_storage_size - 15;
//    //if(id == 'V' || id == 'G'){
//    //    storage_limit = package_storage_size - 1;
//    //}
//    while(bytes_done < new_bytes){
//        if(package_count >= 150){// No memory

//            periodic_waiting_bytes = periodic_waiting_bytes - bytes_done;
//            return;
//        }
//        else{

//            if(new_bytes - bytes_done < 241){
//                len = new_bytes - bytes_done + 3;
//            }
            
//            memset(package_storage[package_head].data, 'a', MAX_PACKAGE_SIZE);
//            package_storage[package_head].data[0] = batch_counter;//b;
//            package_storage[package_head].data[1] = pref_counter;//p;
//            package_storage[package_head].data[2] = DEVICE_ID;
//            package_storage[package_head].data[3] = devices[CHOSEN_HANDLE].tp.own_idx;
//            package_storage[package_head].length = len;

//            package_head = (package_head+1)%package_storage_size;
//            if(pref_counter == 255){
//                pref_counter = 0;
//                batch_counter++;
//            }else{
//                pref_counter ++;
//            }
//            package_count++;
//            //p ++;
//            //if(pref_counter == 0){
//            //    b++;
//            //}
//            bytes_done += len - 3;
//        }
//    }
}

void send_periodic_tlv(void){//(ble_nus_t *p_nus, uint16_t CHOSEN_HANDLE) {
//void send_relay_tlv(void){//(ble_nus_t *p_nus, uint16_t CHOSEN_HANDLE) {
    while(package_count > 0){
        package_storage[package_tail].data[3] = devices[CHOSEN_HANDLE].tp.own_idx;
        ret_code_t err_code = ble_nus_data_send(mm_nus, package_storage[package_tail].data, &package_storage[package_tail].length, CHOSEN_HANDLE);
        if (err_code == NRF_ERROR_RESOURCES) {
            // Wait for BLE_NUS_EVT_TX_RDY before sending more
            return;
        }else if (err_code != NRF_SUCCESS)
        {
            send_usb_data(false, "Failed to send NUS message. Error: 0x%x", err_code);
            return;
        }else{
            //tp_timer_start(&devices[CHOSEN_HANDLE]);
            //send_usb_data(false, "Sent periodic: %d to Cent:%d", package_tail, conn_handle);
            if(package_storage[package_tail].data[1] % 125 == 0){// || (package_storage[package_tail].data[1] == 255 && package_storage[package_tail].length > 6)){
                if(package_storage[package_tail].data[2] == DEVICE_ID){
                    send_usb_data(true, "Sent Periodically up to B:%d, P:%d", package_storage[package_tail].data[0], package_storage[package_tail].data[1]);
                }else{
                    send_usb_data(true, "Relayed %c up to B:%d, P:%d", package_storage[package_tail].data[2], package_storage[package_tail].data[0], package_storage[package_tail].data[1]);
                }
                //if(package_storage[package_tail].data[2] >= 'I' && package_storage[package_tail].data[2] <= 'N'){
                //    char id = package_storage[package_tail].data[2] - 'I' + '1';
                //    char name[6] = "Peri0";
                //    name[4] = id;
                //    name[5] = '\0';
                //    send_usb_data(true, "Relayed %s up to B:%d, P:%d", name, package_storage[package_tail].data[0], package_storage[package_tail].data[1]);
                //}
                ////int h = 0;
                ////if(package_storage[package_tail].data[2] >= 'I'){
                ////    h = package_storage[package_tail].data[2] - 'I' + 0;
                ////}else{
                ////    h = package_storage[package_tail].data[2] - 'C' + 0;
                ////}
                ////if(h >= 0 && h <= 6){
                ////    send_usb_data(true, "Relayed %c up to B:%d, P:%d", devices[h].name, package_storage[package_tail].data[0], package_storage[package_tail].data[1]);
                ////}else{
                ////    send_usb_data(true, "Relayed up to B:%d, P:%d", package_storage[package_tail].data[0], package_storage[package_tail].data[1]);
                ////}
            }
            package_count--;
            package_tail = (package_tail+1)%package_storage_size;
        }
    }
}

void send_stored_tlv(void){
    ret_code_t err_code;
    uint16_t len = 244;
    sending_own_periodic = false;
    //if(finished_stored){
        //tlv_allDataSent[3] = batch_counter;
        //tlv_allDataSent[4] = pref_counter+255;//chunk_sent-1;
        ////send_tlv_from_p(CHOSEN_HANDLE, tlv_allDataSent, 5);
        //len = 5;
        //send_tlv_usb(true, "ALL STORED FINISHED", tlv_allDataSent, 5,5);
        //err_code = ble_nus_data_send(mm_nus, tlv_allDataSent, &len, CHOSEN_HANDLE);

        //if(err_code == NRF_SUCCESS){
        //    sending_stored_tlv = false;
        //    //finished_stored = false;
        //    sending_own_periodic = true;
        //    send_usb_data(false, "Sent ALL STORED DATA, stored bytes = %d, sent = ", stored_bytes, stored_bytes_sent);

        //    pref_counter ++;
        //    if(pref_counter == 0){
        //        batch_counter++;
        //    }
        //    add_periodic_bytes(0);
            
        //    //if(IS_RELAY){
        //    //    send_tlv_from_c(-3, tlv_reqData, 2, "Sent start Sending req data");
        //    //    sending_periodic_tlv = true;
        //    //    sending_own_periodic = false;
        //    //    if(IS_SENSOR_NODE){
        //    //        send_tlv_from_c(-3, tlv_reqData, 2, "Sent start Sending req data");
        //    //        sending_periodic_tlv = true;
        //    //        if(!periodic_timer_started){
        //    //            periodic_timer_start();
        //    //            periodic_timer_started = true;
        //    //        }
        //    //        sending_own_periodic = true;
        //    //        //send_periodic_tlv();
        //    //    }
        //    //}
        //    send_periodic_tlv();
        //    //updating_ongather_conn = true;
        //    //update_all_conn_for_gather();
        //}
    //}

    while(stored_bytes_sent < stored_bytes){
        
        if(stored_bytes - stored_bytes_sent < 241){
            len = stored_bytes - stored_bytes_sent + 3;
        }
        //dummy_data[0] = batch_counter;
        err_code = ble_nus_data_send(mm_nus, dummy_data, &len, CHOSEN_HANDLE);
        if (err_code == NRF_ERROR_RESOURCES) {
            // Wait for BLE_NUS_EVT_TX_RDY before sending more
            return;
        }else if (err_code == NRF_ERROR_INVALID_STATE){
            send_usb_data(false, "Invalid state ERROR for sending stored. Stop sending.");
            //package_tail = (package_tail+255)%package_storage_size;
            sending_stored_tlv = false;
            return;
        }else if (err_code != NRF_SUCCESS)
        {
            send_usb_data(false, "Failed to send NUS message. Error: 0x%x", err_code);
            return;
        }else{
            //package_count--;
            //package_tail = (package_tail+1)%package_storage_size;
            //tp_timer_start(&devices[CHOSEN_HANDLE]);
            stored_bytes_sent += len - 3;

            if (batch_counter % 5 == 20 && stored_bytes_sent < stored_bytes) {
                send_usb_data(true, "Sent stored data up to batch %d to handle %d, %d/%d", batch_counter, CHOSEN_HANDLE,  ((int)batch_counter*256)+(int)pref_counter, stored_bytes/241);
            }
            //else if (pref_counter == 0) {
            //    send_usb_data(true, "Sent stored data to %s, packets sent = %d/%d", devices[CHOSEN_HANDLE].name, (batch_counter*256)+pref_counter, stored_bytes/241);
            //}

            if(stored_bytes_sent >= stored_bytes){
                finished_stored = true;
                tlv_allDataSent[3] = batch_counter;
                tlv_allDataSent[4] = pref_counter;//chunk_sent-1;
                send_tlv_from_p(CHOSEN_HANDLE, tlv_allDataSent, 5);
                //memcpy(devices[CHOSEN_HANDLE].peri.send_finish_p, tlv_allDataSent, 5);
                //first_packet_timer_start(&devices[CHOSEN_HANDLE]);
                len = 5;
                send_tlv_usb(true, "ALL STORED FINISHED, check miss timer:", tlv_allDataSent, 5,5,CHOSEN_HANDLE);
                //err_code = ble_nus_data_send(mm_nus, tlv_allDataSent, &len, CHOSEN_HANDLE);
            }else if(!finished_stored){
                //pref_counter = (pref_counter+1)%255;
                if(stored_bytes_sent > og_stored_bytes && !og_signal_sent){
                    uint8_t og[3] = {74, batch_counter, pref_counter};
                    send_tlv_from_p(CHOSEN_HANDLE, og, 3);
                    og_signal_sent = true;
                }


                if(pref_counter == 255){
                    pref_counter = 0;
                    batch_counter++;
                }else{
                    pref_counter ++;
                }
                dummy_data[0] = batch_counter;
                dummy_data[1] = pref_counter;
                //dummy_data[2] = devices[CHOSEN_HANDLE].tp.own_idx;
                dummy_data[2] = DEVICE_ID;
                dummy_data[3] = devices[CHOSEN_HANDLE].tp.own_idx;
            }
        }         
    }   
}

void send_missing_tlv(void){
    ret_code_t err_code;
    uint16_t len = 244;
    int m_idx = devices[CHOSEN_HANDLE].peri.miss_idx;
    int final_idx = devices[CHOSEN_HANDLE].peri.final_miss_idx;
    int b_idx = devices[CHOSEN_HANDLE].peri.miss_b_idx;
    while (m_idx <= final_idx) {
        dummy_data[0] = devices[CHOSEN_HANDLE].peri.missed[b_idx];
        dummy_data[1] = devices[CHOSEN_HANDLE].peri.missed[m_idx];
        dummy_data[2] = DEVICE_ID;
        err_code = ble_nus_data_send(mm_nus, dummy_data, &len, CHOSEN_HANDLE);

        if (err_code == NRF_ERROR_RESOURCES) {
            // Wait for BLE_NUS_EVT_TX_RDY before sending more
            return;
        }else if (err_code == NRF_ERROR_INVALID_STATE){
            send_usb_data(false, "Invalid state ERROR for sending stored. Stop sending.");
            //package_tail = (package_tail+255)%package_storage_size;
            sending_stored_tlv = false;
            return;
        }else if (err_code != NRF_SUCCESS)
        {
            send_usb_data(false, "Failed to send NUS message. Error: 0x%x", err_code);
            return;
        }else{
            if(m_idx == final_idx){
                m_idx = 255;
            }
            else if(m_idx - 1 - b_idx >= devices[CHOSEN_HANDLE].peri.missed[b_idx+1]){
                b_idx += 2 + devices[CHOSEN_HANDLE].peri.missed[b_idx+1]; 
                devices[CHOSEN_HANDLE].peri.miss_b_idx = b_idx;
                m_idx = b_idx+2;
                devices[CHOSEN_HANDLE].peri.miss_idx = m_idx;
            }else{
                m_idx++;
                devices[CHOSEN_HANDLE].peri.miss_idx = m_idx;
            }
        }
    }
    if(!finished_missing){
        finished_missing = true;
        devices[CHOSEN_HANDLE].peri.miss_rec = false;
        uint8_t tlv_missedSent[5] = {254, 255, DEVICE_ID};
        //memcpy(devices[CHOSEN_HANDLE].peri.send_finish_p, tlv_missedSent, 5);
        //first_packet_timer_start(&devices[CHOSEN_HANDLE]);
        send_tlv_from_p(CHOSEN_HANDLE, tlv_missedSent, 3);
        send_tlv_usb(true, "ALL Missed Sent, check timer:", tlv_missedSent, 3,3,CHOSEN_HANDLE);
    }
}

void send_chunk_tlv(void){//ble_nus_t *p_nuss, uint16_t conn_handlee) {
    ret_code_t err_code;
    //OPTIONAL: Package storage
    while (package_count > 0) {
        err_code = ble_nus_data_send(mm_nus, package_storage[package_tail].data, &package_storage[package_tail].length, CHOSEN_HANDLE);
        if (err_code == NRF_ERROR_RESOURCES) {
            // Wait for BLE_NUS_EVT_TX_RDY before sending more
            return;
        }else if (err_code == NRF_ERROR_INVALID_STATE){
            send_usb_data(false, "Invalid state ERROR for sending chunks. Stop sending.");
            package_tail = (package_tail+255)%package_storage_size;
            sending_tlv_chunks = false;
            return;
        }else if (err_code != NRF_SUCCESS)
        {
            send_usb_data(false, "Failed to send NUS message. Error: 0x%x", err_code);
            return;
        }else{
            package_count--;
            package_tail = (package_tail+1)%package_storage_size;
        }
        // Log progress every 10 chunks
        if (((package_tail+255)%package_storage_size) % 50 == 0 || package_count == 0) {
            send_usb_data(true, "Sent Chunks up to %d to handle %d", (package_tail+255)%package_storage_size, CHOSEN_HANDLE);
        }
    }

    // Handle the last message if necessary
    //if (last) {
    if(more_data_left){
        if(!large_requested){
            send_usb_data(false, "Sent all large, data left:%d", more_data_left);
            sending_tlv_chunks = false;
            //set_cent_info(mm_nus, CHOSEN_HANDLE);
            request_usb_data();
        }
    }else{
        
        send_usb_data(false, "Sent and Rec ALL bacthes, data left:%d", more_data_left);
        //tlv_allDataSent[1] = DEVICE_ID;
        tlv_allDataSent[3] = (uint8_t) large_counter;
        tlv_allDataSent[4] = (uint8_t) (package_tail+255)%package_storage_size;//chunk_sent-1;
        
        //uint16_t length = 5;

        //err_code = ble_nus_data_send(mm_nus, tlv_allDataSent, &length, CHOSEN_HANDLE);
      //if(err_code == NRF_SUCCESS){
      //send_tlv_from_p(conn_handle, tlv_allDataSent, 5);
        send_tlv_from_p(CHOSEN_HANDLE, tlv_allDataSent, 5);
        //uint8_t tlv[3] = {0,1, DEVICE_ID};
        if(SEND_TYPE != 0){
        #if RELAY_REQ_ON_QUEUE
            if(on_req_queue != NULL && !devices[on_req_queue->handle].peri.is_sending_on_req){
                if(on_req_queue != NULL){
                    if(devices[on_req_queue->handle].connected){
                        update_conn_params(on_req_queue->handle, on_request_conn_params);
                        calibrate_conn_params(on_req_queue->handle, 10);
                        send_tlv_from_c(on_req_queue->handle, tlv_reqData, 2, "Sent start Sending req data");
                        devices[on_req_queue->handle].peri.is_sending_on_req = true;
                    }else if(on_req_queue->next != NULL && devices[on_req_queue->next->handle].connected){
                        on_req_queue_t *new_req = (on_req_queue_t *)malloc(sizeof(on_req_queue_t));
                        new_req->handle = on_req_queue->handle;
                        new_req->id = on_req_queue->id;
                        //new_req->connected = false;
                        new_req->next = NULL;
                        last_on_req_queue->next = new_req;
                        last_on_req_queue = new_req;

                        on_req_queue = on_req_queue->next;
                        update_conn_params(on_req_queue->handle, on_request_conn_params);
                        calibrate_conn_params(on_req_queue->handle, 10);
                        send_tlv_from_c(on_req_queue->handle, tlv_reqData, 2, "Sent start Sending req data");
                        devices[on_req_queue->handle].peri.is_sending_on_req = true;
                    }else{
                        //TODO: start scanning to connect to peri
                        if(!scanning_for_on_req){
                            strcpy(m_target_periph_name, devices[on_req_queue->handle].name);
                            scanning_for_on_req = true;
                            send_usb_data(true, "Start to scan for missing On_REQ name = %s", m_target_periph_name);
                            scan_start();
                        }
                    }
                }
            }
            if(there_are_stopped){
                send_tlv_from_c(-4, tlv_resumeData, 1, "Resume periodic.");
            }
        #elif  RELAY_PERIS_CHOOSE
            //TODO: Change conn intervals
            if(count_onreq_not_phone > 0){
                calculate_peri_to_relay_on_request_params(count_onreq_not_phone);
                update_onreq_conn(10);
                send_tlv_from_c(-3, tlv_reqData, 2, "Sent start Sending req data");
            }
            //send_usb_data(true, "Peri1 isPeriodic=%d send request data.", devices[0].peri.isPeriodic);
            
            if(there_are_stopped){
                send_tlv_from_c(-4, tlv_resumeData, 1, "Resume periodic.");
            }
        #else
            //data_relay_in_process = true;
            send_usb_data(false, "Send start to peris: pckgTail=%d, pckgHead=%d, counter=%d", package_tail, package_head, package_count);
            send_tlv_from_c(-3, tlv_reqData, 2, "Sent start Sending to Peris");
            send_tlv_from_c(-4, tlv_resumeData, 1, "Resume periodic.");
        #endif
        }
        start_send_data_not_received = true;
        sending_tlv_chunks = false;
      //}else{
      //    send_usb_data(false, "Failed to send tlv_allDataSent message. Error: 0x%x or %d", err_code, err_code);
      //}
        
    }
}

void send_specific_tlv(void){//(ble_nus_t * p_nus, uint16_t conn_handle) {
    //ret_code_t err_code;

    ////OPTIONAL: Package storage
    //while(curr_not_sent != NULL){
    //    package_storage[curr_not_sent->offset].data[0] = 255;
        
    //    err_code = ble_nus_data_send(mm_nus, package_storage[curr_not_sent->offset].data, &package_storage[curr_not_sent->offset].length, CHOSEN_HANDLE);
    //    if (err_code == NRF_ERROR_RESOURCES) {
    //        return;
    //    } else if (err_code != NRF_SUCCESS) {
    //        send_usb_data(false, "Failed to send NUS SPEC message. Error: 0x%x", err_code);
    //        return;
    //    }
    //    send_usb_data(false, "Sent missing chunk: %d", curr_not_sent->offset);
    //    curr_not_sent = curr_not_sent->next;
    //}

    //if (last) {
    //    uint8_t tlv[3] = {0};
    //    tlv[0] = 255;
    //    tlv[1] = last_received_by_cent;
    //    tlv[2] = send_missing_request_counter;
    //    uint16_t length = 3;
    //    err_code = ble_nus_data_send(mm_nus, tlv, &length, CHOSEN_HANDLE);
    //    if (err_code == NRF_ERROR_RESOURCES) {
    //        return;
    //    } else if (err_code != NRF_SUCCESS) {
    //        send_usb_data(false, "Failed to send NUS last message. Error: 0x%x", err_code);
    //        return;
    //    }
    //    send_tlv_usb(false, "Sent last spec: ", tlv,length,length,CHOSEN_HANDLE);
    //    //test
    //    sending_tlv_spec = 0;
    //    last = false;
    //    return;
    //}
}



//void send_periodic_tlv(void){//(ble_nus_t *p_nus, uint16_t conn_handle) {
//    if(package_count > 0){
//        ret_code_t err_code = ble_nus_data_send(mm_nus, package_storage[package_tail].data, &package_storage[package_tail].length, CHOSEN_HANDLE);
//        if (err_code == NRF_ERROR_RESOURCES) {
//            // Wait for BLE_NUS_EVT_TX_RDY before sending more
//            return;
//        }else if (err_code != NRF_SUCCESS)
//        {
//            send_usb_data(false, "Failed to send NUS message. Error: 0x%x", err_code);
//            return;
//        }else{
//            package_count--;
//            package_tail = (package_tail+1)%package_storage_size;
//        }
//    }
//}

ret_code_t send_specific_tlv_to_central(uint16_t h, int tail){
    return ble_nus_data_send(mm_nus, devices[h].tlvs[tail].data, &devices[h].tlvs[tail].length, h);
}

void send_tlv_to_cent(void) {
   ret_code_t err_code;
    //OPTIONAL: Package storage
    int sent_now = 0;
    while (cent_tlv_count > 0) {
        if(cent_tlv_storage[cent_tlv_tail].length != 0){
            err_code = ble_nus_data_send(mm_nus, cent_tlv_storage[cent_tlv_tail].data, &cent_tlv_storage[cent_tlv_tail].length, cent_tlv_storage[cent_tlv_tail].handle);
            if (err_code == NRF_ERROR_RESOURCES) {
                // Wait for BLE_NUS_EVT_TX_RDY before sending more
                return;
            }else if (err_code == NRF_ERROR_INVALID_STATE){
                send_tlv_usb(false, "ERR: Invalid state on send_tlv_to", cent_tlv_storage[cent_tlv_tail].data, cent_tlv_storage[cent_tlv_tail].length, cent_tlv_storage[cent_tlv_tail].length, cent_tlv_storage[cent_tlv_tail].handle);
                cent_tlv_count--;
                cent_tlv_tail = (cent_tlv_tail+1)%cent_tlv_storage_size;
                return;
            }
            else if (err_code != NRF_SUCCESS)
            {
                char msg[300] = "Failed to send_tlv_to_cent. ";
                int pos = 31;
                pos += sprintf(msg+pos, "Error 0x%x:", err_code);
                send_tlv_usb(false, msg, cent_tlv_storage[cent_tlv_tail].data, cent_tlv_storage[cent_tlv_tail].length, cent_tlv_storage[cent_tlv_tail].length, cent_tlv_storage[cent_tlv_tail].handle);
                return;
            }else{
                //char msg2[300] = "";
                //sprintf(msg2, "%s data sent", devices[cent_tlv_storage[cent_tlv_tail].handle].name);
                //send_tlv_usb(true, "Sent:", cent_tlv_storage[cent_tlv_tail].data, cent_tlv_storage[cent_tlv_tail].length, cent_tlv_storage[cent_tlv_tail].length, cent_tlv_storage[cent_tlv_tail].handle);
                cent_tlv_count--;
                cent_tlv_tail = (cent_tlv_tail+1)%cent_tlv_storage_size;
            }
        }else{
            cent_tlv_count--;
            cent_tlv_tail = (cent_tlv_tail+1)%cent_tlv_storage_size;
        }
    }
    

}

int send_tlv_from_p(int16_t h, uint8_t* tlv, uint16_t length){
    ret_code_t err_code;

    if(h != -1){
        //uint16_t handle = h;
        //memcpy(cent_tlv_storage[cent_tlv_head].data, tlv,length);
        //cent_tlv_storage[cent_tlv_head].length = length;
        //cent_tlv_storage[cent_tlv_head].handle = handle;
        //cent_tlv_head = (cent_tlv_head+1)%cent_tlv_storage_size;
        //if(cent_tlv_count == cent_tlv_storage_size){
        //    send_usb_data(false, "Send to cent max size storage reached");
        //}else{
        //  cent_tlv_count++;
        //}
        add_to_device_tlv_strg(h, tlv, length);
    }else{
        for (uint8_t i=NRF_SDH_BLE_CENTRAL_LINK_COUNT; i<NRF_SDH_BLE_TOTAL_LINK_COUNT; i++){
          if(devices[i].connected){
            //uint16_t handle = h;
            //memcpy(cent_tlv_storage[cent_tlv_head].data, tlv,length);
            //cent_tlv_storage[cent_tlv_head].length = length;
            //cent_tlv_storage[cent_tlv_head].handle = handle;
            //cent_tlv_head = (cent_tlv_head+1)%cent_tlv_storage_size;
            //if(cent_tlv_count == cent_tlv_storage_size){
            //    send_usb_data(false, "Send to cent max size storage reached");
            //}else{
            //  cent_tlv_count++;
            //}
            add_to_device_tlv_strg(i, tlv, length);
          }
        }
    }
    //send_tlv_to_cent();
    return 0;
}
bool waiting_request_received_from_phone = false;
void nus_data_handler(ble_nus_evt_t * p_evt)
{
    if (p_evt == NULL)
    {
        return;
    }
    uint16_t handle = p_evt->conn_handle;
    switch (p_evt->type)
    {
        case BLE_NUS_EVT_RX_DATA:
        {   
            NRF_LOG_HEXDUMP_INFO(p_evt->params.rx_data.p_data, p_evt->params.rx_data.length);
            
            //send_usb_data(false, "REC: %s",p_evt->params.rx_data.p_data);
            int8_t rssi = 0;
            uint8_t channel = 0;
            

            //ret_code_t err_code = sd_ble_gap_rssi_get(handle, &rssi, &channel);

            // Process the received data
            // For example, you can echo the received data back to the central device
            
            uint8_t * received_data = p_evt->params.rx_data.p_data;
            uint16_t received_len = p_evt->params.rx_data.length;
            //send_usb_data(false, "Received RSSI:%d, e:%d, len:%d", rssi, err_code, received_len);
            if(received_data[0] == 255 && received_data[1] == 255 && received_data[2] == 255 && received_data[3] == 255){
                send_tlv_usb(false, "USBerr", received_data,4,received_len, handle);
                break;
            }

            //if(received_len < 11){
            //    if(received_len == devices[handle].last_len){
            //        if(memcmp(received_data,devices[handle].last_rec,received_len) == 0){
            //            return;
            //        }
            //    }
            //    devices[handle].last_len = received_len;
            //    memcpy(devices[handle].last_rec, received_data, received_len);
            //}else{
            //    devices[handle].last_len = 0;
            //}
            //if(!send_test_rssi && received_data[0] != 20){
            //    char msgrssi[200] = "";
            //    sprintf(msgrssi, "Received RSSI=%d: ", rssi);
                if(received_len > 30){
                    send_tlv_usb(true, "GOT MSG:",received_data,30,30,handle);
                }else if(received_len>0){
                    send_tlv_usb(true, "GOT MSG:",received_data,received_len,received_len,handle);
                }
            //}
            //uint8_t first_rec = received_data[1];
            
            int command_id = (int) received_data[0];
            //send_usb_data(false, "Got message command=%d, is3=%d, is6=%d, is7=%d", received_data[0], received_data[0]==3, received_data[0] == 6, received_data[0] == 7);
            //NEW: 1 number commands
            //switch(command_id)
            //{
                if(PERIS_DO_TP){
                    //tp_timer_start(&devices[handle]);
                }
                if(received_data[0] == 0)//case 0:
                {// Phone sent start gather to relay
                    if (!data_gather_relay_on){
                        data_gather_relay_on = true;
                       

                        CHOSEN_HANDLE = PHONE_HANDLE;
                        if(received_len == 2){//3){
                            uint8_t tlv_phoneScanPeris[10] = {8};
                            int count_phoneScanPeris = 1;
                        
                            if(ble_conn_state_central_conn_count()>0){// && received_len == 3){
                                    for(int i = 0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
                                        if(devices[i].connected && devices[i].id != 'V' && devices[i].id != 'G' &&
                                            devices[i].peri.type <= 2 && !devices[i].peri.askedAdvPhone)
                                            //devices[handle].tp.own_idx <= devices[i].tp.max_tx_idx ||
                                            //devices[handle].tp.ema_rssi + tx_range[devices[handle].tp.own_idx] - tx_range[devices[i].tp.max_tx_idx] > -70 )//||
                                            ////devices[handle].tp.ema_rssi> -72 ||
                                            ////devices[handle].tp.last_rssi > -72)// TP check
                                            //&& (devices[i].peri.type == 1 || devices[i].peri.type == 2)
                                            //&& !devices[i].peri.askedAdvPhone)  // allowed types for direct sending
                                        {
                                            //if(!devices[i].peri.isPeriodic){
                                            send_tlv_from_c(i, tlv_advertiseSink, 4, "Type 1 start advertising");
                                            send_usb_data(false, "%s Start adv for phone", devices[i].name);
                                            //count_onreq_not_phone--;
                                            tlv_phoneScanPeris[count_phoneScanPeris] = devices[i].id;
                                            count_phoneScanPeris++;
                                            //relay_info.count_type1_sink++;
                                            send_usb_data(false, "Sent adv for Phone to %s, count phone scan peris = %d", devices[i].name, relay_info.count_type1_sink);
                                            devices[i].peri.chosenRelay = false;
                                            devices[i].peri.askedAdvPhone = true;
                                            devices[i].peri.firstAskAdv = true;
                                            //if(count_phoneScanPeris==3){break;}
                                            //=
                                        }
                                    }
                                     send_usb_data(false, "Send PhoneScanPeris, count phone scan peris = %d, t1_REL=%d, t1_Sink", count_phoneScanPeris, relay_info.count_type1, relay_info.count_type1_sink);
                                    if(count_phoneScanPeris > 1){
                                        send_tlv_from_p(handle, tlv_phoneScanPeris, count_phoneScanPeris);
                                    }else{
                                        send_tlv_from_p(handle, tlv_phoneScanPeris, 1);
                                    }
                                    //sending_periodic_tlv = true;
                            }
                        }//else 
                        if(received_len==2){

                            if(!sending_stored_tlv && !sending_periodic_tlv){
                                //TODO: check if these two below are needed
                                ////calculate_peri_to_relay_on_request_params(count_onreq_not_phone);
                                ////update_onreq_conn(10);

                                ////send_tlv_from_c(-3, tlv_reqData, 2, "Sent start Sending req data");
                                //for(int i = 0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
                                //    if(devices[i].connected
                                //        && (devices[i].peri.type == 3) //|| devices[i].peri.type == 2)
                                //        && !devices[i].peri.askedAdvPhone
                                //        && devices[i].peri.chosenRelay) // allowed types for direct sending
                                //    {
                                //        //devices[i].tp.use_pdr = true;
                                //        send_tlv_from_c(i, tlv_reqData, 2, "Sent start Sending req data");
                                //        set_other_device_op_mode(i, 1);
                                //    }
                                //}
                                devices[handle].conn_params = conn_params_phone;
                                if(IS_RELAY){
                                    tlv_reqData[0] = 1;
                                    for(int i = 0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
                                        if(devices[i].peri.chosenRelay && devices[i].connected){// && (devices[i].peri.type != 3)){
                                            set_other_device_op_mode(i, 1);
                                            //send_tlv_from_c(i, tlv_reqData, 2, "Sent start Sending req data");
                                            //send_tlv_usb(true, "Sent start Sending req data", tlv_reqData, 2,2, i);
                                        }
                                    }
                                    //send_usb_data(true, "Phone updated, update all peris");
                                    ////devices[h].conn_params = conn_params_phone;
                                    //updating_relay_conns = true;
                                    //updated_phone_conn = true;
                                    //updating_relay_conns = update_all_relay_conn_tlv(empty_tlv, 0);
                                }

                                //uint8_t tlv_phoneScanPeris[10] = {8};
                                //int count_phoneScanPeris = 1;
                            
                                //if(ble_conn_state_central_conn_count()>0 && received_len == 3){
                                //        for(int i = 0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
                                //            if(devices[i].connected && devices[i].id != 'V' && devices[i].id != 'G' &&
                                //                devices[i].peri.type <= 2 && !devices[i].peri.askedAdvPhone)
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
                                //                devices[i].peri.firstAskAdv = true;
                                //                //if(count_phoneScanPeris==3){break;}
                                //                //=
                                //            }
                                //        }
                                //         send_usb_data(false, "Send PhoneScanPeris, count phone scan peris = %d, t1_REL=%d, t1_Sink", count_phoneScanPeris, relay_info.count_type1, relay_info.count_type1_sink);
                                //        if(count_phoneScanPeris > 1){
                                //            send_tlv_from_p(handle, tlv_phoneScanPeris, count_phoneScanPeris);
                                //        }else{
                                //            send_tlv_from_p(handle, tlv_phoneScanPeris, 1);
                                //        }
                                //        sending_periodic_tlv = true;
                                //}

                           

                            
                                ////update_conn_params(handle, conn_params_phone);
                                //updating_ongather_conn = true;
                                //update_all_conn_for_gather();
                                sending_own_periodic = false;
                                if(IS_SENSOR_NODE && stored_bytes_sent < stored_bytes){
                                    sending_stored_tlv = true;
                                    send_stored_tlv();
                                    bsp_board_leds_off();
                                    bsp_board_led_on(led_GREEN);
                                }else if(IS_SENSOR_NODE){
                                    //send_tlv_from_c(-3, tlv_reqData, 2, "Sent start Sending req data");
                                    sending_periodic_tlv = true;
                                    if(!periodic_timer_started){
                                        periodic_timer_start();
                                        periodic_timer_started = true;
                                        bsp_board_leds_off();
                                        bsp_board_led_on(led_BLUE);
                                    }
                                    sending_own_periodic = true;
                                    //send_periodic_tlv();
                                }
                                if(ble_conn_state_central_conn_count()>0){
                                    sending_periodic_tlv = true;
                                    send_periodic_tlv();
                                }
                                //updating_ongather_conn = true;
                                //update_all_conn_for_gather();


                                //uint8_t tlv_otherName[4] = {3};
                                //for(int i=0;i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
                                //    if(devices[i].connected && !devices[i].peri.askedAdvPhone && devices[i].id != 255){
                                //        tlv_otherName[1] = devices[i].id;
                                //        tlv_otherName[2] = devices[i].peri.type;
                                //        tlv_otherName[3] = devices[i].tp.tx_idx;
                                //        //send_tlv_from_p(handle, tlv_otherName, 4);
                                //    }
                                //}
                            }
                        }
                    }
                    //break;
                }else if(received_data[0] == 11 && sending_stored_tlv){
                    if(handle == PHONE_HANDLE){
                        devices[handle].conn_params = conn_params_phone;
                        if(IS_RELAY){
                            tlv_reqData[0] = 1;
                            for(int i = 0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
                                if(devices[i].peri.chosenRelay && devices[i].connected){// && (devices[i].peri.type != 3)){
                                    set_other_device_op_mode(i, 1);
                                    if(devices[i].id == 'M'){
                                        set_other_device_op_mode(i, 4);
                                        //update_conn_params(h, devices[h].conn_params);
                                    }
                                    send_tlv_from_c(i, tlv_reqData, 2, "Sent start Sending req data");
                                    send_tlv_usb(true, "Sent start Sending req data", tlv_reqData, 2,2, i);
                                }
                            }
                            send_usb_data(true, "Phone updated, update all peris");
                            //devices[h].conn_params = conn_params_phone;
                            updating_relay_conns = true;
                            updated_phone_conn = true;
                            updating_relay_conns = update_all_relay_conn_tlv(fast_tlv, 1);
                        }
                    }
                    //else{
                    //    devices[handle].conn_params = conn_params_stored;
                    //}
                    //if(devices[PHONE_HANDLE].connected && !waiting_request_received_from_phone){
                    //    waiting_request_received_from_phone = true;
                    //    if(waiting_to_send_phone){
                    //        devices[PHONE_HANDLE].conn_params = conn_params_phone;
                    //        //waiting_to_send_phone = false;
                    //    }else if(sending_periodic_tlv){
                    //        devices[PHONE_HANDLE].conn_params = conn_params_periodic;
                    //    }
                    //    if (!sending_stored_tlv){
                    //        uint8_t not_stored_tlv[2] = {11, 11};
                    //        if(sending_periodic_tlv){
                    //            send_tlv_from_p(PHONE_HANDLE, not_stored_tlv, 1);
                    //        }else{
                    //            send_tlv_from_p(PHONE_HANDLE, not_stored_tlv, 2);
                    //        }
                    //    }
                    //    update_conn_params(PHONE_HANDLE, devices[PHONE_HANDLE].conn_params);

                    //}
                }else if(received_data[0] == 12 && received_len == 4){
                    //if(IS_RELAY && (char) received_data[1] != DEVICE_ID){
                    //    for(int j=0;j<NRF_SDH_BLE_CENTRAL_LINK_COUNT;j++){
                    //        if(received_data[1] == devices[j].id){
                    //            if(devices[handle].connected){
                    //                send_tlv_from_c(j, received_data, received_len, "tell start sending at prefs");
                    //                devices[handle].peri.last = received_data[2];
                    //                devices[handle].peri.last = received_data[3];
                    //            }
                    //            return;
                    //        }
                    //    }
                    //}else if(handle == RELAY_HANDLE){
                    //    CHOSEN_HANDLE = RELAY_HANDLE;
                    //    int resend_bytes = 0;
                    //    if(batch_counter > received_data[3]){
                    //        resend_bytes = 241 * (255 - received_data[2] + pref_counter);
                    //    }else{
                    //        resend_bytes = 241 * (pref_counter - received_data[2]);
                    //    }
                    //    stored_bytes_sent += resend_bytes;
                    //    batch_counter = received_data[3];
                    //    pref_counter = received_data[2];
                    //    dummy_data[0] = batch_counter;
                    //    dummy_data[1] = pref_counter;
                    //}
                }
                else if(received_data[0] == 1 && received_len == 2)//case 1:
                {// To Peris: Start sending data
                    //if (start_send_data_not_received && !sending_tlv_spec && !sending_tlv_chunks){
                    if (!sending_stored_tlv && !sending_periodic_tlv){
                        if(received_len == 2 && received_data[1] == 'P'){// if sender is Phone
                            PHONE_HANDLE = handle;
                            strcpy(devices[handle].name, "Phone");
                            devices[handle].cent.isPhone = true;
                            connectedToPhone = true;
                            is_sending_to_phone = true;
                            //calculate_on_request_params(count_onreq_not_phone);
                            //update_conn_params(handle, on_request_conn_params);
                            //disconnect_handle(RELAY_HANDLE);
                            //update_conn_params(handle, conn_params_phone);
                            bsp_board_leds_off();
                            bsp_board_led_on(led_BLUE);
                        }
                        //start_send_data_not_received = false;
                        CHOSEN_HANDLE = handle;
                        
                        
                        sending_own_periodic = false;
                        if(stored_bytes_sent < stored_bytes){
                            sending_stored_tlv = true;
                            send_stored_tlv();
                            bsp_board_led_on(led_GREEN);
                        }else{
                            sending_periodic_tlv = true;
                            sending_own_periodic = true;
                            if(!periodic_timer_started){
                                periodic_timer_start();
                                periodic_timer_started = true;
                            }
                            send_periodic_tlv();
                            bsp_board_led_on(led_BLUE);
                        }
                            
                        uint8_t filed_conn_phone[2] = {72, 1};
                        if(devices[RELAY_HANDLE].connected && handle == PHONE_HANDLE){
                            send_usb_data(false, "Relay handle is %d, conn?=", RELAY_HANDLE, devices[RELAY_HANDLE].connected);
                            send_tlv_from_p(RELAY_HANDLE, filed_conn_phone, 2);
                        }
                        //uint8_t started_sending_tlv[2] = {12, 12};
                        //if(received_data[1] == 'P'){
                        //    if(sending_stored_tlv){
                        //        send_tlv_from_p(PHONE_HANDLE, started_sending_tlv, 1);
                        //    }else if(sending_periodic_tlv){
                        //        send_tlv_from_p(PHONE_HANDLE, started_sending_tlv, 2);
                        //    }
                        //}
                    }
                    //break;
                }
                else if(received_data[0] == 2 && !send_test_rssi)//case 2:
                {
                    //if(!periodic_timer_started){
                    //    periodic_timer_start();
                    //    periodic_timer_started = true;
                    //}
                    //break;
                }
                else if(received_data[0] == 36 && received_len == 2){
                    devices[handle].tp.other_tx_idx = received_data[1];
                    if(!PERIS_DO_TP){
                        devices[handle].tp.tx_idx = received_data[1];
                    }
                    devices[handle].tp.requested_tx_idx = -1;
                    if(devices[handle].tp.received_counter != -1){
                        devices[handle].tp.received_counter = 0;
                    }
                    send_usb_data(true, "INIT OTHERS %s:%d to ME to %d dBm, idx:%d, PERISdoTPC = %d", devices[handle].name, handle, devices[handle].tp.tx_range[devices[handle].tp.other_tx_idx], devices[handle].tp.other_tx_idx, PERIS_DO_TP);
                }

                else if(received_data[0] == 99 && received_len <= 3 ){
                    //devices[handle].tp.tx_idx = received_data[1];
                    //devices[handle].tp.requested_tx_idx = -1;
                    //if(devices[handle].tp.received_counter != -1){
                    //    devices[handle].tp.received_counter = 0;
                    //}
                    do_rssi_ema_self(handle, (int8_t) received_data[1]);
                }
                else if(received_data[0] == 13 && received_len == 1 && IS_RELAY){
                    uint8_t tlv_phoneScanPeris1[10] = {8};
                    int count_phoneScanPeris1 = 1;
                    if(ble_conn_state_central_conn_count()>0){
                        for(int i = 0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
                            if(devices[i].connected && 
                                devices[i].peri.type < 3 && !devices[i].peri.askedAdvPhone)
                                //devices[handle].tp.own_idx <= devices[i].tp.max_tx_idx ||
                                //devices[handle].tp.ema_rssi + tx_range[devices[handle].tp.own_idx] - tx_range[devices[i].tp.max_tx_idx] > -70 )//||
                                ////devices[handle].tp.ema_rssi> -72 ||
                                ////devices[handle].tp.last_rssi > -72)// TP check
                                //&& (devices[i].peri.type == 1 || devices[i].peri.type == 2)
                                //&& !devices[i].peri.askedAdvPhone)  // allowed types for direct sending
                            {
                                //if(!devices[i].peri.isPeriodic){
                                send_tlv_from_c(i, tlv_advertiseSink, 4, "Type 1 start advertising");
                                send_usb_data(false, "%s Start adv for phone", devices[i].name);
                                //count_onreq_not_phone--;
                                tlv_phoneScanPeris1[count_phoneScanPeris1] = devices[i].id;
                                count_phoneScanPeris1++;
                                //relay_info.count_type1_sink++;
                                send_usb_data(false, "Sent adv for Phone to %s, count phone scan peris = %d", devices[i].name, relay_info.count_type1_sink);
                                devices[i].peri.chosenRelay = false;
                                devices[i].peri.askedAdvPhone = true;
                                //devices[i].peri.firstAskAdv = true;

                                //if(count_phoneScanPeris1 == 3){
                                //    break;
                                //}
                                
                            }
                            
                        }
                    }
                    ////send_usb_data(false, "Send PhoneScanPeris, count phone scan peris = %d, t1_REL=%d, t1_Sink", count_phoneScanPeris, relay_info.count_type1, relay_info.count_type1_sink);
                    if(count_phoneScanPeris1 > 1){
                    //update_conn_params(handle, phone_scan_for_peris_conn_params);
                        send_tlv_from_p(handle, tlv_phoneScanPeris1, count_phoneScanPeris1);
                        //devices[handle].conn_params = conn_params_conn_phone_wait;
                    }else{
                        send_tlv_from_p(handle, tlv_phoneScanPeris1, 1);
                    }
                }
                else if(received_data[0] == 3){//TODO: NEW NAME CODE
                    if(devices[handle].connected || (received_len != 5 && received_len != 3)){//devices[handle].id == received_data[1] && devices[handle].connected){
                        return;
                    }
                    set_other_device_sim_params(handle, received_data);
                    sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_CONN, handle, tx_range[default_tx_idx]);
                    send_usb_data(true, "INIT Own TP to %s:%d to %d dBm", devices[handle].name, handle, devices[handle].tp.tx_range[default_tx_idx]);
                    devices[handle].tp.own_idx = default_tx_idx;
                    tlv_rssiChange[0] = 26;
                    tlv_rssiChange[1] = default_tx_idx;
                    send_tlv_from_p(handle, tlv_rssiChange, 2); // init rssi

                    if(devices[handle].id == 'P'){

                        if(IS_RELAY){// Tell peripherals to start advertising or not
                            //uint8_t tlv_phoneScanPeris[10] = {8};
                            //int count_phoneScanPeris = 1;
                            
                            //int x = 1;
                            //if(ble_conn_state_central_conn_count()>0){
                            //    while(true){
                            //        for(int i = 0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
                            //            if(devices[i].connected && 
                            //                devices[i].peri.type == x && !devices[i].peri.askedAdvPhone)
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
                            //                devices[i].peri.firstAskAdv = true;
                            //                if(count_phoneScanPeris == 2){
                            //                    break;
                            //                }
                            //            }
                            //        }
                            //        if(count_phoneScanPeris<2 && x == 1){
                            //            x = 2;
                            //        }else{
                            //            break;
                            //        }
                            //    }
                            //}

                            //send_usb_data(false, "Send PhoneScanPeris, count phone scan peris = %d, t1_REL=%d, t1_Sink", count_phoneScanPeris, relay_info.count_type1, relay_info.count_type1_sink);
                            //if(count_phoneScanPeris > 1){
                            //    send_tlv_from_p(handle, tlv_phoneScanPeris, count_phoneScanPeris);
                            //}else{
                            //    send_tlv_from_p(handle, tlv_phoneScanPeris, 1);
                            //}
                        }
                        if(!IS_RELAY){
                            ////devices[handle].cent.start_send_after_conn_update = false;
                            //uint8_t filed_conn_phone[2] = {72, 1};
                            //if(devices[RELAY_HANDLE].connected){
                            //    send_tlv_from_p(RELAY_HANDLE, filed_conn_phone, 2);
                            //}
                        }
                        //waiting_to_send_phone = true;
                    }
                        
                }
                else if(received_data[0] == 3 && false)//case 3:
                {// CENT SENT NAME
                    //else if (received_data[1] == 3 && received_len==3){// double check peris send 0,3 to phone
                    //if (received_len==3){
                        //ALLCONN: Add Cent id 
                        //send_tlv_usb(true, "Got peri name ", received_data, 5, 5, handle);
                        if(devices[handle].connected || (received_len != 5 && received_len != 3)){//devices[handle].id == received_data[1] && devices[handle].connected){
                            return;
                        }
                        //devices[handle].connected = true;
                    

                        //register_device_info(handle, received_data[1], received_data[2]==0);
                        set_other_device_sim_params(handle, received_data);
                        
                        //ble_gap_phys_t phys;
                        //phys.rx_phys = BLE_GAP_PHY_2MBPS; // Set RX to 2M PHY
                        //phys.tx_phys = BLE_GAP_PHY_2MBPS; // Set TX to 2M PHY

                        //err_code = sd_ble_gap_phy_update(handle, &phys);
                        //send_usb_data(false, "Requested 2MBPS PHY, err:%d", err_code);

                        sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_CONN, handle, tx_range[default_tx_idx]);
                        send_usb_data(true, "INIT Own TP to %s:%d to %d dBm", devices[handle].name, handle, devices[handle].tp.tx_range[default_tx_idx]);
                        devices[handle].tp.own_idx = default_tx_idx;
                        //change_conn_tx(tx_range[default_tx_idx], handle, (int8_t) devices[handle].tp.ema_rssi, (int8_t) devices[handle].tp.last_rssi);
                        tlv_rssiChange[0] = 26;
                        tlv_rssiChange[1] = default_tx_idx;
                        //tlv_rssiChange[2] = 0;
                        //tlv_rssiChange[3] = 0;
                        //if(i != devices[h].tp.requested_tx_idx && devices[h].tp.requested_tx_idx == -1){
                            //devices[h].tp.requested_tx_idx = i;
                            //if(h < NRF_SDH_BLE_CENTRAL_LINK_COUNT){
                                //send_tlv_from_c(h, tlv_rssiChange, 4, "Incr TX pow");
                            //}else{
                                send_tlv_from_p(handle, tlv_rssiChange, 2);
                        

                        if(send_test_rssi && handle == NRF_SDH_BLE_CENTRAL_LINK_COUNT){
                            //start_test_rssi();
                            return;
                        }
                        //devices[handle].peri.all_received_counter = 0;

                        // Cent start periodic and tell other SNs to do so too
                        if(devices[handle].id == 'P'){
                            devices[handle].conn_params = conn_params_conn_phone_wait;
                            uint8_t tlv_phoneReadyToStart[2] = {0,0};
                            if(IS_RELAY){
                                //uint8_t tlv_allNames[17] = {3, DEVICE_ID, SEND_TYPE, min_tx_idx_own, max_tx_idx_own};

                                send_tlv_from_p(handle, tlv_nameParams, 5);
                                

                                uint8_t start_periodic_tlv[1] = {2};
                                send_tlv_from_c(-1, start_periodic_tlv, 1, "Start periodic data");
                                //if(IS_SENSOR_NODE && !periodic_timer_started){
                                //    periodic_timer_start();
                                //    periodic_timer_started = true;
                                //}

                                //if(received_data[2] == 1){//P connected to gather data
                                    uint8_t tlv_phoneScanPeris[10] = {8};
                                    int count_phoneScanPeris = 1;
                                    
                                    //////for(int i = 0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
                                    //////    if(devices[i].connected && 
                                    //////        (devices[handle].tp.own_idx <= devices[i].tp.max_tx_idx ||

                                    //////        devices[handle].tp.ema_rssi> -72 ||
                                    //////        devices[handle].tp.last_rssi > -72)// TP check
                                    //////        && (devices[i].peri.type == 1 || devices[i].peri.type == 2)
                                    //////        && !devices[i].peri.askedAdvPhone) // allowed types for direct sending
                                    //////    {
                                    //////        relay_info.count_type1_sink++;
                                    //////    }
                                    //////}

                                    //////calculate_relay_params();
                                    //int x = 1;
                                    if(ble_conn_state_central_conn_count()>0){
                                        //while(true){
                                            for(int i = 0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
                                                if(devices[i].connected && 
                                                    devices[i].peri.type <= 2)
                                                    //devices[handle].tp.own_idx <= devices[i].tp.max_tx_idx ||
                                                    //devices[handle].tp.ema_rssi + tx_range[devices[handle].tp.own_idx] - tx_range[devices[i].tp.max_tx_idx] > -70 )//||
                                                    ////devices[handle].tp.ema_rssi> -72 ||
                                                    ////devices[handle].tp.last_rssi > -72)// TP check
                                                    //&& (devices[i].peri.type == 1 || devices[i].peri.type == 2)
                                                    //&& !devices[i].peri.askedAdvPhone)  // allowed types for direct sending
                                                {
                                                    //if(!devices[i].peri.isPeriodic){
                                                    send_tlv_from_c(i, tlv_advertiseSink, 4, "Type 1 start advertising");
                                                    send_usb_data(false, "%s Start adv for phone", devices[i].name);
                                                    //count_onreq_not_phone--;
                                                    tlv_phoneScanPeris[count_phoneScanPeris] = devices[i].id;
                                                    count_phoneScanPeris++;
                                                    //relay_info.count_type1_sink++;
                                                    send_usb_data(false, "Sent adv for Phone to %s, count phone scan peris = %d", devices[i].name, relay_info.count_type1_sink);
                                                    devices[i].peri.chosenRelay = false;
                                                    devices[i].peri.askedAdvPhone = true;
                                                    devices[i].peri.firstAskAdv = true;
                                                    //if(count_phoneScanPeris == 2){
                                                    //    break;
                                                    //}
                                                }
                                            }
                                            //if(count_phoneScanPeris<2 && x == 1){
                                            //    x = 2;
                                            //}else{
                                            //    break;
                                            //}
                                        //}
                                    }
                                    ////send_usb_data(false, "Send PhoneScanPeris, count phone scan peris = %d, t1_REL=%d, t1_Sink", count_phoneScanPeris, relay_info.count_type1, relay_info.count_type1_sink);
                                    if(count_phoneScanPeris > 1){
                                    //update_conn_params(handle, phone_scan_for_peris_conn_params);
                                        send_tlv_from_p(handle, tlv_phoneScanPeris, count_phoneScanPeris);
                                        devices[handle].conn_params = conn_params_conn_phone_wait;
                                    }
                                    ////}else{
                                    ////    send_tlv_from_p(handle, tlv_phoneScanPeris, 1);
                                    ////    //devices[handle].conn_params = conn_params_phone;
                                    ////    //update_conn_params(handle, conn_params_phone);
                                    ////    //devices[handle].cent.start_send_after_conn_update = true;
                                    ////}
                                    //send_tlv_from_p(handle, tlv_phoneReadyToStart, 1);
                                    //if(devices[handle].conn_params.min_conn_interval != devices[handle].curr_interval){
                                        send_usb_data(true, "Current connection interval for %s:%d: INT %d ms, latency=%d, HAVE TO CHANGE to INT: %d ms", 
                                            devices[handle].name, handle,
                                            devices[handle].curr_interval, devices[handle].curr_sl,
                                            conn_params_conn_phone_wait.min_conn_interval);

                                        devices[handle].conn_params = conn_params_conn_phone_wait;
                                        //update_conn_params(handle, devices[handle].conn_params);
                                        //devices[handle].cent.start_send_after_conn_update = true;
                                    //}

                                //}
                                //update_conn_params(handle, conn_params_periodic);
                                //devices[handle].conn_params = conn_params_phone;//conn_params_on_gather_sink;
                                
                                //update_conn_params(handle, conn_params_on_gather_sink);

                                //devices[handle].cent.start_send_after_conn_update = true;
                                //update_all_conn_for_gather_t23();
                            }
                            else{
                                //devices[handle].conn_params = conn_params_phone;//conn_params_on_gather_sink;
                                //update_conn_params(handle, conn_params_phone);
                                ////uint8_t tlv_phoneReadyToStart[2] = {0,0};
                                ////send_tlv_from_p(handle, tlv_phoneReadyToStart, 2);
                                //devices[handle].cent.start_send_after_conn_update = true;
                                send_usb_data(false, "Relay handle is %d, conn?=", RELAY_HANDLE, devices[RELAY_HANDLE].connected);
                                uint8_t filed_conn_phone[2] = {72, 1};
                                if(devices[RELAY_HANDLE].connected){
                                    send_tlv_from_p(RELAY_HANDLE, filed_conn_phone, 2);
                                }
                                //waiting_to_send_phone = true;
                                //if(devices[handle].conn_params.min_conn_interval != devices[handle].curr_interval){
                                    send_usb_data(true, "Current connection interval for %s:%d: INT %d ms, latency=%d, HAVE TO CHANGE to INT: %d ms", 
                                        devices[handle].name, handle,
                                        devices[handle].curr_interval, devices[handle].curr_sl,
                                        conn_params_conn_phone_wait.min_conn_interval);

                                    devices[handle].conn_params = conn_params_conn_phone_wait;
                                    //update_conn_params(handle, devices[handle].conn_params);
                                    //devices[handle].cent.start_send_after_conn_update = true;
                                //}
                                //send_tlv_from_p(handle, tlv_phoneReadyToStart, 2);
                            }

                            stop_adv();
                            if(IS_RELAY){
                                scan_stop();
                            }
                        }

                    //break;
                }
                else if(received_data[0] == 4)//case 4: // was 10
                {// PAUSE SENDING 
                    //else if(received_data[1] == 0){// 0 == pause send data
                        if(SEND_TYPE && devices[handle].cent.isNotPaused){
                            package_tail = (package_tail+255)%package_storage_size;
                            package_count++;
                        }else{
                            //last_prefix = received_data[2];
                            devices[handle].cent.stoppedBatch = received_data[1];
                            devices[handle].cent.stoppedPrefix = received_data[2];
                            pause_chunks = true;
                            send_usb_data(false, "%s is paused.", devices[handle].name);
                            uint8_t tlv[1] = {0}; //was 0,0
                            send_tlv_from_p(handle, tlv, 1);
                        }
                        devices[handle].cent.isNotPaused = false;
                    //}

                    //break;
                }
                //else if(received_data[0] == 5)//case 5: // was 11
                //{ // RESUME SENDING
                //    //else if (received_data[1] == 1){//1 == Resume send data
                //        ////if(sending_tlv_spec){
                //        ////    pause_chunks = false;
                //        ////    send_specific_tlv();//(m_nus, p_evt->conn_handle);
                //        ////    send_usb_data(false, "Got Start again Spec");
                //        ////}else 
                //        // TODO: DOnt know whats happening, maybe use only else part
                //        if(devices[handle].cent.stoppedBatch == 0){//received_len == 2){//normal resume
                //            if(sending_periodic_tlv){
                //                devices[handle].cent.isNotPaused = true;
                //                if(CHOSEN_HANDLE == handle){
                //                    send_periodic_tlv();//(m_nus, CHOSEN_HANDLE);
                //                    send_usb_data(false, "%s is resumed. CHOSEN:%d", devices[handle].name, CHOSEN_HANDLE == handle);
                //                }
                        
                //            }else{
                //                pause_chunks = false;
                //                package_tail = last_prefix+1;
                //                CHOSEN_HANDLE = handle;
                //                send_stored_tlv();//(m_nus, p_evt->conn_handle);
                //                send_usb_data(false, "Got Start again Chunks");
                //            }
                //        }else{//disconnected while sending on req
                //            //TODO: Check where it starts sending
                //            CHOSEN_HANDLE = handle;
                //            pause_chunks = false;
                //            sending_tlv_chunks = true;
                //            // NEW
                //            if(large_counter == devices[handle].cent.stoppedBatch){
                //                send_usb_data(false, "Got restart sending SAME BATCH, old count=%d, new count=%d; old tail=%d, new tail=%d",
                //                                package_count, package_count + (package_tail - devices[handle].cent.stoppedPrefix), package_tail, devices[handle].cent.stoppedPrefix);
                //                package_count = package_count + (package_tail - devices[handle].cent.stoppedPrefix );
                //                package_tail = devices[handle].cent.stoppedPrefix;
                //            }else if (large_counter < devices[handle].cent.stoppedBatch){
                //                send_usb_data(false, "Got restart sending on BIGGER BATCH, old count=%d, old tail=%d; recBatch=%d, recPref=%d, oldBatch=%d, oldPref=%d",
                //                                package_count, package_tail, devices[handle].cent.stoppedBatch, devices[handle].cent.stoppedPrefix, large_counter, package_tail);
                //                send_usb_data(false, "Get new batch usb and sending data to handle:%d", p_evt->conn_handle);
                //                large_counter = devices[handle].cent.stoppedBatch-1;
                //                request_usb_data();
                //            }else{
                //                send_usb_data(false, "Got restart sending on SMALLER BATCH, old count=%d, old tail=%d; recBatch=%d, recPref=%d, oldBatch=%d, oldPref=%d",
                //                                package_count, package_tail, devices[handle].cent.stoppedBatch, devices[handle].cent.stoppedPrefix, large_counter, package_tail);
                //                send_chunk_tlv();
                //            }

                //            // OLD
                //            //if(large_counter == received_data[2]){
                //            //    send_usb_data(false, "Got restart sending SAME BATCH, old count=%d, new count=%d; old tail=%d, new tail=%d",
                //            //                    package_count, package_count + (package_tail - received_data[3]), package_tail, received_data[3]);
                //            //    package_count = package_count + (package_tail - received_data[3]);
                //            //    package_tail = received_data[3];
                //            //}else if (large_counter < received_data[2]){
                //            //    send_usb_data(false, "Got restart sending on BIGGER BATCH, old count=%d, old tail=%d; recBatch=%d, recPref=%d, oldBatch=%d, oldPref=%d",
                //            //                    package_count, package_tail, received_data[2], received_data[3], large_counter, package_tail);
                //            //    send_usb_data(false, "Get new batch usb and sending data to handle:%d", p_evt->conn_handle);
                //            //    large_counter = received_data[2]-1;
                //            //    request_usb_data();
                //            //}else{
                //            //    send_usb_data(false, "Got restart sending on SMALLER BATCH, old count=%d, old tail=%d; recBatch=%d, recPref=%d, oldBatch=%d, oldPref=%d",
                //            //                    package_count, package_tail, received_data[2], received_data[3], large_counter, package_tail);
                //            //    send_chunk_tlv();
                //            //}
                //        }
                //    //}

                //    //break;
                //}
                else if(received_data[0] == 66)//case 6:// was 17
                { // MUST CHANGE TX 
                    //else if(received_data[1] == 7){//Must incr rssi from Cent 1,7
                        int8_t rec_tx = (int8_t) received_data[1];
                        if(devices[handle].tp.own_idx != rec_tx){
                            send_usb_data(true, "Got TIMEOUT Request own TP to %s:%d to %d dBm from %d dBm; ema=%d, rssiLast=%d", devices[handle].name, handle, devices[handle].tp.tx_range[rec_tx], tx_range[devices[handle].tp.own_idx], (int8_t) received_data[2], (int8_t) received_data[3]);
                            change_conn_tx(devices[handle].tp.tx_range[rec_tx], handle, (int8_t) devices[handle].tp.ema_rssi, (int8_t) devices[handle].tp.last_rssi);
                            
                        }
                    //}
                    //break;
                } 
                else if(received_data[0] == 6)//case 6:// was 17
                { // MUST CHANGE TX 
                    //else if(received_data[1] == 7){//Must incr rssi from Cent 1,7
                        int8_t rec_tx = (int8_t) received_data[1];
                        if(rec_tx == devices[handle].tp.requested_tx_idx){
                            devices[handle].tp.requested_tx_idx = 0;
                        }
                        if(devices[handle].tp.own_idx != rec_tx){
                            send_usb_data(true, "Got Request own TP to %s:%d to %d dBm from %d dBm; ema=%d, rssiLast=%d", devices[handle].name, handle, devices[handle].tp.tx_range[rec_tx], tx_range[devices[handle].tp.own_idx], (int8_t) received_data[2], (int8_t) received_data[3]);
                            change_conn_tx(devices[handle].tp.tx_range[rec_tx], handle, (int8_t) devices[handle].tp.ema_rssi, (int8_t) devices[handle].tp.last_rssi);
                            
                        }
                    //}
                    //break;
                } 
                else if(received_data[0] == 7)//case 7: // was 16
                {// OTHER CHANGED TX
                   
                        if (received_data[1] != devices[handle].tp.tx_idx){
                            if(received_data[1] > devices[handle].tp.tx_idx){
                                //send_usb_data(true, "%s:%d Increased TP to %d has ema=%d, rssiLast=%d",devices[handle].name, handle, devices[handle].tp.tx_range[received_data[1]], (int8_t) received_data[2], (int8_t) received_data[3]);
                                send_usb_data(true, "%s:%d Increased TP to %d has ema=%d, rssiLast=%d",devices[handle].name, handle, devices[handle].tp.tx_range[received_data[1]], (int8_t) devices[handle].tp.ema_rssi, (int8_t) devices[handle].tp.last_rssi);
                                ////send_usb_data(true, "Here to %s, emaOld=%d, emaNew=%d",devices[handle].name, devices[handle].tp.ema_rssi, rssi);
                                ////devices[handle].tp.ema_rssi = devices[handle].tp.ema_rssi + abs(devices[handle].tp.tx_range[received_data[1]] - devices[handle].tp.tx_range[devices[handle].tp.tx_idx]);
                                ////devices[handle].tp.ema_rssi = (float) rssi * 1.0;
                                ////if(devices[handle].tp.received_counter >= 3){
                                ////    devices[handle].tp.received_counter -= 3;
                                ////}
                                if(WAIT_FOR_TP_CONFIRMATION){
                                    devices[handle].tp.received_counter = 0; //devices[handle].tp.received_counter/2;
                                }

                            }
                            else if(received_data[1] < devices[handle].tp.tx_idx){
                                //send_usb_data(true, "%s:%d Decreased TP to %d has ema=%d, rssiLast=%d",devices[handle].name, handle, devices[handle].tp.tx_range[received_data[1]], (int8_t) received_data[2], (int8_t) received_data[3]);
                                send_usb_data(true, "%s:%d Decreased TP to %d has ema=%d, rssiLast=%d",devices[handle].name, handle, devices[handle].tp.tx_range[received_data[1]], (int8_t) devices[handle].tp.ema_rssi, (int8_t) devices[handle].tp.last_rssi);
                                ////send_usb_data(true, "Here to %s, emaOld=%d, emaNew=%d",devices[handle].name, devices[handle].tp.ema_rssi, rssi);
                                ////devices[handle].tp.ema_rssi = devices[handle].tp.ema_rssi - abs(devices[handle].tp.tx_range[received_data[1]] - devices[handle].tp.tx_range[devices[handle].tp.tx_idx]);
                                ////devices[handle].tp.ema_rssi = (float) rssi * 1.0;
                                ////if(devices[handle].tp.received_counter >= 3){
                                ////    devices[handle].tp.received_counter -= 3;
                                ////}
                                if(WAIT_FOR_TP_CONFIRMATION){
                                   devices[handle].tp.received_counter = 0; //devices[handle].tp.received_counter/2;
                                }
                            }
                            if(WAIT_FOR_TP_CONFIRMATION){
                                devices[handle].tp.tx_idx = received_data[1];
                                devices[handle].tp.requested_tx_idx = -1;
                            }
                            if(PERIS_DO_TP){
                                devices[handle].tp.other_tx_idx = received_data[1];
                            }
                        }
                    //}
                    //break;
                }
                else if(received_data[0] == 10 && received_len>=2){// new missing check
                    send_usb_data(true, "Got missing from phone for %c, len=%d", received_data[1], received_len);
                    
                    if((char) received_data[1] != DEVICE_ID){
                        send_usb_data(true, "Missing to be relayed to %c", (char) received_data[1]);
                        for(int j=0;j<NRF_SDH_BLE_CENTRAL_LINK_COUNT;j++){
                            if(received_data[1] == devices[j].id){
                                send_tlv_from_c(j, received_data, received_len, "Relayed missing from phone");
                                if(devices[handle].tp.use_pdr && received_len > 2){
                                    devices[handle].tp.use_pdr = false;
                                    devices[handle].tp.stopped_pdr = true;
                                }

                                if(devices[handle].tp.stopped_pdr && received_len == 2){
                                    devices[handle].tp.stopped_pdr = false;
                                    devices[handle].tp.use_pdr = true;
                                }
                                return;
                            }
                        }
                    }else{
                        //int l = 0;
                        //int ls = 0;
                        //while(l>0){
                        //    if(received_len - ls > 100){
                        //        l = 100;
                        //    }else{
                        //        l = received_len -ls;
                        //    }
                        //    send_tlv_usb(true, "Miss1:", received_data, l, l, PHONE_HANDLE);
                        //}
                        if(received_len == 2 && !sending_own_periodic){
                                sending_missed_tlv = false;
                                finished_missing = false;
                            //if(sending_stored_tlv){
                                if(!finished_stored){
                                    send_stored_tlv();
                                    send_usb_data(true, "finished with stored missing, continue stored");
                                }else{
                                    send_usb_data(true, "finished with stored missing, start periodic");
                                    sending_stored_tlv = false;
                                    //finished_stored = false;
                                    if((IS_RELAY || DEVICE_ID == 'C') && !sending_own_periodic){
                                        first_packet_timer_start();
                                    }
                                    sending_periodic_tlv = true;
                                    sending_own_periodic = true;
                                    
                                    //if(!periodic_timer_started){
                                        periodic_timer_start();
                                        periodic_timer_started = true;
                                    //}
                                    if(DEVICE_ID == 'M' || DEVICE_ID == 'C' || !devices[RELAY_HANDLE].connected || IS_RELAY){
                                        bool all_stored_finished = false;  
                                        if(IS_RELAY){
                                            all_stored_finished = true;
                                            for(int i = 0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
                                            // check if there are any peris askAdvPhone not done yet
                                                if(!(devices[i].peri.sending_periodic || devices[i].peri.sending_to_phone))
                                                {
                                                    all_stored_finished = false;
                                                    break;
                                                }
                                            }
                                            if(all_stored_finished){
                                                uint8_t tlv_p[3] = {12, 12, devices[handle].id};
                                                tlv_p[2] = DEVICE_ID;
                                                send_tlv_from_p(PHONE_HANDLE, tlv_p, 3);
                                            }
                                        }
                                        if(!all_stored_finished){
                                            send_tlv_from_p(handle, starting_periodic_tlv,1);
                                            //update_conn_params(handle, conn_params_periodic);
                                            send_usb_data(true, "Start sending periodically to Phone");
                                        }
                                    }else if(handle == PHONE_HANDLE && devices[RELAY_HANDLE].connected){
                                        send_tlv_from_p(handle, starting_periodic_tlv,2);
                                        send_tlv_from_p(RELAY_HANDLE, starting_periodic_tlv,1);
                                        CHOSEN_HANDLE = RELAY_HANDLE;
                                        send_usb_data(true, "Disconnect Phone, start sending periodically to relay.");
                                    }else{
                                        send_tlv_from_p(RELAY_HANDLE, starting_periodic_tlv,1);
                                    }
                                    //update_conn_params(handle, conn_params_periodic);
                                    //pref_counter ++;
                                    //if(pref_counter == 0){
                                    //    batch_counter++;
                                    //}
                                    if(pref_counter == 255){
                                        pref_counter = 0;
                                        batch_counter++;
                                    }else{
                                        pref_counter ++;
                                    }
                                    add_periodic_bytes(0);
                                    send_periodic_tlv();
                            
                                    bsp_board_led_on(led_BLUE);
                                }
                            //}
                            //}
                        }else if(!devices[CHOSEN_HANDLE].peri.miss_rec && received_len > 2 && !sending_own_periodic){
                            if(!sending_missed_tlv || (sending_missed_tlv && finished_missing)){
                                //for(int j=0; j < received_len - 2; j++){
                                    memcpy(devices[CHOSEN_HANDLE].peri.missed, received_data, received_len-2);
                                //}
                                devices[CHOSEN_HANDLE].peri.miss_rec = true;
                                if(received_data[received_len-1] != 255 &&  received_data[received_len-2] != 255){
                                    //int resend_bytes = 0;
                                    //if(batch_counter > received_data[received_len-2]){
                                    //    resend_bytes = 241 * (255 - received_data[received_len-1] + pref_counter);
                                    //}else{
                                    //    resend_bytes = 241 * (pref_counter - received_data[received_len-1]);
                                    //}
                                    //stored_bytes_sent += resend_bytes;
                                    batch_counter = received_data[received_len-2];
                                    pref_counter = received_data[received_len-1];
                                    dummy_data[0] = batch_counter;
                                    dummy_data[1] = pref_counter;
                                }else{
                                        pref_counter = (pref_counter+1)%255;
                                        if(pref_counter == 0){
                                            batch_counter++;
                                        }
                                }

                                devices[CHOSEN_HANDLE].peri.miss_idx = 4;
                                devices[CHOSEN_HANDLE].peri.final_miss_idx = received_len - 3;
                                devices[CHOSEN_HANDLE].peri.miss_b_idx = 2;

                                sending_missed_tlv = true;
                                
                                finished_missing = false;
                                send_missing_tlv();
                            }
                            
                        }
                    }
                }
                //else if(received_data[0] == 8 && !send_test_rssi)//case 8:// was 12
                //{// MISSING CHECK FROM PHONE
                //    //if(received_data[1] == 2){
                //        //TODO: get successfully sent chunkIDs from phone
                //        //TODO: in phone when sending missed, seperate different large counters with [255,255]
                    
                //        //for (int a = 3; a < received_data[2]; a+=3) {
                //        //    size_t idx = circ_buf.tail; //oldest
                //        //    // Check if the chunk ID is in the received_ids array
                //        //    for (size_t i = 0; i < 20; i++) {
                //        //        if ((received_data[a] == circ_buf.buffer[idx]) 
                //        //          && (received_data[a+1] == circ_buf.buffer[idx+1]) 
                //        //          && (received_data[a+2] == circ_buf.buffer[idx+2])) 
                //        //        {
                //        //            // Mark this chunk as invalid (e.g., by setting chunk_id to 0)
                //        //            circ_buf.buffer[idx] = 255;
                //        //            circ_buf.size--;
                //        //            break;
                //        //        }
                //        //        // Move to the next chunk in the circular buffer
                //        //        idx = (idx + 245) % MAX_FILE_SIZE;
                //        //    }
                //        //}
                      
                //      //send_usb_data(false, "Sent ALL STORED DATA, stored bytes = %d, sent = ", stored_bytes, stored_bytes_sent);
                      
                //      //if(IS_RELAY){
                //      //    send_tlv_from_c(-3, tlv_reqData, 2, "Sent start Sending req data");
                //      //    sending_periodic_tlv = true;
                //      //    sending_own_periodic = false;
                //      //    if(IS_SENSOR_NODE){
                //      //        send_tlv_from_c(-3, tlv_reqData, 2, "Sent start Sending req data");
                //      //        sending_periodic_tlv = true;
                //      //        if(!periodic_timer_started){
                //      //            periodic_timer_start();
                //      //            periodic_timer_started = true;
                //      //        }
                //      //        sending_own_periodic = true;
                //      //        //send_periodic_tlv();
                //      //    }
                //      //}
                      

                //        if(received_data[1] == (uint8_t) DEVICE_ID){//(ble_conn_state_central_conn_count()==0)){
                //            if(sentAllOwnStoredData){
                //                return;
                //                //int batch = 0;
                //                //int l = 0;
                //                //while(int i=2; i<received_len; i++){
                                    
                //                //}
                //            }
                //            if(received_len == 2 || true){//TODO:MISSSSSS
                //                //uint8_t temp[13] = {0,0,255,255,0,0,0,0,0,0,0,0,0};
                //                send_usb_data(true, "ALL STORED DATA sent correctly!!!");
                                
                //                memset(tlv_chooseCentResponse, 0, sizeof(tlv_chooseCentResponse));
                //                //send_usb_data(false, "count onreq not phone = %d", count_onreq_not_phone);
                //                //send_usb_data(false, "Disconnecting from Cent Handle:%d", p_evt->conn_handle);
                //                sentAllOwnStoredData = true;
                //                //if(devices[handle].id == 'P'){
                //                    if(!IS_RELAY){//DEVICE_ID != 'C' && DEVICE_ID != 'D'){
                //                        start_send_data_not_received = true;
                //                        //is_sending_to_phone = false;
                //                        //disconnect_handle(PHONE_HANDLE);
                //                        sending_stored_tlv = false;
                //                        sending_periodic_tlv = true;
                //                        sending_own_periodic = true;
                //                        if(!periodic_timer_started){
                //                            periodic_timer_start();
                //                            periodic_timer_started = true;
                //                        }
                //                        if(DEVICE_ID == 'M' || DEVICE_ID == 'C' || !devices[RELAY_HANDLE].connected){
                //                            send_tlv_from_p(handle, starting_periodic_tlv,1);
                //                            update_conn_params(handle, conn_params_periodic);
                //                            send_usb_data(true, "Start sending periodically to Phone");
                //                        }else if(handle == PHONE_HANDLE && devices[RELAY_HANDLE].connected){
                //                            send_tlv_from_p(handle, starting_periodic_tlv,2);
                //                            send_tlv_from_p(RELAY_HANDLE, starting_periodic_tlv,1);
                //                            CHOSEN_HANDLE = RELAY_HANDLE;
                //                            send_usb_data(true, "Disconnect Phone, start sending periodically to relay.");
                //                        }
                //                        //update_conn_params(handle, conn_params_periodic);
                //                        pref_counter ++;
                //                        if(pref_counter == 0){
                //                            batch_counter++;
                //                        }
                //                        add_periodic_bytes(0);
                //                        send_periodic_tlv();
                //                        //send_usb_data(true, "Start sending periodically");
                //                        //ble_gap_conn_params_t slow_periodic = conn_params_periodic;
                //                        //slow_periodic.slave_latency = slow_periodic.slave_latency / 2;
                                        
                //                        bsp_board_led_on(led_BLUE);

                //                        //change_gap_params(false);
                //                        //uint8_t tlv_discPhone[2] = {3, 1};
                //                        //for(int i=NRF_SDH_BLE_CENTRAL_LINK_COUNT; i<NRF_SDH_BLE_TOTAL_LINK_COUNT; i++){
                //                        //    if(IS_RELAY){
                //                        //        send_tlv_from_p(i, tlv_discPhone, 2);
                //                        //    }
                //                        //}
                //                    }else{// IS RELAY
                //                        sending_stored_tlv = false;
                //                        sending_own_periodic = true;
                //                        sending_periodic_tlv = true;
                //                        if(!periodic_timer_started){
                //                            periodic_timer_start();
                //                            periodic_timer_started = true;
                //                        }
                //                        pref_counter ++;
                //                        if(pref_counter == 0){
                //                            batch_counter++;
                //                        }
                //                        add_periodic_bytes(0);
                                        
                //                        send_tlv_from_p(handle, starting_periodic_tlv,1);
                //                        send_periodic_tlv();
                //                        send_usb_data(true, "Start sending own data periodically"); 
                //                        bsp_board_led_on(led_BLUE);
                //                    }
                //                    //else if(count_onreq_not_phone == 0){
                //                    //    update_conn_params(handle, phone_default_conn_params);
                //                    //}
                //                    //update_conn_params(handle, on_request_power_saving_conn_params);
                //                //}
                //                //adv_start(false);

                //            }else{
                //                //TODO: missing
                //                send_tlv_usb(false, "Got missing from phone for me ", received_data, received_len, received_len,handle);
                //            }
                //        }else if(IS_RELAY){
                //            //OPTIONAL: NEW
                //            //send_tlv_usb(false, "Got missing from phone for Relayed ", received_data, received_len, received_len, handle);
                //            bool everythingSent = true;
                //            int kot = -1;
                //            for(int j=0;j<NRF_SDH_BLE_CENTRAL_LINK_COUNT;j++){
                //                if(received_data[1] == devices[j].id){
                                    
                //                    if(devices[j].peri.finished != true){
                //                        kot = j;
                //                        //send_tlv_from_c(j, received_data, received_len, "Send missing from phone norm");
                //                        //send_tlv_from_c(-3, received_data, received_len, "Send missing from phone with -3");
                //                        send_tlv_from_c(kot, received_data, received_len, "Relay missing from phone");
                                        
                //                        if(received_len == 2){// No missing
                //                            //reset_device_peri(j);
                //                            //peri_relay_info[j].allDataSentFromPeri = false;
                //                            //peri_relay_info[j].last = -1;

                //                            //peri_relay_info[j].recPhoneMissData = false;
                //                            //peri_relay_info[j].no_space = -1;
                //                            //peri_relay_info[j].received_all_count = 0;
                //                            //peri_relay_info[j].last_missing_rec = -1;
                //                            //peri_relay_info[j].sent_missed_msg_cnt = 0;
                //                            //peri_relay_info[j].received_last_msg_cnt = 0;
                //                            //peri_relay_info[j].all_received_counter = 0;
                //                            //peri_relay_info[j].stopped = false;
                //                            //peri_relay_info[j].missed[0] = 2; // idx of last missed
                //                            //peri_relay_info[j].missed[1] = 0; // chunkID of last received, missed left to be rec. after
                //                            //peri_relay_info[j].waiting_retransmit = false;
                //                            if(devices[j].id != 'M'){
                //                                devices[j].peri.finished = true;
                //                                set_other_device_op_mode(j,2);
                //                                update_conn_params(j, devices[j].conn_params);
                //                                devices[j].peri.chosenRelay = true;
                //                            }
                //                            //break;
                //                            //return;
                //                        }else{
                //                            send_tlv_usb(true, "Missing from phone: ", received_data, 1,1, j);
                //                            if(devices[j].id != 'M'){
                //                                devices[j].peri.finished = true;
                //                                set_other_device_op_mode(j,2);
                //                                update_conn_params(j, devices[j].conn_params);
                //                            }
                //                        }
                //                    }
                //                }
                //                if(devices[j].peri.finished != true && !devices[j].peri.askedAdvPhone){
                //                    everythingSent = false;
                //                }
                //           }   
                //           if(everythingSent && sentAllOwnStoredData){
                //               //data_relay_in_process = false;
                //               start_send_data_not_received = true;
                //               //more_data_left = true;
                //               peri_relay_count = 0;
                //               send_usb_data(true, "ALL STORED DATA HAS BEEN RELAYED!");
                //               update_conn_params(handle, conn_params_periodic);
                //               //scan_start();

                //               //data_gather_relay_on = false;

                //               //is_sending_to_phone = false;
                //               //scan_on_interval_start();
                //               //adv_start(false);
                //           }
                //        }
                //    //}

                //    //break;
                //}
                else if(received_data[0] == 9 && !send_test_rssi && CONN_PHONE_TIMERS)//case 9:
                {// cent sent start adv for phone
                    //TODO
                    if(!IS_RELAY){ // NOT RELAY
                        change_gap_params(true);
                        send_usb_data(false, "Starting timed %d advertising to connect to Phone", 5000);
                        if(devices[handle].peri.type < 2){
                            adv_timer_start(15000);
                        }else{
                            adv_timer_start(15000);
                        }

                        if(received_len == 2 && received_data[1] == 255){
                            stop_adv();
                        }
                        //else{
                        //    if(received_len == 4 && !got_adv_phone){
                        //        got_adv_phone = true;
                        //        //count_onreq = received_data[1];// idk what they do
                        //        //count_onreq_not_phone = received_data[1];// idk what they do

                        //        //calculate_t1_sink_params(received_data[1], received_data[2], received_data[3]);
                        //        send_usb_data(false, "Starting advertising to connect to Phone");
                        //        change_gap_params(true);
                        //        adv_start(false);
                        //    }else if (got_adv_phone){// idk what they do 
                        //        ble_gap_conn_params_t old_sink_params = {
                        //            .min_conn_interval = conn_params_on_gather_sink.min_conn_interval,
                        //            .max_conn_interval = conn_params_on_gather_sink.max_conn_interval,
                        //            .slave_latency = conn_params_on_gather_sink.slave_latency,
                        //            .conn_sup_timeout = conn_params_on_gather_sink.conn_sup_timeout,
                        //        };
                        //        //calculate_t1_sink_params(received_data[1], received_data[2], received_data[3]);
                        //        if(connectedToPhone && old_sink_params.min_conn_interval != conn_params_on_gather_sink.min_conn_interval){
                        //            send_usb_data(false, "Got new gather params from relay");
                        //            update_conn_params(PHONE_HANDLE, conn_params_on_gather_sink);
                        //        }
                        //    }
                        //}
                    }else{//RELAY
                        //if(received_len > 1){
                        //    bool isNotScanned = true;
                        //    uint8_t tlv_relayedPeriName[4] = {3};
                        //    for(int j=0; j<NRF_SDH_BLE_CENTRAL_LINK_COUNT; j++){
                        //        isNotScanned = true;
                        //        if(devices[j].peri.askedAdvPhone && !devices[j].peri.chosenRelay){
                        //            for(int i=1; i<received_len;i++){
                        //                if(devices[j].id == received_data[i]){
                        //                    isNotScanned = false;
                        //                    break;
                        //                }
                        //            }
                        //            if(isNotScanned){
                        //                devices[j].peri.askedAdvPhone = false;
                        //                devices[j].peri.chosenRelay = true;
                                        
                        //                count_onreq_not_phone++;
                        //                relay_info.count_type1_sink--;
                        //                send_usb_data(false, "Phone did not scan %s, count onreq not phone = %d", devices[j].name, count_onreq_not_phone);
                        //                uint8_t tlv_stopAdv[2] = {9,255};
                        //                send_tlv_from_c(j, tlv_stopAdv, 2, "Stop advertising for Phone");
                        //                //tlv_relayedPeriName[1] = devices[j].id;
                        //                //tlv_relayedPeriName[2] = devices[j].peri.type;
                        //                //tlv_relayedPeriName[3] = devices[j].tp.tx_idx;
                        //                //send_tlv_from_p(handle, tlv_relayedPeriName, 3);
                        //            }else{
                        //                set_other_device_op_mode(j, 3);
                        //            }
                        //        }else if(!devices[j].peri.askedAdvPhone){
                        //            //tlv_relayedPeriName[1] = devices[j].id;
                        //            //tlv_relayedPeriName[2] = devices[j].peri.type;
                        //            //tlv_relayedPeriName[3] = devices[j].tp.tx_idx;
                        //            //send_tlv_from_p(handle, tlv_relayedPeriName, 4);
                        //        }
                        //    }
                        //    //if(received_len-1 == count_onreq && count_onreq_not_phone != 0){
                        //    //    send_usb_data(false, "FIX IN [9,peris], count onreq not phone = %d", count_onreq_not_phone);
                        //    //    count_onreq_not_phone = 0;

                        //    //}
                        //}
                        //else{
                        ////    //calculate_on_request_params(count_onreq);
                        ////    update_conn_params(handle, on_request_conn_params);
                        //    send_usb_data(false, "Phone connected to all direct sending devices, start conn param update");
                            
                        //}
                        //devices[handle].conn_params = conn_params_phone;
                        //update_conn_params(handle, conn_params_phone);
                        //devices[handle].cent.start_send_after_conn_update = true;
                        ////ble_gap_conn_params_t old_t1_sink_params = {
                        ////    .min_conn_interval = conn_params_on_gather_sink.min_conn_interval,
                        ////    .max_conn_interval = conn_params_on_gather_sink.max_conn_interval,
                        ////    .slave_latency = conn_params_on_gather_sink.slave_latency,
                        ////    .conn_sup_timeout = conn_params_on_gather_sink.conn_sup_timeout,
                        ////    };
                        
                        ////if(!devices[handle].cent.start_send_after_conn_update){
                        ////    //calculate_relay_params();
                        ////    if(connectedToPhone && old_t1_sink_params.min_conn_interval != conn_params_on_gather_type1_sink.min_conn_interval){
                        ////        send_tlv_from_p(handle, tlv_advertiseSink, 4);
                        ////    }
                        ////    devices[handle].conn_params = conn_params_phone
                        ////    update_conn_params(handle, conn_params_phone);
                        ////    //devices[handle].cent.start_send_after_conn_update = true;
                        ////}
                    }
                    //break;
                }
                //else if(received_data[0] == 9 && !send_test_rssi && !CONN_PHONE_TIMERS)//case 9: OLDDD
                //{// cent sent start adv for phone
                //    //TODO
                //    if(!IS_RELAY){
                //        if(received_len == 2 && received_data[1] == 255){
                //            stop_adv();
                //        }else{
                //            if(received_len == 4 && !got_adv_phone){
                //                got_adv_phone = true;
                //                //count_onreq = received_data[1];// idk what they do
                //                //count_onreq_not_phone = received_data[1];// idk what they do

                //                //calculate_t1_sink_params(received_data[1], received_data[2], received_data[3]);
                //                send_usb_data(false, "Starting advertising to connect to Phone");
                //                change_gap_params(true);
                //                adv_start(false);
                //            }else if (got_adv_phone){// idk what they do 
                //                ble_gap_conn_params_t old_sink_params = {
                //                    .min_conn_interval = conn_params_on_gather_sink.min_conn_interval,
                //                    .max_conn_interval = conn_params_on_gather_sink.max_conn_interval,
                //                    .slave_latency = conn_params_on_gather_sink.slave_latency,
                //                    .conn_sup_timeout = conn_params_on_gather_sink.conn_sup_timeout,
                //                };
                //                //calculate_t1_sink_params(received_data[1], received_data[2], received_data[3]);
                //                if(connectedToPhone && old_sink_params.min_conn_interval != conn_params_on_gather_sink.min_conn_interval){
                //                    send_usb_data(false, "Got new gather params from relay");
                //                    update_conn_params(PHONE_HANDLE, conn_params_on_gather_sink);
                //                }
                //            }
                //        }
                //    }else{
                //        if(received_len > 1){
                //            bool isNotScanned = true;
                //            uint8_t tlv_relayedPeriName[4] = {3};
                //            for(int j=0; j<NRF_SDH_BLE_CENTRAL_LINK_COUNT; j++){
                //                isNotScanned = true;
                //                if(devices[j].peri.askedAdvPhone && !devices[j].peri.chosenRelay){
                //                    for(int i=1; i<received_len;i++){
                //                        if(devices[j].id == received_data[i]){
                //                            isNotScanned = false;
                //                            break;
                //                        }
                //                    }
                //                    if(isNotScanned){
                //                        devices[j].peri.askedAdvPhone = false;
                //                        devices[j].peri.chosenRelay = true;
                                        
                //                        count_onreq_not_phone++;
                //                        relay_info.count_type1_sink--;
                //                        send_usb_data(false, "Phone did not scan %s, count onreq not phone = %d", devices[j].name, count_onreq_not_phone);
                //                        uint8_t tlv_stopAdv[2] = {9,255};
                //                        send_tlv_from_c(j, tlv_stopAdv, 2, "Stop advertising for Phone");
                //                        //tlv_relayedPeriName[1] = devices[j].id;
                //                        //tlv_relayedPeriName[2] = devices[j].peri.type;
                //                        //tlv_relayedPeriName[3] = devices[j].tp.tx_idx;
                //                        //send_tlv_from_p(handle, tlv_relayedPeriName, 3);
                //                    }else{
                //                        set_other_device_op_mode(j, 3);
                //                    }
                //                }else if(!devices[j].peri.askedAdvPhone){
                //                    //tlv_relayedPeriName[1] = devices[j].id;
                //                    //tlv_relayedPeriName[2] = devices[j].peri.type;
                //                    //tlv_relayedPeriName[3] = devices[j].tp.tx_idx;
                //                    //send_tlv_from_p(handle, tlv_relayedPeriName, 4);
                //                }
                //            }
                //            //if(received_len-1 == count_onreq && count_onreq_not_phone != 0){
                //            //    send_usb_data(false, "FIX IN [9,peris], count onreq not phone = %d", count_onreq_not_phone);
                //            //    count_onreq_not_phone = 0;

                //            //}
                //        }
                //        else{
                //        //    //calculate_on_request_params(count_onreq);
                //        //    update_conn_params(handle, on_request_conn_params);
                //            send_usb_data(false, "Phone connected to all direct sending devices, start conn param update");
                            
                //        }
                //        devices[handle].conn_params = conn_params_phone;
                //        //update_conn_start_scan_wait_conn_param_updateparams(handle, conn_params_phone);
                //        devices[handle].cent.start_send_after_conn_update = true;
                //        //ble_gap_conn_params_t old_t1_sink_params = {
                //        //    .min_conn_interval = conn_params_on_gather_sink.min_conn_interval,
                //        //    .max_conn_interval = conn_params_on_gather_sink.max_conn_interval,
                //        //    .slave_latency = conn_params_on_gather_sink.slave_latency,
                //        //    .conn_sup_timeout = conn_params_on_gather_sink.conn_sup_timeout,
                //        //    };
                        
                //        //if(!devices[handle].cent.start_send_after_conn_update){
                //        //    //calculate_relay_params();
                //        //    if(connectedToPhone && old_t1_sink_params.min_conn_interval != conn_params_on_gather_type1_sink.min_conn_interval){
                //        //        send_tlv_from_p(handle, tlv_advertiseSink, 4);
                //        //    }
                //        //    devices[handle].conn_params = conn_params_phone
                //        //    update_conn_params(handle, conn_params_phone);
                //        //    //devices[handle].cent.start_send_after_conn_update = true;
                //        //}
                //    }
                //    //break;
                //}
                else if(received_data[0] == 20 && send_test_rssi)//test rssi
                {
                    ////if(send_test_rssi && received_len == 4 && received_data[1] == package_storage[0].data[1]){
                    ////    package_storage[0].data[1] = package_storage[0].data[1] + 1;
                    ////    send_test_rssi_tlv();
                    ////}

                   
                    if(doTESTRSSI_SOLOBOLO){
                        change_rssi_margins_cent(-69, -62, -66);
                        change_ema_coefficient_window_specific(handle, RSSI_WINDOW_SIZE);
                        devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.received_counter = 1;
                        package_storage[0].data[0] = 21;
                        package_storage[0].data[2] = cent_min_rssi;//devices[6].tp.min_rssi_limit;
                        package_storage[0].data[3] = cent_min_rssi + 7;//devices[6].tp.max_rssi_limit;
                        package_storage[0].data[4] = cent_min_rssi + 3;//devices[6].tp.ideal;
                        package_storage[0].data[6] = last_val_rssi;
                        package_storage[0].data[7] = (int8_t) devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.ema_rssi;
                        package_storage[0].data[8] = (int8_t) tx_range[devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.own_idx];
                        package_storage[0].data[9] = (int8_t) devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.received_counter;

                        //package_storage[0].data[1] = package_storage[0].data[1] + 1; 
                        send_tlv_from_p(NRF_SDH_BLE_CENTRAL_LINK_COUNT, package_storage[0].data, package_storage[0].length);
                        //break;
                    }else{
                         if(handle == NRF_SDH_BLE_CENTRAL_LINK_COUNT){
                            start_test_rssi();
                            return;
                        }
                    }
                }
                else if(received_data[0] == 21 && send_test_rssi && (devices[handle].last_test_pref < received_data[1] || (devices[handle].last_test_pref == 255 && received_data[1] == 0)))//test rssi
                {
                    if(doTESTRSSI_SOLOBOLO){
                        package_storage[0].data[6] = last_val_rssi;
                        package_storage[0].data[7] = (int8_t) devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.ema_rssi;
                        package_storage[0].data[8] = (int8_t) tx_range[devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.own_idx];
                        package_storage[0].data[9] = (int8_t) devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.received_counter;

                        package_storage[0].data[1] = received_data[1] + 1; 
                        devices[handle].last_test_pref = received_data[1];
                        send_tlv_from_p(NRF_SDH_BLE_CENTRAL_LINK_COUNT, package_storage[0].data, package_storage[0].length);
                    }
                    //if(handle == NRF_SDH_BLE_CENTRAL_LINK_COUNT){
                    //    start_test_rssi();
                    //    return;
                    //}
                    //break;
                }
                
                //default:
                //{
                //    //send_usb_data("Got unknown command from Central %s", devices[handle].name);
                //    break;
                //} 
                
            
            //}
            break;

            //OLD: 2 number commands -> Look in the end
            
        }

        case BLE_NUS_EVT_TX_RDY:
        {
            //if(peri_tlv_count > 0){
            //    send_tlv_to_peripherals();
            //}
            //if(cent_tlv_count > 0){
            //    send_tlv_to_cent();
            //}
            if(TEST_RSSI && sending_rssi && !IS_RELAY && devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].connected && handle == NRF_SDH_BLE_CENTRAL_LINK_COUNT){
                //tp_timer_start(&devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT]);
            }
            if(total_pckg_count > 0){
                process_stored_tlv();
            }
            if(start_send_data_not_received && there_are_stopped && package_count < package_storage_size / 3){
                send_tlv_from_c(-4, tlv_resumeData, 1, "RESUME");
            }

            if(sending_periodic_tlv && CHOSEN_HANDLE != NRF_SDH_BLE_TOTAL_LINK_COUNT && devices[CHOSEN_HANDLE].cent.isNotPaused && package_count>0){
                tp_timer_start(&devices[handle]);
                //tp_timer_stop(&devices[evt.conn_handle]);
                send_periodic_tlv();//(m_nus, CHOSEN_HANDLE);
            }
            if(sending_stored_tlv && !pause_chunks){
                if(sending_missed_tlv && !finished_missing){
                    send_missing_tlv();
                }
                else if(!sending_missed_tlv && !finished_stored){
                    //tp_timer_stop(&devices[evt.conn_handle]);
                    tp_timer_start(&devices[handle]);
                    send_stored_tlv();
                }
            }
            //if(sending_tlv_chunks && !pause_chunks){
            //    //chunk_completed++;
            //    send_chunk_tlv();//(m_nus, p_evt->conn_handle);
            //}
            //if(sending_relay_tlv){
            //    send_relay_tlv();//(m_nus, p_evt->conn_handle);
            //}
            //if(sending_rssi && devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].connected){
            //    send_test_rssi_tlv();
            //}
            //if(sending_tlv_spec && !pause_chunks){
            //    send_specific_tlv();//(m_nus, p_evt->conn_handle);
            //}

            //if(!SEND_TYPE && waitingRecFromUsb && !start_send_data_not_received && !sending_tlv_spec && !sending_tlv_chunks){
            //    if(package_count == 0){
            //        waitingRecFromUsb = false;
            //        if(large_received){
            //            //send_tlv_from_c(-1,tlv_reqData,(uint16_t) 2, "send data");
            //            sending_tlv_chunks = true;
            //            //chunk_sent = 0;
            //            package_tail = 0;
            //            send_usb_data(false, "sending data to handle:%d", p_evt->conn_handle);
            //            large_received = 0;
            //            last = true; //
            //            send_chunk_tlv();//(m_nus, p_evt->conn_handle);
                
            //        }
            //        else{
            //            //send_tlv_from_c(-1,tlv_reqData,(uint16_t) 2, "get and send data");
            //            //chunk_sent = 0;
            //            package_tail = 0;
            //            //sending_tlv_chunks = true;
            //            //set_cent_info(m_nus, p_evt->conn_handle);
            //            CHOSEN_HANDLE = p_evt->conn_handle;
            //            send_usb_data(false, "receiving usb and sending data to handle:%d", p_evt->conn_handle);
            //            request_usb_data();
            //        }
            //    }
            //}
            break;
        }
        case BLE_NUS_EVT_COMM_STARTED:
            ////change_conn_tx(-20,p_evt->conn_handle,-1,true);
            send_usb_data(false, "NUS communication started with hndl:%d.", p_evt->conn_handle);
            //ret_code_t err_code = sd_ble_gap_rssi_start(p_evt->conn_handle, 1, 0);
            //if(PERIS_DO_TP){
                //sd_ble_gap_rssi_start(p_evt->conn_handle, 1, 0);
            //}
            ////APP_ERROR_CHECK(err_code);
            //send_usb_data(false, "Started RSSI for Cent handle:%d ,e:%d", p_evt->conn_handle, err_code);
            ////devices[p_evt->conn_handle].tp.rssi = 0;
            //////devices[p_evt->conn_handle].handle = p_evt->conn_handle;
            ////devices[p_evt->conn_handle].tp.incr_counter = 0;
            ////devices[p_evt->conn_handle].tp.decr_counter = 0;
            ////devices[p_evt->conn_handle].tp.tx_idx = 0;
            //send_usb_data(false, "devices[%d] was added for hndl:%d, own TP:%d", p_evt->conn_handle, p_evt->conn_handle, tx_range[devices[p_evt->conn_handle].tp.tx_idx]);
            ////uint8_t tlv_name[4] ={3,DEVICE_ID, SEND_TYPE, IS_RELAY}; // was [4] = 0,3,...
            ////memcpy(tlv_name+2, DEVICE_NAME, 5);
            send_tlv_from_p(handle, tlv_nameParams, 5);

            //send_tlv_from_p(handle, tlv_nameParams, 5);
            //update_conn_params(handle, conn_params_conn_phone_wait);
            //first_packet_timer_handle = handle;
            //first_packet_timer_start();

            //change_conn_tx(tx_range[default_tx_idx], handle, (int8_t) devices[handle].tp.ema_rssi, (int8_t) devices[handle].tp.last_rssi);
            //if(ble_conn_state_peripheral_conn_count() < NRF_SDH_BLE_PERIPHERAL_LINK_COUNT){
            //    adv_start(false);
            //}
            break;

        case BLE_NUS_EVT_COMM_STOPPED:
            send_usb_data(false, "NUS communication stopped.");
            break;

        default:
            // Handle other possible events, if necessary
            break;
    }
}

#endif // NRF_MODULE_ENABLED(BLE_NUS)


//void nus_data_handler(ble_nus_evt_t * p_evt)
//{
//    if (p_evt == NULL)
//    {
//        return;
//    }
//    uint16_t handle = p_evt->conn_handle;
//    switch (p_evt->type)
//    {
//        case BLE_NUS_EVT_RX_DATA:
//        {   
//            NRF_LOG_HEXDUMP_INFO(p_evt->params.rx_data.p_data, p_evt->params.rx_data.length);
            
//            //send_usb_data(false, "REC: %s",p_evt->params.rx_data.p_data);
//            int8_t rssi = 0;
//            uint8_t channel = 0;
            

//            ret_code_t err_code = sd_ble_gap_rssi_get(handle, &rssi, &channel);

//            // Process the received data
//            // For example, you can echo the received data back to the central device
            
//            uint8_t * received_data = p_evt->params.rx_data.p_data;
//            uint16_t received_len = p_evt->params.rx_data.length;
//            //send_usb_data(false, "Received RSSI:%d, e:%d, len:%d", rssi, err_code, received_len);
//            if(received_data[0] == 255 && received_data[1] == 255 && received_data[2] == 255 && received_data[3] == 255){
//                send_tlv_usb(false, "USBerr", received_data,4,received_len, handle);
//                break;
//            }
//            //if(!send_test_rssi && received_data[0] != 20){
//            //    char msgrssi[200] = "";
//            //    sprintf(msgrssi, "Received RSSI=%d: ", rssi);
//            //    send_tlv_usb(true, msgrssi,received_data,received_len,received_len,handle);
//            //}
//            //uint8_t first_rec = received_data[1];
            
//            int command_id = (int) received_data[0];
//            //send_usb_data(false, "Got message command=%d, is3=%d, is6=%d, is7=%d", received_data[0], received_data[0]==3, received_data[0] == 6, received_data[0] == 7);
//            //NEW: 1 number commands
//            //switch(command_id)
//            //{
//                if(PERIS_DO_TP){
//                    //tp_timer_start(&devices[handle]);
//                }
//                if(received_data[0] == 0 && !send_test_rssi)//case 0:
//                {// Phone sent start gather to relay
//                    if (!data_gather_relay_on){
//                        data_gather_relay_on = true;
                       

//                        CHOSEN_HANDLE = PHONE_HANDLE;
                        
//                        if(!sending_stored_tlv && !sending_periodic_tlv){
//                            //TODO: check if these two below are needed
//                            //calculate_peri_to_relay_on_request_params(count_onreq_not_phone);
//                            //update_onreq_conn(10);

//                            //send_tlv_from_c(-3, tlv_reqData, 2, "Sent start Sending req data");
//                            for(int i = 0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
//                                if(devices[i].connected
//                                    //&& (devices[i].peri.type == 1 || devices[i].peri.type == 2)
//                                    && !devices[i].peri.askedAdvPhone
//                                    && devices[i].peri.chosenRelay) // allowed types for direct sending
//                                {
//                                    //devices[i].tp.use_pdr = true;
//                                    set_other_device_op_mode(i, 1);
//                                }
//                            }

//                            sending_periodic_tlv = true;
//                            //update_conn_params(handle, conn_params_phone);
//                            updating_ongather_conn = true;
//                            update_all_conn_for_gather();
//                            sending_own_periodic = false;
//                            if(IS_SENSOR_NODE && stored_bytes_sent < stored_bytes){
//                                sending_stored_tlv = true;
//                                send_stored_tlv();
//                                bsp_board_leds_off();
//                                bsp_board_led_on(led_GREEN);
//                            }else if(IS_SENSOR_NODE){
//                                //send_tlv_from_c(-3, tlv_reqData, 2, "Sent start Sending req data");
//                                sending_periodic_tlv = true;
//                                if(!periodic_timer_started){
//                                    periodic_timer_start();
//                                    periodic_timer_started = true;
//                                    bsp_board_leds_off();
//                                    bsp_board_led_on(led_BLUE);
//                                }
//                                sending_own_periodic = true;
//                                //send_periodic_tlv();
//                            }
//                            send_periodic_tlv();

//                            //updating_ongather_conn = true;
//                            //update_all_conn_for_gather();


//                            //uint8_t tlv_otherName[4] = {3};
//                            //for(int i=0;i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
//                            //    if(devices[i].connected && !devices[i].peri.askedAdvPhone && devices[i].id != 255){
//                            //        tlv_otherName[1] = devices[i].id;
//                            //        tlv_otherName[2] = devices[i].peri.type;
//                            //        tlv_otherName[3] = devices[i].tp.tx_idx;
//                            //        //send_tlv_from_p(handle, tlv_otherName, 4);
//                            //    }
//                            //}
//                        }
//                    }
//                    //break;
//                }
//                else if(received_data[0] == 1 && !send_test_rssi)//case 1:
//                {// To Peris: Start sending data
//                    //if (start_send_data_not_received && !sending_tlv_spec && !sending_tlv_chunks){
//                    if (!sending_stored_tlv && !sending_periodic_tlv){
//                        if(received_len == 2 && received_data[1] == 'P'){// if sender is Phone
//                            PHONE_HANDLE = handle;
//                            strcpy(devices[handle].name, "Phone");
//                            devices[handle].cent.isPhone = true;
//                            connectedToPhone = true;
//                            is_sending_to_phone = true;
//                            //calculate_on_request_params(count_onreq_not_phone);
//                            //update_conn_params(handle, on_request_conn_params);
//                            //disconnect_handle(RELAY_HANDLE);
//                            //update_conn_params(handle, conn_params_phone);
//                            bsp_board_leds_off();
//                            bsp_board_led_on(led_BLUE);
//                        }
//                        //start_send_data_not_received = false;
//                        CHOSEN_HANDLE = handle;
                        
                        
//                        sending_own_periodic = false;
//                        if(stored_bytes_sent < stored_bytes){
//                            sending_stored_tlv = true;
//                            send_stored_tlv();
//                            bsp_board_led_on(led_GREEN);
//                        }else{
//                            sending_periodic_tlv = true;
//                            sending_own_periodic = true;
//                            if(!periodic_timer_started){
//                                periodic_timer_start();
//                                periodic_timer_started = true;
//                            }
//                            send_periodic_tlv();
//                            bsp_board_led_on(led_BLUE);
//                        }
//                    }
//                    //break;
//                }
//                else if(received_data[0] == 2 && !send_test_rssi)//case 2:
//                {
//                    //if(!periodic_timer_started){
//                    //    periodic_timer_start();
//                    //    periodic_timer_started = true;
//                    //}
//                    //break;
//                }
//                else if(received_data[0] == 36 && received_len == 2){
//                    devices[handle].tp.other_tx_idx = received_data[1];
//                    if(!PERIS_DO_TP){
//                        devices[handle].tp.tx_idx = received_data[1];
//                    }
//                    devices[handle].tp.requested_tx_idx = -1;
//                    if(devices[handle].tp.received_counter != -1){
//                        devices[handle].tp.received_counter = 0;
//                    }
//                    send_usb_data(true, "INIT OTHERS %s:%d to ME to %d dBm, idx:%d, PERISdoTPC = %d", devices[handle].name, handle, devices[handle].tp.tx_range[devices[handle].tp.other_tx_idx], devices[handle].tp.other_tx_idx, PERIS_DO_TP);
//                }

//                else if(received_data[0] == 99 && received_len <= 3 ){
//                    //devices[handle].tp.tx_idx = received_data[1];
//                    //devices[handle].tp.requested_tx_idx = -1;
//                    //if(devices[handle].tp.received_counter != -1){
//                    //    devices[handle].tp.received_counter = 0;
//                    //}
//                    do_rssi_ema_self(handle, (int8_t) received_data[1]);
//                }

//                else if(received_data[0] == 3)//case 3:
//                {// CENT SENT NAME
//                    //else if (received_data[1] == 3 && received_len==3){// double check peris send 0,3 to phone
//                    //if (received_len==3){
//                        //ALLCONN: Add Cent id 
//                        //send_tlv_usb(true, "Got peri name ", received_data, 5, 5, handle);
//                        if(devices[handle].connected || (received_len != 5 && received_len != 3)){//devices[handle].id == received_data[1] && devices[handle].connected){
//                            return;
//                        }
//                        //devices[handle].connected = true;
                    

//                        //register_device_info(handle, received_data[1], received_data[2]==0);
//                        set_other_device_sim_params(handle, received_data);
                        
                        
//                        sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_CONN, handle, tx_range[default_tx_idx]);
//                        send_usb_data(true, "INIT Own TP to %s:%d to %d dBm", devices[handle].name, handle, devices[handle].tp.tx_range[default_tx_idx]);
//                        devices[handle].tp.own_idx = default_tx_idx;
//                        //change_conn_tx(tx_range[default_tx_idx], handle, (int8_t) devices[handle].tp.ema_rssi, (int8_t) devices[handle].tp.last_rssi);
//                        tlv_rssiChange[0] = 26;
//                        tlv_rssiChange[1] = default_tx_idx;
//                        //tlv_rssiChange[2] = 0;
//                        //tlv_rssiChange[3] = 0;
//                        //if(i != devices[h].tp.requested_tx_idx && devices[h].tp.requested_tx_idx == -1){
//                            //devices[h].tp.requested_tx_idx = i;
//                            //if(h < NRF_SDH_BLE_CENTRAL_LINK_COUNT){
//                                //send_tlv_from_c(h, tlv_rssiChange, 4, "Incr TX pow");
//                            //}else{
//                                send_tlv_from_p(handle, tlv_rssiChange, 2);
                        

//                        if(send_test_rssi && handle == NRF_SDH_BLE_CENTRAL_LINK_COUNT){
//                            //start_test_rssi();
//                            return;
//                        }
//                        //devices[handle].peri.all_received_counter = 0;

//                        // Cent start periodic and tell other SNs to do so too
//                        if(devices[handle].id == 'P'){
//                            if(IS_RELAY){
//                                //uint8_t tlv_allNames[17] = {3, DEVICE_ID, SEND_TYPE, min_tx_idx_own, max_tx_idx_own};

//                                send_tlv_from_p(handle, tlv_nameParams, 5);
//                                uint8_t start_periodic_tlv[1] = {2};
//                                send_tlv_from_c(-1, start_periodic_tlv, 1, "Start periodic data");
//                                //if(IS_SENSOR_NODE && !periodic_timer_started){
//                                //    periodic_timer_start();
//                                //    periodic_timer_started = true;
//                                //}

//                                if(received_data[2] == 1){//P connected to gather data
//                                    uint8_t tlv_phoneScanPeris[10] = {8};
//                                    int count_phoneScanPeris = 1;
                                    
//                                    for(int i = 0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
//                                        if(devices[i].connected && 
//                                            (devices[handle].tp.own_idx <= devices[i].tp.max_tx_idx ||

//                                            devices[handle].tp.ema_rssi> -72 ||
//                                            devices[handle].tp.last_rssi > -72)// TP check
//                                            && (devices[i].peri.type == 1 || devices[i].peri.type == 2)
//                                            && !devices[i].peri.askedAdvPhone) // allowed types for direct sending
//                                        {
//                                            relay_info.count_type1_sink++;
//                                        }
//                                    }

//                                    //calculate_relay_params();

//                                    for(int i = 0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
//                                        if(devices[i].connected && 
//                                            (devices[handle].tp.own_idx <= devices[i].tp.max_tx_idx ||
//                                            devices[handle].tp.ema_rssi + tx_range[devices[handle].tp.own_idx] - tx_range[devices[i].tp.max_tx_idx] > -70 )//||
//                                            //devices[handle].tp.ema_rssi> -72 ||
//                                            //devices[handle].tp.last_rssi > -72)// TP check
//                                            && (devices[i].peri.type == 1 || devices[i].peri.type == 2)
//                                            && !devices[i].peri.askedAdvPhone)  // allowed types for direct sending
//                                        {
//                                            //if(!devices[i].peri.isPeriodic){
//                                            send_tlv_from_c(i, tlv_advertiseSink, 4, "Type 1 start advertising");
//                                            send_usb_data(false, "%s Start adv for phone", devices[i].name);
//                                            //count_onreq_not_phone--;
//                                            tlv_phoneScanPeris[count_phoneScanPeris] = devices[i].id;
//                                            count_phoneScanPeris++;
//                                            //relay_info.count_type1_sink++;
//                                            send_usb_data(false, "Sent adv for Phone to %s, count phone scan peris = %d", devices[i].name, relay_info.count_type1_sink);
//                                            devices[i].peri.chosenRelay = false;
//                                            devices[i].peri.askedAdvPhone = true;
//                                        }
//                                    }

//                                    send_usb_data(false, "Send PhoneScanPeris, count phone scan peris = %d, t1_REL=%d, t1_Sink", count_phoneScanPeris, relay_info.count_type1, relay_info.count_type1_sink);
//                                    if(count_phoneScanPeris > 1){
//                                    //update_conn_params(handle, phone_scan_for_peris_conn_params);
//                                        send_tlv_from_p(handle, tlv_phoneScanPeris, count_phoneScanPeris);
//                                    }else{
//                                        devices[handle].conn_params = conn_params_phone;
//                                        update_conn_params(handle, conn_params_phone);
//                                        devices[handle].cent.start_send_after_conn_update = true;
//                                    }

//                                }
//                                //update_conn_params(handle, conn_params_periodic);
//                                devices[handle].conn_params = conn_params_phone;//conn_params_on_gather_sink;
                                
//                                //update_conn_params(handle, conn_params_on_gather_sink);

//                                //devices[handle].cent.start_send_after_conn_update = true;
//                                //update_all_conn_for_gather_t23();
//                            }
//                            else{
//                                devices[handle].conn_params = conn_params_phone;//conn_params_on_gather_sink;
//                                update_conn_params(handle, conn_params_phone);
//                                //uint8_t tlv_phoneReadyToStart[2] = {0,0};
//                                //send_tlv_from_p(handle, tlv_phoneReadyToStart, 2);
//                                devices[handle].cent.start_send_after_conn_update = true;
//                            }
//                            stop_adv();
//                            if(IS_RELAY){
//                                scan_stop();
//                            }
//                        }

//                    //break;
//                }
//                else if(received_data[0] == 4)//case 4: // was 10
//                {// PAUSE SENDING 
//                    //else if(received_data[1] == 0){// 0 == pause send data
//                        if(SEND_TYPE && devices[handle].cent.isNotPaused){
//                            package_tail = (package_tail+255)%package_storage_size;
//                            package_count++;
//                        }else{
//                            //last_prefix = received_data[2];
//                            devices[handle].cent.stoppedBatch = received_data[1];
//                            devices[handle].cent.stoppedPrefix = received_data[2];
//                            pause_chunks = true;
//                            send_usb_data(false, "%s is paused.", devices[handle].name);
//                            uint8_t tlv[1] = {0}; //was 0,0
//                            send_tlv_from_p(handle, tlv, 1);
//                        }
//                        devices[handle].cent.isNotPaused = false;
//                    //}

//                    //break;
//                }
//                else if(received_data[0] == 5)//case 5: // was 11
//                { // RESUME SENDING
//                    //else if (received_data[1] == 1){//1 == Resume send data
//                        ////if(sending_tlv_spec){
//                        ////    pause_chunks = false;
//                        ////    send_specific_tlv();//(m_nus, p_evt->conn_handle);
//                        ////    send_usb_data(false, "Got Start again Spec");
//                        ////}else 
//                        // TODO: DOnt know whats happening, maybe use only else part
//                        if(devices[handle].cent.stoppedBatch == 0){//received_len == 2){//normal resume
//                            if(sending_periodic_tlv){
//                                devices[handle].cent.isNotPaused = true;
//                                if(CHOSEN_HANDLE == handle){
//                                    send_periodic_tlv();//(m_nus, CHOSEN_HANDLE);
//                                    send_usb_data(false, "%s is resumed. CHOSEN:%d", devices[handle].name, CHOSEN_HANDLE == handle);
//                                }
                        
//                            }else{
//                                pause_chunks = false;
//                                package_tail = last_prefix+1;
//                                CHOSEN_HANDLE = handle;
//                                send_chunk_tlv();//(m_nus, p_evt->conn_handle);
//                                send_usb_data(false, "Got Start again Chunks");
//                            }
//                        }else{//disconnected while sending on req
//                            //TODO: Check where it starts sending
//                            CHOSEN_HANDLE = handle;
//                            pause_chunks = false;
//                            sending_tlv_chunks = true;
//                            // NEW
//                            if(large_counter == devices[handle].cent.stoppedBatch){
//                                send_usb_data(false, "Got restart sending SAME BATCH, old count=%d, new count=%d; old tail=%d, new tail=%d",
//                                                package_count, package_count + (package_tail - devices[handle].cent.stoppedPrefix), package_tail, devices[handle].cent.stoppedPrefix);
//                                package_count = package_count + (package_tail - devices[handle].cent.stoppedPrefix );
//                                package_tail = devices[handle].cent.stoppedPrefix;
//                            }else if (large_counter < devices[handle].cent.stoppedBatch){
//                                send_usb_data(false, "Got restart sending on BIGGER BATCH, old count=%d, old tail=%d; recBatch=%d, recPref=%d, oldBatch=%d, oldPref=%d",
//                                                package_count, package_tail, devices[handle].cent.stoppedBatch, devices[handle].cent.stoppedPrefix, large_counter, package_tail);
//                                send_usb_data(false, "Get new batch usb and sending data to handle:%d", p_evt->conn_handle);
//                                large_counter = devices[handle].cent.stoppedBatch-1;
//                                request_usb_data();
//                            }else{
//                                send_usb_data(false, "Got restart sending on SMALLER BATCH, old count=%d, old tail=%d; recBatch=%d, recPref=%d, oldBatch=%d, oldPref=%d",
//                                                package_count, package_tail, devices[handle].cent.stoppedBatch, devices[handle].cent.stoppedPrefix, large_counter, package_tail);
//                                send_chunk_tlv();
//                            }

//                            // OLD
//                            //if(large_counter == received_data[2]){
//                            //    send_usb_data(false, "Got restart sending SAME BATCH, old count=%d, new count=%d; old tail=%d, new tail=%d",
//                            //                    package_count, package_count + (package_tail - received_data[3]), package_tail, received_data[3]);
//                            //    package_count = package_count + (package_tail - received_data[3]);
//                            //    package_tail = received_data[3];
//                            //}else if (large_counter < received_data[2]){
//                            //    send_usb_data(false, "Got restart sending on BIGGER BATCH, old count=%d, old tail=%d; recBatch=%d, recPref=%d, oldBatch=%d, oldPref=%d",
//                            //                    package_count, package_tail, received_data[2], received_data[3], large_counter, package_tail);
//                            //    send_usb_data(false, "Get new batch usb and sending data to handle:%d", p_evt->conn_handle);
//                            //    large_counter = received_data[2]-1;
//                            //    request_usb_data();
//                            //}else{
//                            //    send_usb_data(false, "Got restart sending on SMALLER BATCH, old count=%d, old tail=%d; recBatch=%d, recPref=%d, oldBatch=%d, oldPref=%d",
//                            //                    package_count, package_tail, received_data[2], received_data[3], large_counter, package_tail);
//                            //    send_chunk_tlv();
//                            //}
//                        }
//                    //}

//                    //break;
//                }
//                else if(received_data[0] == 66)//case 6:// was 17
//                { // MUST CHANGE TX 
//                    //else if(received_data[1] == 7){//Must incr rssi from Cent 1,7
//                        int8_t rec_tx = (int8_t) received_data[1];
//                        if(devices[handle].tp.own_idx != rec_tx){
//                            send_usb_data(true, "Got TIMEOUT Request own TP to %s:%d to %d dBm from %d dBm; ema=%d, rssiLast=%d", devices[handle].name, handle, devices[handle].tp.tx_range[rec_tx], tx_range[devices[handle].tp.own_idx], (int8_t) received_data[2], (int8_t) received_data[3]);
//                            change_conn_tx(devices[handle].tp.tx_range[rec_tx], handle, (int8_t) devices[handle].tp.ema_rssi, (int8_t) devices[handle].tp.last_rssi);
                            
//                        }
//                    //}
//                    //break;
//                } 
//                else if(received_data[0] == 6)//case 6:// was 17
//                { // MUST CHANGE TX 
//                    //else if(received_data[1] == 7){//Must incr rssi from Cent 1,7
//                        int8_t rec_tx = (int8_t) received_data[1];
//                        if(devices[handle].tp.own_idx != rec_tx){
//                            send_usb_data(true, "Got Request own TP to %s:%d to %d dBm from %d dBm; ema=%d, rssiLast=%d", devices[handle].name, handle, devices[handle].tp.tx_range[rec_tx], tx_range[devices[handle].tp.own_idx], (int8_t) received_data[2], (int8_t) received_data[3]);
//                            change_conn_tx(devices[handle].tp.tx_range[rec_tx], handle, (int8_t) devices[handle].tp.ema_rssi, (int8_t) devices[handle].tp.last_rssi);
                            
//                        }
//                    //}
//                    //break;
//                } 
//                else if(received_data[0] == 7)//case 7: // was 16
//                {// OTHER CHANGED TX
                   
//                        if (received_data[1] != devices[handle].tp.tx_idx){
//                            if(received_data[1] > devices[handle].tp.tx_idx){
//                                //send_usb_data(true, "%s:%d Increased TP to %d has ema=%d, rssiLast=%d",devices[handle].name, handle, devices[handle].tp.tx_range[received_data[1]], (int8_t) received_data[2], (int8_t) received_data[3]);
//                                send_usb_data(true, "%s:%d Increased TP to %d has ema=%d, rssiLast=%d",devices[handle].name, handle, devices[handle].tp.tx_range[received_data[1]], (int8_t) devices[handle].tp.ema_rssi, (int8_t) devices[handle].tp.last_rssi);
//                                ////send_usb_data(true, "Here to %s, emaOld=%d, emaNew=%d",devices[handle].name, devices[handle].tp.ema_rssi, rssi);
//                                ////devices[handle].tp.ema_rssi = devices[handle].tp.ema_rssi + abs(devices[handle].tp.tx_range[received_data[1]] - devices[handle].tp.tx_range[devices[handle].tp.tx_idx]);
//                                ////devices[handle].tp.ema_rssi = (float) rssi * 1.0;
//                                ////if(devices[handle].tp.received_counter >= 3){
//                                ////    devices[handle].tp.received_counter -= 3;
//                                ////}
//                                if(WAIT_FOR_TP_CONFIRMATION){
//                                    devices[handle].tp.received_counter = 0; //devices[handle].tp.received_counter/2;
//                                }

//                            }
//                            else if(received_data[1] < devices[handle].tp.tx_idx){
//                                //send_usb_data(true, "%s:%d Decreased TP to %d has ema=%d, rssiLast=%d",devices[handle].name, handle, devices[handle].tp.tx_range[received_data[1]], (int8_t) received_data[2], (int8_t) received_data[3]);
//                                send_usb_data(true, "%s:%d Decreased TP to %d has ema=%d, rssiLast=%d",devices[handle].name, handle, devices[handle].tp.tx_range[received_data[1]], (int8_t) devices[handle].tp.ema_rssi, (int8_t) devices[handle].tp.last_rssi);
//                                ////send_usb_data(true, "Here to %s, emaOld=%d, emaNew=%d",devices[handle].name, devices[handle].tp.ema_rssi, rssi);
//                                ////devices[handle].tp.ema_rssi = devices[handle].tp.ema_rssi - abs(devices[handle].tp.tx_range[received_data[1]] - devices[handle].tp.tx_range[devices[handle].tp.tx_idx]);
//                                ////devices[handle].tp.ema_rssi = (float) rssi * 1.0;
//                                ////if(devices[handle].tp.received_counter >= 3){
//                                ////    devices[handle].tp.received_counter -= 3;
//                                ////}
//                                if(WAIT_FOR_TP_CONFIRMATION){
//                                   devices[handle].tp.received_counter = 0; //devices[handle].tp.received_counter/2;
//                                }
//                            }
//                            if(WAIT_FOR_TP_CONFIRMATION){
//                                devices[handle].tp.tx_idx = received_data[1];
//                                devices[handle].tp.requested_tx_idx = -1;
//                            }
//                            if(PERIS_DO_TP){
//                                devices[handle].tp.other_tx_idx = received_data[1];
//                            }
//                        }
//                    //}
//                    //break;
//                }
//                else if(received_data[0] == 8 && !send_test_rssi)//case 8:// was 12
//                {// MISSING CHECK FROM PHONE
//                    //if(received_data[1] == 2){
//                        //TODO: get successfully sent chunkIDs from phone
//                        //TODO: in phone when sending missed, seperate different large counters with [255,255]
                    
//                        //for (int a = 3; a < received_data[2]; a+=3) {
//                        //    size_t idx = circ_buf.tail; //oldest
//                        //    // Check if the chunk ID is in the received_ids array
//                        //    for (size_t i = 0; i < 20; i++) {
//                        //        if ((received_data[a] == circ_buf.buffer[idx]) 
//                        //          && (received_data[a+1] == circ_buf.buffer[idx+1]) 
//                        //          && (received_data[a+2] == circ_buf.buffer[idx+2])) 
//                        //        {
//                        //            // Mark this chunk as invalid (e.g., by setting chunk_id to 0)
//                        //            circ_buf.buffer[idx] = 255;
//                        //            circ_buf.size--;
//                        //            break;
//                        //        }
//                        //        // Move to the next chunk in the circular buffer
//                        //        idx = (idx + 245) % MAX_FILE_SIZE;
//                        //    }
//                        //}
                      
//                      //send_usb_data(false, "Sent ALL STORED DATA, stored bytes = %d, sent = ", stored_bytes, stored_bytes_sent);
                      
//                      //if(IS_RELAY){
//                      //    send_tlv_from_c(-3, tlv_reqData, 2, "Sent start Sending req data");
//                      //    sending_periodic_tlv = true;
//                      //    sending_own_periodic = false;
//                      //    if(IS_SENSOR_NODE){
//                      //        send_tlv_from_c(-3, tlv_reqData, 2, "Sent start Sending req data");
//                      //        sending_periodic_tlv = true;
//                      //        if(!periodic_timer_started){
//                      //            periodic_timer_start();
//                      //            periodic_timer_started = true;
//                      //        }
//                      //        sending_own_periodic = true;
//                      //        //send_periodic_tlv();
//                      //    }
//                      //}
                      

//                        if(received_data[1] == (uint8_t) DEVICE_ID){//(ble_conn_state_central_conn_count()==0)){
//                            if(sentAllOwnStoredData){
//                                return;
//                                //int batch = 0;
//                                //int l = 0;
//                                //while(int i=2; i<received_len; i++){
                                    
//                                //}
//                            }
//                            if(received_len == 2 || true){
//                                //uint8_t temp[13] = {0,0,255,255,0,0,0,0,0,0,0,0,0};
//                                send_usb_data(true, "ALL STORED DATA sent correctly!!!");
                                
//                                memset(tlv_chooseCentResponse, 0, sizeof(tlv_chooseCentResponse));
//                                //send_usb_data(false, "count onreq not phone = %d", count_onreq_not_phone);
//                                //send_usb_data(false, "Disconnecting from Cent Handle:%d", p_evt->conn_handle);
//                                sentAllOwnStoredData = true;
//                                //if(devices[handle].id == 'P'){
//                                    if(!IS_RELAY){//DEVICE_ID != 'C' && DEVICE_ID != 'D'){
//                                        start_send_data_not_received = true;
//                                        //is_sending_to_phone = false;
//                                        //disconnect_handle(PHONE_HANDLE);
//                                        sending_stored_tlv = false;
//                                        sending_periodic_tlv = true;
//                                        sending_own_periodic = true;
//                                        if(!periodic_timer_started){
//                                            periodic_timer_start();
//                                            periodic_timer_started = true;
//                                        }
//                                        //update_conn_params(handle, conn_params_periodic);
//                                        pref_counter ++;
//                                        if(pref_counter == 0){
//                                            batch_counter++;
//                                        }
//                                        add_periodic_bytes(0);
//                                        send_periodic_tlv();
//                                        send_usb_data(true, "Start sending periodically");
//                                        //ble_gap_conn_params_t slow_periodic = conn_params_periodic;
//                                        //slow_periodic.slave_latency = slow_periodic.slave_latency / 2;
//                                        update_conn_params(handle, conn_params_periodic);
//                                        bsp_board_led_on(led_BLUE);

//                                        //change_gap_params(false);
//                                        //uint8_t tlv_discPhone[2] = {3, 1};
//                                        //for(int i=NRF_SDH_BLE_CENTRAL_LINK_COUNT; i<NRF_SDH_BLE_TOTAL_LINK_COUNT; i++){
//                                        //    if(IS_RELAY){
//                                        //        send_tlv_from_p(i, tlv_discPhone, 2);
//                                        //    }
//                                        //}
//                                    }else{
//                                        sending_stored_tlv = false;
//                                        sending_own_periodic = true;
//                                        sending_periodic_tlv = true;
//                                        if(!periodic_timer_started){
//                                            periodic_timer_start();
//                                            periodic_timer_started = true;
//                                        }
//                                        pref_counter ++;
//                                        if(pref_counter == 0){
//                                            batch_counter++;
//                                        }
//                                        add_periodic_bytes(0);
//                                        send_periodic_tlv();
//                                        send_usb_data(true, "Start sending own data periodically"); 
//                                        bsp_board_led_on(led_BLUE);
//                                    }
//                                    //else if(count_onreq_not_phone == 0){
//                                    //    update_conn_params(handle, phone_default_conn_params);
//                                    //}
//                                    //update_conn_params(handle, on_request_power_saving_conn_params);
//                                //}
//                                //adv_start(false);

//                            }else{
//                                //TODO: missing
//                                send_tlv_usb(false, "Got missing from phone for me ", received_data, received_len, received_len,handle);
//                            }
//                        }else if(IS_RELAY){
//                            //OPTIONAL: NEW
//                            //send_tlv_usb(false, "Got missing from phone for Relayed ", received_data, received_len, received_len, handle);
//                            bool everythingSent = true;
//                            int kot = -1;
//                            for(int j=0;j<NRF_SDH_BLE_CENTRAL_LINK_COUNT;j++){
//                                if(received_data[1] == devices[j].id){
                                    
//                                    if(devices[j].peri.finished != true){
//                                        kot = j;
//                                        //send_tlv_from_c(j, received_data, received_len, "Send missing from phone norm");
//                                        //send_tlv_from_c(-3, received_data, received_len, "Send missing from phone with -3");
//                                        send_tlv_from_c(kot, received_data, received_len, "Relay missing from phone");
//                                        if(received_len == 2){
//                                            //reset_device_peri(j);
//                                            //peri_relay_info[j].allDataSentFromPeri = false;
//                                            //peri_relay_info[j].last = -1;

//                                            //peri_relay_info[j].recPhoneMissData = false;
//                                            //peri_relay_info[j].no_space = -1;
//                                            //peri_relay_info[j].received_all_count = 0;
//                                            //peri_relay_info[j].last_missing_rec = -1;
//                                            //peri_relay_info[j].sent_missed_msg_cnt = 0;
//                                            //peri_relay_info[j].received_last_msg_cnt = 0;
//                                            //peri_relay_info[j].all_received_counter = 0;
//                                            //peri_relay_info[j].stopped = false;
//                                            //peri_relay_info[j].missed[0] = 2; // idx of last missed
//                                            //peri_relay_info[j].missed[1] = 0; // chunkID of last received, missed left to be rec. after
//                                            //peri_relay_info[j].waiting_retransmit = false;
//                                            devices[j].peri.finished = true;
//                                            break;
//                                            //return;
//                                        }
//                                    }
//                                }
//                                if(devices[j].peri.finished != true && !devices[j].peri.askedAdvPhone){
//                                    everythingSent = false;
//                                }
//                           }   
//                           if(everythingSent && sentAllOwnStoredData){
//                               //data_relay_in_process = false;
//                               start_send_data_not_received = true;
//                               //more_data_left = true;
//                               peri_relay_count = 0;
//                               send_usb_data(true, "ALL STORED DATA HAS BEEN RELAYED!");
//                               update_conn_params(handle, conn_params_periodic);
//                               //scan_start();

//                               //data_gather_relay_on = false;

//                               //is_sending_to_phone = false;
//                               //scan_on_interval_start();
//                               //adv_start(false);
//                           }
//                        }
//                    //}

//                    //break;
//                }
//                else if(received_data[0] == 9 && !send_test_rssi)//case 9:
//                {// cent sent start adv for phone
//                    //TODO
//                    if(!IS_RELAY){
//                        if(received_len == 2 && received_data[1] == 255){
//                            stop_adv();
//                        }else{
//                            if(received_len == 4 && !got_adv_phone){
//                                got_adv_phone = true;
//                                //count_onreq = received_data[1];// idk what they do
//                                //count_onreq_not_phone = received_data[1];// idk what they do

//                                //calculate_t1_sink_params(received_data[1], received_data[2], received_data[3]);
//                                send_usb_data(false, "Starting advertising to connect to Phone");
//                                change_gap_params(true);
//                                adv_start(false);
//                            }else if (got_adv_phone){// idk what they do 
//                                ble_gap_conn_params_t old_sink_params = {
//                                    .min_conn_interval = conn_params_on_gather_sink.min_conn_interval,
//                                    .max_conn_interval = conn_params_on_gather_sink.max_conn_interval,
//                                    .slave_latency = conn_params_on_gather_sink.slave_latency,
//                                    .conn_sup_timeout = conn_params_on_gather_sink.conn_sup_timeout,
//                                };
//                                //calculate_t1_sink_params(received_data[1], received_data[2], received_data[3]);
//                                if(connectedToPhone && old_sink_params.min_conn_interval != conn_params_on_gather_sink.min_conn_interval){
//                                    send_usb_data(false, "Got new gather params from relay");
//                                    update_conn_params(PHONE_HANDLE, conn_params_on_gather_sink);
//                                }
//                            }
//                        }
//                    }else{
//                        if(received_len > 1){
//                            bool isNotScanned = true;
//                            uint8_t tlv_relayedPeriName[4] = {3};
//                            for(int j=0; j<NRF_SDH_BLE_CENTRAL_LINK_COUNT; j++){
//                                isNotScanned = true;
//                                if(devices[j].peri.askedAdvPhone && !devices[j].peri.chosenRelay){
//                                    for(int i=1; i<received_len;i++){
//                                        if(devices[j].id == received_data[i]){
//                                            isNotScanned = false;
//                                            break;
//                                        }
//                                    }
//                                    if(isNotScanned){
//                                        devices[j].peri.askedAdvPhone = false;
//                                        devices[j].peri.chosenRelay = true;
                                        
//                                        count_onreq_not_phone++;
//                                        relay_info.count_type1_sink--;
//                                        send_usb_data(false, "Phone did not scan %s, count onreq not phone = %d", devices[j].name, count_onreq_not_phone);
//                                        uint8_t tlv_stopAdv[2] = {9,255};
//                                        send_tlv_from_c(j, tlv_stopAdv, 2, "Stop advertising for Phone");
//                                        //tlv_relayedPeriName[1] = devices[j].id;
//                                        //tlv_relayedPeriName[2] = devices[j].peri.type;
//                                        //tlv_relayedPeriName[3] = devices[j].tp.tx_idx;
//                                        //send_tlv_from_p(handle, tlv_relayedPeriName, 3);
//                                    }else{
//                                        set_other_device_op_mode(j, 3);
//                                    }
//                                }else if(!devices[j].peri.askedAdvPhone){
//                                    //tlv_relayedPeriName[1] = devices[j].id;
//                                    //tlv_relayedPeriName[2] = devices[j].peri.type;
//                                    //tlv_relayedPeriName[3] = devices[j].tp.tx_idx;
//                                    //send_tlv_from_p(handle, tlv_relayedPeriName, 4);
//                                }
//                            }
//                            //if(received_len-1 == count_onreq && count_onreq_not_phone != 0){
//                            //    send_usb_data(false, "FIX IN [9,peris], count onreq not phone = %d", count_onreq_not_phone);
//                            //    count_onreq_not_phone = 0;

//                            //}
//                        }
//                        else{
//                        //    //calculate_on_request_params(count_onreq);
//                        //    update_conn_params(handle, on_request_conn_params);
//                            send_usb_data(false, "Phone connected to all direct sending devices, start conn param update");
                            
//                        }
//                        devices[handle].conn_params = conn_params_phone;
//                        update_conn_params(handle, conn_params_phone);
//                        devices[handle].cent.start_send_after_conn_update = true;
//                        //ble_gap_conn_params_t old_t1_sink_params = {
//                        //    .min_conn_interval = conn_params_on_gather_sink.min_conn_interval,
//                        //    .max_conn_interval = conn_params_on_gather_sink.max_conn_interval,
//                        //    .slave_latency = conn_params_on_gather_sink.slave_latency,
//                        //    .conn_sup_timeout = conn_params_on_gather_sink.conn_sup_timeout,
//                        //    };
                        
//                        //if(!devices[handle].cent.start_send_after_conn_update){
//                        //    //calculate_relay_params();
//                        //    if(connectedToPhone && old_t1_sink_params.min_conn_interval != conn_params_on_gather_type1_sink.min_conn_interval){
//                        //        send_tlv_from_p(handle, tlv_advertiseSink, 4);
//                        //    }
//                        //    devices[handle].conn_params = conn_params_phone
//                        //    update_conn_params(handle, conn_params_phone);
//                        //    //devices[handle].cent.start_send_after_conn_update = true;
//                        //}
//                    }
//                    //break;
//                }
//                else if(received_data[0] == 20 && send_test_rssi)//test rssi
//                {
//                    ////if(send_test_rssi && received_len == 4 && received_data[1] == package_storage[0].data[1]){
//                    ////    package_storage[0].data[1] = package_storage[0].data[1] + 1;
//                    ////    send_test_rssi_tlv();
//                    ////}

                   
//                    if(doTESTRSSI_SOLOBOLO){
//                        change_rssi_margins_cent(-69, -62, -66);
//                        change_ema_coefficient_window_specific(handle, RSSI_WINDOW_SIZE);
//                        devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.received_counter = 1;
//                        package_storage[0].data[0] = 21;
//                        package_storage[0].data[2] = cent_min_rssi;//devices[6].tp.min_rssi_limit;
//                        package_storage[0].data[3] = cent_min_rssi + 7;//devices[6].tp.max_rssi_limit;
//                        package_storage[0].data[4] = cent_min_rssi + 3;//devices[6].tp.ideal;
//                        package_storage[0].data[6] = last_val_rssi;
//                        package_storage[0].data[7] = (int8_t) devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.ema_rssi;
//                        package_storage[0].data[8] = (int8_t) tx_range[devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.own_idx];
//                        package_storage[0].data[9] = (int8_t) devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.received_counter;

//                        //package_storage[0].data[1] = package_storage[0].data[1] + 1; 
//                        send_tlv_from_p(NRF_SDH_BLE_CENTRAL_LINK_COUNT, package_storage[0].data, package_storage[0].length);
//                        //break;
//                    }else{
//                         if(handle == NRF_SDH_BLE_CENTRAL_LINK_COUNT){
//                            start_test_rssi();
//                            return;
//                        }
//                    }
//                }
//                else if(received_data[0] == 21 && send_test_rssi && (devices[handle].last_test_pref < received_data[1] || (devices[handle].last_test_pref == 255 && received_data[1] == 0)))//test rssi
//                {
//                    if(doTESTRSSI_SOLOBOLO){
//                        package_storage[0].data[6] = last_val_rssi;
//                        package_storage[0].data[7] = (int8_t) devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.ema_rssi;
//                        package_storage[0].data[8] = (int8_t) tx_range[devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.own_idx];
//                        package_storage[0].data[9] = (int8_t) devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.received_counter;

//                        package_storage[0].data[1] = received_data[1] + 1; 
//                        devices[handle].last_test_pref = received_data[1];
//                        send_tlv_from_p(NRF_SDH_BLE_CENTRAL_LINK_COUNT, package_storage[0].data, package_storage[0].length);
//                    }
//                    //if(handle == NRF_SDH_BLE_CENTRAL_LINK_COUNT){
//                    //    start_test_rssi();
//                    //    return;
//                    //}
//                    //break;
//                }
                
//                //default:
//                //{
//                //    //send_usb_data("Got unknown command from Central %s", devices[handle].name);
//                //    break;
//                //} 
                
            
//            //}
//            break;

//            //OLD: 2 number commands -> Look in the end
            
//        }

//        case BLE_NUS_EVT_TX_RDY:
//        {
//            //if(peri_tlv_count > 0){
//            //    send_tlv_to_peripherals();
//            //}
//            //if(cent_tlv_count > 0){
//            //    send_tlv_to_cent();
//            //}
//            if(TEST_RSSI && sending_rssi && !IS_RELAY && devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].connected && handle == NRF_SDH_BLE_CENTRAL_LINK_COUNT){
//                //tp_timer_start(&devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT]);
//            }
//            if(total_pckg_count > 0){
//                process_stored_tlv();
//            }
//            if(start_send_data_not_received && there_are_stopped && package_count < package_storage_size / 3){
//                send_tlv_from_c(-4, tlv_resumeData, 1, "RESUME");
//            }

//            if(sending_periodic_tlv && CHOSEN_HANDLE != 255 && devices[CHOSEN_HANDLE].cent.isNotPaused && package_count>0){
//                send_periodic_tlv();//(m_nus, CHOSEN_HANDLE);
//            }
//            if(sending_stored_tlv && !pause_chunks){
//                send_stored_tlv();
//            }
//            //if(sending_tlv_chunks && !pause_chunks){
//            //    //chunk_completed++;
//            //    send_chunk_tlv();//(m_nus, p_evt->conn_handle);
//            //}
//            //if(sending_relay_tlv){
//            //    send_relay_tlv();//(m_nus, p_evt->conn_handle);
//            //}
//            if(sending_rssi && devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].connected){
//                send_test_rssi_tlv();
//            }
//            //if(sending_tlv_spec && !pause_chunks){
//            //    send_specific_tlv();//(m_nus, p_evt->conn_handle);
//            //}

//            //if(!SEND_TYPE && waitingRecFromUsb && !start_send_data_not_received && !sending_tlv_spec && !sending_tlv_chunks){
//            //    if(package_count == 0){
//            //        waitingRecFromUsb = false;
//            //        if(large_received){
//            //            //send_tlv_from_c(-1,tlv_reqData,(uint16_t) 2, "send data");
//            //            sending_tlv_chunks = true;
//            //            //chunk_sent = 0;
//            //            package_tail = 0;
//            //            send_usb_data(false, "sending data to handle:%d", p_evt->conn_handle);
//            //            large_received = 0;
//            //            last = true; //
//            //            send_chunk_tlv();//(m_nus, p_evt->conn_handle);
                
//            //        }
//            //        else{
//            //            //send_tlv_from_c(-1,tlv_reqData,(uint16_t) 2, "get and send data");
//            //            //chunk_sent = 0;
//            //            package_tail = 0;
//            //            //sending_tlv_chunks = true;
//            //            //set_cent_info(m_nus, p_evt->conn_handle);
//            //            CHOSEN_HANDLE = p_evt->conn_handle;
//            //            send_usb_data(false, "receiving usb and sending data to handle:%d", p_evt->conn_handle);
//            //            request_usb_data();
//            //        }
//            //    }
//            //}
//            break;
//        }
//        case BLE_NUS_EVT_COMM_STARTED:
//            ////change_conn_tx(-20,p_evt->conn_handle,-1,true);
//            //send_usb_data(false, "NUS communication started with hndl:%d.", p_evt->conn_handle);
//            //ret_code_t err_code = sd_ble_gap_rssi_start(p_evt->conn_handle, 1, 0);
//            if(PERIS_DO_TP){
//                sd_ble_gap_rssi_start(p_evt->conn_handle, 1, 0);
//            }
//            ////APP_ERROR_CHECK(err_code);
//            //send_usb_data(false, "Started RSSI for Cent handle:%d ,e:%d", p_evt->conn_handle, err_code);
//            ////devices[p_evt->conn_handle].tp.rssi = 0;
//            //////devices[p_evt->conn_handle].handle = p_evt->conn_handle;
//            ////devices[p_evt->conn_handle].tp.incr_counter = 0;
//            ////devices[p_evt->conn_handle].tp.decr_counter = 0;
//            ////devices[p_evt->conn_handle].tp.tx_idx = 0;
//            //send_usb_data(false, "devices[%d] was added for hndl:%d, own TP:%d", p_evt->conn_handle, p_evt->conn_handle, tx_range[devices[p_evt->conn_handle].tp.tx_idx]);
//            ////uint8_t tlv_name[4] ={3,DEVICE_ID, SEND_TYPE, IS_RELAY}; // was [4] = 0,3,...
//            ////memcpy(tlv_name+2, DEVICE_NAME, 5);
//            ////send_tlv_from_p(p_evt->conn_handle, tlv_name, 4);
//            send_tlv_from_p(handle, tlv_nameParams, 5);
//            //change_conn_tx(tx_range[default_tx_idx], handle, (int8_t) devices[handle].tp.ema_rssi, (int8_t) devices[handle].tp.last_rssi);
//            //if(ble_conn_state_peripheral_conn_count() < NRF_SDH_BLE_PERIPHERAL_LINK_COUNT){
//            //    adv_start(false);
//            //}
//            break;

//        case BLE_NUS_EVT_COMM_STOPPED:
//            //send_usb_data(false, "NUS communication stopped.");
//            break;

//        default:
//            // Handle other possible events, if necessary
//            break;
//    }
//}