#include "sdk_common.h"
#if NRF_MODULE_ENABLED(BLE_NUS_C)
#include <stdlib.h>

#include "ble.h"
#include "ble_nus_c2.h"
#include "ble_gattc.h"
#include "ble_srv_common.h"
#include "ble_conn_state.h"

//#include "usb_handler.h"
#include "boards.h"
//#include "bsp.h"
//#include "app_error.h"

//#define NRF_LOG_MODULE_NAME ble_nus_c
//#include "nrf_log.h"
#include "common.h"
#include "device_conn.h"
#include "devices_conn_params.h"
#include "devices_tp.h"
//int (*tlv_from_c_sender)(int h, bool rssi, uint8_t* tlv, uint16_t length, char* type) = send_tlv_from_c;
//NRF_LOG_MODULE_REGISTER();

// Queue tlv messages to centrals
int peri_tlv_head=0;
int peri_tlv_tail=0;
int peri_tlv_count=0;
int peri_tlv_max_size = MAX_PACKAGE_SIZE; // 1 less than max 244
int peri_tlv_storage_size = 50;
peri_tlv_storage_t peri_tlv_storage[50];

int max_peris_to_relay = 3;

/**@brief Function for intercepting the errors of GATTC and the BLE GATT Queue.
 *
 * @param[in] nrf_error   Error code.
 * @param[in] p_ctx       Parameter from the event handler.
 * @param[in] conn_handle Connection handle.
 */
static void gatt_error_handler(uint32_t   nrf_error,
                               void     * p_ctx,
                               uint16_t   conn_handle)
{
    ble_nus_c_t * p_ble_nus_c = (ble_nus_c_t *)p_ctx;

    //NRF_LOG_DEBUG("A GATT Client error has occurred on conn_handle: 0X%X", conn_handle);
    send_usb_data(false, "A GATT Client error has occurred on conn_handle: 0X%X", conn_handle);

    if (p_ble_nus_c->error_handler != NULL)
    {
        p_ble_nus_c->error_handler(nrf_error);
    }
}


void ble_nus_c_on_db_disc_evt(ble_nus_c_t * p_ble_nus_c, ble_db_discovery_evt_t * p_evt)
{
    ble_nus_c_evt_t nus_c_evt;
    memset(&nus_c_evt,0,sizeof(ble_nus_c_evt_t));

    ble_gatt_db_char_t * p_chars = p_evt->params.discovered_db.charateristics;

    // Check if the NUS was discovered.
    if (    (p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE)
        &&  (p_evt->params.discovered_db.srv_uuid.uuid == BLE_UUID_NUS_SERVICE)
        &&  (p_evt->params.discovered_db.srv_uuid.type == p_ble_nus_c->uuid_type))
    {
        for (uint32_t i = 0; i < p_evt->params.discovered_db.char_count; i++)
        {
            switch (p_chars[i].characteristic.uuid.uuid)
            {
                case BLE_UUID_NUS_RX_CHARACTERISTIC:
                    nus_c_evt.handles.nus_rx_handle = p_chars[i].characteristic.handle_value;
                    break;

                case BLE_UUID_NUS_TX_CHARACTERISTIC:
                    nus_c_evt.handles.nus_tx_handle = p_chars[i].characteristic.handle_value;
                    nus_c_evt.handles.nus_tx_cccd_handle = p_chars[i].cccd_handle;
                    break;

                default:
                    break;
            }
        }
        if (p_ble_nus_c->evt_handler != NULL)
        {
            nus_c_evt.conn_handle = p_evt->conn_handle;
            nus_c_evt.evt_type    = BLE_NUS_C_EVT_DISCOVERY_COMPLETE;
            p_ble_nus_c->evt_handler(p_ble_nus_c, &nus_c_evt);
        }
    }
}

/**@brief     Function for handling Handle Value Notification received from the SoftDevice.
 *
 * @details   This function uses the Handle Value Notification received from the SoftDevice
 *            and checks if it is a notification of the NUS TX characteristic from the peer.
 *            If it is, this function decodes the data and sends it to the application.
 *            
 * @param[in] p_ble_nus_c Pointer to the NUS Client structure.
 * @param[in] p_ble_evt   Pointer to the BLE event received.
 */
static void on_hvx(ble_nus_c_t * p_ble_nus_c, ble_evt_t const * p_ble_evt)
{
    // HVX can only occur from client sending.
    if (   (p_ble_nus_c->handles.nus_tx_handle != BLE_GATT_HANDLE_INVALID)
        && (p_ble_evt->evt.gattc_evt.params.hvx.handle == p_ble_nus_c->handles.nus_tx_handle)
        && (p_ble_nus_c->evt_handler != NULL))
    {
        ble_nus_c_evt_t ble_nus_c_evt;

        ble_nus_c_evt.evt_type = BLE_NUS_C_EVT_NUS_TX_EVT;
        ble_nus_c_evt.p_data   = (uint8_t *)p_ble_evt->evt.gattc_evt.params.hvx.data;
        ble_nus_c_evt.data_len = p_ble_evt->evt.gattc_evt.params.hvx.len;
        ble_nus_c_evt.conn_handle = p_ble_nus_c->conn_handle;

        p_ble_nus_c->evt_handler(p_ble_nus_c, &ble_nus_c_evt);
        //NRF_LOG_DEBUG("Client sending data.");
        //send_usb_data(false, "Client sending data.");
    }
}

uint32_t ble_nus_c_init(ble_nus_c_t * p_ble_nus_c, ble_nus_c_init_t * p_ble_nus_c_init)
{
    uint32_t      err_code;
    ble_uuid_t    uart_uuid;
    ble_uuid128_t nus_base_uuid = NUS_BASE_UUID;

    VERIFY_PARAM_NOT_NULL(p_ble_nus_c);
    VERIFY_PARAM_NOT_NULL(p_ble_nus_c_init);
    VERIFY_PARAM_NOT_NULL(p_ble_nus_c_init->p_gatt_queue);

    err_code = sd_ble_uuid_vs_add(&nus_base_uuid, &p_ble_nus_c->uuid_type);
    VERIFY_SUCCESS(err_code);

    uart_uuid.type = p_ble_nus_c->uuid_type;
    uart_uuid.uuid = BLE_UUID_NUS_SERVICE;

    p_ble_nus_c->conn_handle           = BLE_CONN_HANDLE_INVALID;
    p_ble_nus_c->evt_handler           = p_ble_nus_c_init->evt_handler;
    p_ble_nus_c->error_handler         = p_ble_nus_c_init->error_handler;
    p_ble_nus_c->handles.nus_tx_handle = BLE_GATT_HANDLE_INVALID;
    p_ble_nus_c->handles.nus_rx_handle = BLE_GATT_HANDLE_INVALID;
    p_ble_nus_c->p_gatt_queue          = p_ble_nus_c_init->p_gatt_queue;

    return ble_db_discovery_evt_register(&uart_uuid);
}

void ble_nus_c_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    ble_nus_c_t * p_ble_nus_c = (ble_nus_c_t *)p_context;

    if ((p_ble_nus_c == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    if ( (p_ble_nus_c->conn_handle == BLE_CONN_HANDLE_INVALID)
       ||(p_ble_nus_c->conn_handle != p_ble_evt->evt.gap_evt.conn_handle)
       )
    {
        return;
    }

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTC_EVT_HVX:
            on_hvx(p_ble_nus_c, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            if (p_ble_evt->evt.gap_evt.conn_handle == p_ble_nus_c->conn_handle
                    && p_ble_nus_c->evt_handler != NULL)
            {
                ble_nus_c_evt_t nus_c_evt;

                nus_c_evt.evt_type = BLE_NUS_C_EVT_DISCONNECTED;

                p_ble_nus_c->conn_handle = BLE_CONN_HANDLE_INVALID;
                p_ble_nus_c->evt_handler(p_ble_nus_c, &nus_c_evt);
            }
            break;

        default:
            // No implementation needed.
            break;
    }
}

/**@brief Function for creating a message for writing to the CCCD. */
static uint32_t cccd_configure(ble_nus_c_t * p_ble_nus_c, bool notification_enable)
{
    nrf_ble_gq_req_t cccd_req;
    uint8_t          cccd[BLE_CCCD_VALUE_LEN];
    uint16_t         cccd_val = notification_enable ? BLE_GATT_HVX_NOTIFICATION : 0;

    memset(&cccd_req, 0, sizeof(nrf_ble_gq_req_t));

    cccd[0] = LSB_16(cccd_val);
    cccd[1] = MSB_16(cccd_val);

    cccd_req.type                        = NRF_BLE_GQ_REQ_GATTC_WRITE;
    cccd_req.error_handler.cb            = gatt_error_handler;
    cccd_req.error_handler.p_ctx         = p_ble_nus_c;
    cccd_req.params.gattc_write.handle   = p_ble_nus_c->handles.nus_tx_cccd_handle;
    cccd_req.params.gattc_write.len      = BLE_CCCD_VALUE_LEN;
    cccd_req.params.gattc_write.offset   = 0;
    cccd_req.params.gattc_write.p_value  = cccd;
    cccd_req.params.gattc_write.write_op = BLE_GATT_OP_WRITE_REQ;
    cccd_req.params.gattc_write.flags    = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE;

    return nrf_ble_gq_item_add(p_ble_nus_c->p_gatt_queue, &cccd_req, p_ble_nus_c->conn_handle);
}


uint32_t ble_nus_c_tx_notif_enable(ble_nus_c_t * p_ble_nus_c)
{
    VERIFY_PARAM_NOT_NULL(p_ble_nus_c);

    if ( (p_ble_nus_c->conn_handle == BLE_CONN_HANDLE_INVALID)
       ||(p_ble_nus_c->handles.nus_tx_cccd_handle == BLE_GATT_HANDLE_INVALID)
       )
    {
        return NRF_ERROR_INVALID_STATE;
    }
    return cccd_configure(p_ble_nus_c, true);
}


uint32_t ble_nus_c_string_send(ble_nus_c_t * p_ble_nus_c, uint8_t * p_string, uint16_t length)
{
    VERIFY_PARAM_NOT_NULL(p_ble_nus_c);

    nrf_ble_gq_req_t write_req;

    memset(&write_req, 0, sizeof(nrf_ble_gq_req_t));

    if (length > BLE_NUS_MAX_DATA_LEN)
    {
        //NRF_LOG_WARNING("Content too long.");
        send_usb_data(false, "Content too long.");
        return NRF_ERROR_INVALID_PARAM;
    }
    if (p_ble_nus_c->conn_handle == BLE_CONN_HANDLE_INVALID)
    {
        //NRF_LOG_WARNING("Connection handle invalid.");
        send_usb_data(false, "Connection handle invalid.");
        return NRF_ERROR_INVALID_STATE;
    }

    write_req.type                        = NRF_BLE_GQ_REQ_GATTC_WRITE;
    write_req.error_handler.cb            = gatt_error_handler;
    write_req.error_handler.p_ctx         = p_ble_nus_c;
    write_req.params.gattc_write.handle   = p_ble_nus_c->handles.nus_rx_handle;
    write_req.params.gattc_write.len      = length;
    write_req.params.gattc_write.offset   = 0;
    write_req.params.gattc_write.p_value  = p_string;
    write_req.params.gattc_write.write_op = BLE_GATT_OP_WRITE_CMD;
    write_req.params.gattc_write.flags    = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE;

    return nrf_ble_gq_item_add(p_ble_nus_c->p_gatt_queue, &write_req, p_ble_nus_c->conn_handle);
}


uint32_t ble_nus_c_handles_assign(ble_nus_c_t               * p_ble_nus,
                                  uint16_t                    conn_handle,
                                  ble_nus_c_handles_t const * p_peer_handles)
{
    VERIFY_PARAM_NOT_NULL(p_ble_nus);

    p_ble_nus->conn_handle = conn_handle;
    if (p_peer_handles != NULL)
    {
        p_ble_nus->handles.nus_tx_cccd_handle = p_peer_handles->nus_tx_cccd_handle;
        p_ble_nus->handles.nus_tx_handle      = p_peer_handles->nus_tx_handle;
        p_ble_nus->handles.nus_rx_handle      = p_peer_handles->nus_rx_handle;
    }
    return nrf_ble_gq_conn_handle_register(p_ble_nus->p_gatt_queue, conn_handle);
}

ret_code_t send_specific_tlv_to_peripheral(uint16_t h, int tail){
    return ble_nus_c_string_send(devices[h].peri.nus, devices[h].tlvs[tail].data, devices[h].tlvs[tail].length);
}

void send_tlv_to_peripherals(void) {
   ret_code_t err_code;
    //OPTIONAL: Package storage
    while (peri_tlv_count > 0) {
        if(peri_tlv_storage[peri_tlv_tail].length != 0){
            err_code = ble_nus_c_string_send(devices[peri_tlv_storage[peri_tlv_tail].handle].peri.nus, peri_tlv_storage[peri_tlv_tail].data, peri_tlv_storage[peri_tlv_tail].length);
            if (err_code == NRF_ERROR_RESOURCES) {
                // Wait for BLE_NUS_EVT_TX_RDY before sending more
                return;
            }else if (err_code == NRF_ERROR_INVALID_STATE){
                send_tlv_usb(false, "ERR: Invalid state on send_tlv_to", peri_tlv_storage[peri_tlv_tail].data, peri_tlv_storage[peri_tlv_tail].length, peri_tlv_storage[peri_tlv_tail].length, peri_tlv_storage[peri_tlv_tail].handle);
                peri_tlv_count--;
                peri_tlv_tail = (peri_tlv_tail+1)%peri_tlv_storage_size;
                return;
            }
            else if (err_code != NRF_SUCCESS)
            {
                char msg[300] = "Failed to send_tlv_to_peri. ";
                int pos = 31;
                pos += sprintf(msg+pos, "Error 0x%x:", err_code);
                send_tlv_usb(false, msg, peri_tlv_storage[peri_tlv_tail].data, peri_tlv_storage[peri_tlv_tail].length, peri_tlv_storage[peri_tlv_tail].length, peri_tlv_storage[peri_tlv_tail].handle);
                return;
            }else{
                //char msg2[300] = "";
                //sprintf(msg2, "%s data sent, Type:%s", devices[peri_tlv_storage[peri_tlv_tail].handle].name, peri_tlv_storage[peri_tlv_tail].type);
                if(!send_test_rssi){
                    send_tlv_usb(true, peri_tlv_storage[peri_tlv_tail].type, peri_tlv_storage[peri_tlv_tail].data, peri_tlv_storage[peri_tlv_tail].length, peri_tlv_storage[peri_tlv_tail].length, peri_tlv_storage[peri_tlv_tail].handle);
                }
                peri_tlv_count--;
                peri_tlv_tail = (peri_tlv_tail+1)%peri_tlv_storage_size;
            }
        }else{
            peri_tlv_count--;
            peri_tlv_tail = (peri_tlv_tail+1)%peri_tlv_storage_size;
        }
    }

}


/**@brief Callback handling Nordic UART Service (NUS) client events.
 *
 * @details This function is called to notify the application of NUS client events.
 *
 * @param[in]   p_ble_nus_c   NUS client handle. This identifies the NUS client.
 * @param[in]   p_ble_nus_evt Pointer to the NUS client event.
 */
 // Removed rssi from hereasdsad
int send_tlv_from_c_OLD(int h, uint8_t* tlv, uint16_t length, char* type){
    ret_code_t err_code;

    //char msg[300] = "";
    //sprintf(msg, "Added msg to PeriQ:%s", type);
    //send_tlv_usb(false, msg, tlv, length, length, h);

    if(h > -1){
        uint16_t handle = h;
        memcpy(peri_tlv_storage[peri_tlv_head].data, tlv,length);
        //int typeLen = strlen(type);
        //if(typeLen > 99){
        //  typeLen = 99;
        //}
        //memcpy(peri_tlv_storage[peri_tlv_head].type, type, typeLen);
        if(strlen(type) < 299){
            strcpy(peri_tlv_storage[peri_tlv_head].type, type);
        }else{
            strncpy(peri_tlv_storage[peri_tlv_head].type, type, 250);
        }
        peri_tlv_storage[peri_tlv_head].length = length;
        peri_tlv_storage[peri_tlv_head].handle = handle;
        peri_tlv_head = (peri_tlv_head+1)%peri_tlv_storage_size;
        if(peri_tlv_count == peri_tlv_storage_size){
            send_usb_data(false, "Send to peri max size storage reached");
        }else{
          peri_tlv_count++;
        }
        
    }else if(h == -4){
        for (uint8_t i = 0; i < NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++)
        {   
            if(devices[i].id != 255 && devices[i].peri.stopped){
                uint16_t handle = i;
                memcpy(peri_tlv_storage[peri_tlv_head].data, tlv,length);
                int typeLen = strlen(type);
                if(typeLen > 99){
                  typeLen = 99;
                }
                memcpy(peri_tlv_storage[peri_tlv_head].type, type, typeLen);
                peri_tlv_storage[peri_tlv_head].length = length;
                peri_tlv_storage[peri_tlv_head].handle = handle;
                peri_tlv_head = (peri_tlv_head+1)%peri_tlv_storage_size;
                if(peri_tlv_count == peri_tlv_storage_size){
                    send_usb_data(false, "Send to peri MAX size storage reached");
                }else{
                  peri_tlv_count++;
                }
            }
        }
        there_are_stopped = false;
    }else{
        for (uint8_t i = 0; i < NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++)
        {   
            //if(i == 0 && h == -3){
            //    send_usb_data(true, "Sending req data to %s:NUM=%d ifPeriodic=%d, peri_tlv_count=%d", devices[i].name, devices[i].nameNum, !devices[i].peri.isPeriodic, peri_tlv_count);
            //    send_tlv_usb(true, "To Peri req data", tlv, length, length, i);
            //}
            if(devices[i].id != 255 && devices[i].connected &&
                (
                  h == -1 || 
                    //(devices[i].peri.chosenRelay == true &&  
                      //((h == -2 && devices[i].peri.isPeriodic) || (h == -3 && !devices[i].peri.isPeriodic)) 
                      ((h == -2) || (h == -3 && !devices[i].peri.askedAdvPhone)) 
                    //)
                )
            ){
                uint16_t handle = i;
                memcpy(peri_tlv_storage[peri_tlv_head].data, tlv,length);
                int typeLen = strlen(type);
                if(typeLen > 99){
                  typeLen = 99;
                }
                memcpy(peri_tlv_storage[peri_tlv_head].type, type, typeLen);
                peri_tlv_storage[peri_tlv_head].length = length;
                peri_tlv_storage[peri_tlv_head].handle = handle;
                peri_tlv_head = (peri_tlv_head+1)%peri_tlv_storage_size;
                if(peri_tlv_count == peri_tlv_storage_size){
                    send_usb_data(false, "Send to peri MAX size storage reached");
                }else{
                  peri_tlv_count++;
                }
                //if(i == 0 && h == -3){
                //    send_usb_data(true, "Sending req data to %s:NUM=%d ifPeriodic=%d, peri_tlv_count=%d, belowHandle:%d", devices[i].name, devices[i].nameNum, !devices[i].peri.isPeriodic, peri_tlv_count, peri_tlv_storage[peri_tlv_head].handle);
                //    send_tlv_usb(true, peri_tlv_storage[peri_tlv_head].type, peri_tlv_storage[peri_tlv_head].data, peri_tlv_storage[peri_tlv_head].length, peri_tlv_storage[peri_tlv_head].length, peri_tlv_storage[peri_tlv_head].handle);
                //}
            }
        }
    }
    send_tlv_to_peripherals();
    return 0;
}



int send_tlv_from_c(int h, uint8_t* tlv, uint16_t length, char* type){
    ret_code_t err_code;
    //if(h >=0){
    //    char msg[300] = "";
    //    sprintf(msg, "Added msg to PeriQ:%s", type);
    //    send_tlv_usb(false, msg, tlv, length, length, h);
    //}
    if(h > -1){
        add_to_device_tlv_strg(h, tlv, length);
    }else if(h == -4){
        for (uint8_t i = 0; i < NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++)
        {   
            if(devices[i].id != 255 && devices[i].peri.stopped){
                //uint16_t handle = i;
                //memcpy(peri_tlv_storage[peri_tlv_head].data, tlv,length);
                ////int typeLen = strlen(type);
                ////if(typeLen > 99){
                ////  typeLen = 99;
                ////}
                ////memcpy(peri_tlv_storage[peri_tlv_head].type, type, typeLen);
                //peri_tlv_storage[peri_tlv_head].length = length;
                //peri_tlv_storage[peri_tlv_head].handle = handle;
                //peri_tlv_head = (peri_tlv_head+1)%peri_tlv_storage_size;
                //if(peri_tlv_count == peri_tlv_storage_size){
                //    send_usb_data(false, "Send to peri MAX size storage reached");
                //}else{
                //  peri_tlv_count++;
                //}
                add_to_device_tlv_strg(i, tlv, length);
            }
        }
        there_are_stopped = false;
    }else{
        for (uint8_t i = 0; i < NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++)
        {   
            //if(i == 0 && h == -3){
            //    send_usb_data(true, "Sending req data to %s:NUM=%d ifPeriodic=%d, peri_tlv_count=%d", devices[i].name, devices[i].nameNum, !devices[i].peri.isPeriodic, peri_tlv_count);
            //    send_tlv_usb(true, "To Peri req data", tlv, length, length, i);
            //}
            if(devices[i].id != 255 && devices[i].connected &&
                (
                  h == -1 || 
                    //(devices[i].peri.chosenRelay == true &&  
                      //((h == -2 && devices[i].peri.isPeriodic) || (h == -3 && !devices[i].peri.isPeriodic)) 
                      ((h == -2) || (h == -3 && !devices[i].peri.askedAdvPhone)) 
                    //)
                )
            ){
                //uint16_t handle = i;
                //memcpy(peri_tlv_storage[peri_tlv_head].data, tlv,length);
                ////int typeLen = strlen(type);
                ////if(typeLen > 99){
                ////  typeLen = 99;
                ////}
                ////memcpy(peri_tlv_storage[peri_tlv_head].type, type, typeLen);
                //peri_tlv_storage[peri_tlv_head].length = length;
                //peri_tlv_storage[peri_tlv_head].handle = handle;
                //peri_tlv_head = (peri_tlv_head+1)%peri_tlv_storage_size;
                //if(peri_tlv_count == peri_tlv_storage_size){
                //    send_usb_data(false, "Send to peri MAX size storage reached");
                //}else{
                //  peri_tlv_count++;
                //}
                ////if(i == 0 && h == -3){
                ////    send_usb_data(true, "Sending req data to %s:NUM=%d ifPeriodic=%d, peri_tlv_count=%d, belowHandle:%d", devices[i].name, devices[i].nameNum, !devices[i].peri.isPeriodic, peri_tlv_count, peri_tlv_storage[peri_tlv_head].handle);
                ////    send_tlv_usb(true, peri_tlv_storage[peri_tlv_head].type, peri_tlv_storage[peri_tlv_head].data, peri_tlv_storage[peri_tlv_head].length, peri_tlv_storage[peri_tlv_head].length, peri_tlv_storage[peri_tlv_head].handle);
                ////}
                add_to_device_tlv_strg(i, tlv, length);
               
            }
        }
    }
    //send_tlv_to_peripherals();
    return 0;
}

int last_window = 0;

/**@snippet [Handling events from the ble_nus_c module] */
void ble_nus_c_evt_handler(ble_nus_c_t * p_ble_nus_c, ble_nus_c_evt_t const * p_ble_nus_evt)
{
    ret_code_t err_code;

    switch (p_ble_nus_evt->evt_type)
    {
        case BLE_NUS_C_EVT_DISCOVERY_COMPLETE:
            //send_usb_data(false, "Discovery complete.");
            err_code = ble_nus_c_handles_assign(p_ble_nus_c, p_ble_nus_evt->conn_handle, &p_ble_nus_evt->handles);
            //APP_ERROR_CHECK(err_code);
            check_error("ble_nus_c_handles_assign ERROR:",err_code);

            err_code = ble_nus_c_tx_notif_enable(p_ble_nus_c);
            //APP_ERROR_CHECK(err_code);
            check_error("ble_nus_c_tx_notif_enable ERROR:",err_code);
            send_usb_data(false, "Connected to device with Nordic UART Service.");
            break;

        case BLE_NUS_C_EVT_NUS_TX_EVT:
        {
            int h = p_ble_nus_evt->conn_handle;
            if(h >= NRF_SDH_BLE_CENTRAL_LINK_COUNT){
                return;
            }
            uint8_t *received_data;
            uint16_t received_len;
            uint8_t received_batch;
            uint8_t received_prefix;
            
            
            if(IS_RELAY){

            if(p_ble_nus_evt->data_len >=1){
                received_data = p_ble_nus_evt->p_data;
                received_len = p_ble_nus_evt->data_len;
                received_batch = received_data[0];
                if(p_ble_nus_evt->data_len >=2){
                    received_prefix = received_data[1];
                }
            }else{
                return;
            }
            
            if(!(received_len==5 && received_data[0] == 3)){
                if(received_len < 11){
                    if(received_len == devices[h].last_len){
                        if(memcmp(received_data,devices[h].last_rec,received_len) == 0){
                            return;
                        }
                    }
                    devices[h].last_len = received_len;
                    memcpy(devices[h].last_rec, received_data, received_len);
                }else{
                    devices[h].last_len = 0;
                }
            }
            
            if(received_len<14){//RELAY
                send_tlv_usb(true, "GOT to RELAY: ", p_ble_nus_evt->p_data, p_ble_nus_evt->data_len, p_ble_nus_evt->data_len, h);
            }
            //if(devices[h].curr_interval != 0 && !PERIS_DO_TP && !PERI_TP_TIMER){
            //    tp_timer_start(&devices[h]);
            //}
            if(received_data[0] == 255 && received_len >= 4 && received_data[1] == 255 && received_data[2] == 255 && received_data[3] == 255){
                send_tlv_usb(false, "USBerr", received_data,4,received_len,h);
                break;
            }
            //else if(received_len == 1 && received_data[0] == 12){
            //    // Peri will start sending periodic here
            //    if(!devices[h].peri.chosenRelay){
            //        devices[h].peri.chosenRelay = true;
            //        set_other_device_op_mode(h,2);
            //        update_conn_params(h, devices[h].conn_params);
            //    }
            //}
            else if(received_len == 2 && received_data[0] == 72 && devices[h].peri.askedAdvPhone){ // did peri conn to phone//RELAY
                if(received_data[1] == 0){// didnt conn to phone, use relay
                    send_usb_data(true, "%s did not connect to PHone", devices[h].name);
                    devices[h].peri.connToPhone = false;
                    devices[h].peri.chosenRelay = true;
                    if(devices[h].id == 'M'){
                        set_other_device_op_mode(h, 4);
                        update_conn_params(h, devices[h].conn_params);
                        
                    }else{
                        set_other_device_op_mode(h, 1);
                    }
                    //send_tlv_from_c(h, tlv_reqData, 2, "Sent start Sending req data");
                }else if(received_data[1] == 1){// conn to phone
                    if(devices[h].id == 'M'){
                      disconnect_handle(h);
                      
                    }else{
                        send_usb_data(true, "%s DID connect to PHone", devices[h].name);
                        devices[h].peri.connToPhone = true;
                        devices[h].peri.chosenRelay = false;
                        set_other_device_op_mode(h, 3);
                    }
                    //update_conn_params(h, devices[h].conn_params);
                }
                devices[h].peri.askedAdvPhone = false;
                bool askedAdvFinished = true;
                bool firstAsked = false;
                bool allToPhone = true;
                uint8_t ids[10] = {8};
                uint8_t tlv_phoneScanPeris[10] = {8};
                int count_phoneScanPeris1 = 1;
                for(int i = 0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
                // check if there are any peris askAdvPhone not done yet
                    if(devices[i].connected){ 
                        if(devices[i].peri.askedAdvPhone)
                        {
                            askedAdvFinished = false;
                            //break;
                        }
                        //if(devices[i].peri.firstAskAdv && devices[h].peri.chosenRelay){
                        //    allToPhone = false;
                        //}
                    }

                    //if(devices[i].connected && 
                    //            devices[i].peri.type < 3 && !devices[i].peri.firstAskAdv){
                    //   tlv_phoneScanPeris1[count_phoneScanPeris1] = devices[i].id;
                    //   ids[i] = i;
                    //   count_phoneScanPeris1++;                        
                    //}
                }
                if(askedAdvFinished){
                    send_tlv_from_p(PHONE_HANDLE, fast_tlv, 1);
                    //if(count_phoneScanPeris1 > 1 && !allToPhone){
                    //    send_tlv_from_p(PHONE_HANDLE, tlv_phoneScanPeris1, count_phoneScanPeris1);
                    //    for(int i=1; i<count_phoneScanPeris1;i++){
                    //        devices[ids[i]].peri.firstAskAdv = true;
                    //    }
                    //}
                    //else if (count_phoneScanPeris1 == 1 && allToPhone){
                    //    uint8_t tlv_phoneScanPeris[1] = {11};
                    //    send_tlv_from_p(PHONE_HANDLE, tlv_phoneScanPeris, 1);
                    //}

                    ////waiting_to_send_phone = false;
                    //devices[h].conn_params = conn_params_phone;
                    //updating_relay_conns = true;
                    //updated_phone_conn = false;
                    //updating_relay_conns = update_all_relay_conn();
                }

            }
            else if(received_len == 1 && received_data[0] == 2){//RELAY
                devices[h].peri.connToPhone = true;
                count_onreq_not_phone--;
            }else if(received_len == 2 && received_data[0] == 3){//RELAY
                devices[h].peri.connToPhone = false;
                if(received_data[2] == 1){
                    //devices[h].peri.finished = true;
                }else{
                    count_onreq_not_phone++;
                }
            }
            else if(received_batch == 0 && received_prefix == 0 && TEST_RSSI && received_len>200 && devices[h].tp.received_counter == -1 && !PERIS_DO_TP && !devices[h].tp.doing_const_test){
                  if(devices[h].tp.received_counter == -1){
                      devices[h].tp.received_counter = 0;
                      //devices[h].tp.do_first_ema = true;
                      devices[h].tp.ema_rssi = 0;
                  }
            }
            else if (received_len==5 && received_data[0] == 3){// && received_data[0] == 0//RELAY
                    char name[6];
                    //send_tlv_usb(true, "Got peri name ", received_data, 2, 3, h);
                    if(received_data[1] == devices[h].id){
                        if(devices[h].connected){
                            return;
                        }else if(devices[h].peri.is_sending_on_req && !devices[h].peri.finished){
                            //DEVICE LOST CONNECTION MID SENDING
                            //TODO: send resume with last prefix
                            send_usb_data(false, "%s connected again after being disconnected mid sending", devices[h].name);
                        }
                    }

                    set_other_device_sim_params(h, received_data);
                    //change_conn_tx(0, h, (int8_t) devices[h].tp.ema_rssi, (int8_t) devices[h].tp.last_rssi);
                    clear_updates_for_handle(h);
                    devices[h].peri.chosenRelay = true;
                    memcpy(devices[h].current_updating_conn_params.tlv, empty_tlv, 0);
                    devices[h].current_updating_conn_params.len = 0;
                    devices[h].current_updating_conn_params.do_tlv = false;
                        //}else{
                            //send_tlv_from_p(h, tlv_rssiChange, 2);
  
                    //if(devices[h].peri.isPeriodic){
                    //    count_periodic++;
                    //}else{
                    //    count_onreq++;
                    //    count_onreq_not_phone++;
                    //}
                    //if(devices[h].id != 255){
                    //    send_usb_data(false, "%s connected -> scan_for_req=%d, target_peri=%s, is_target_peri=%d, h==on_req->h =>%d", 
                    //                  devices[h].name, scanning_for_on_req, m_target_periph_name, 
                    //                  strncmp(name, m_target_periph_name,5), h == on_req_queue->handle);
                    //}

                    //if(devices[h].id != 255 && scanning_for_on_req && h == on_req_queue->handle){//devices[h].peri.is_sending_on_req){
                    //    // Device connecting again, or handle was used before
                    //    //memcpy(name, "Peri",4);
                    //    //name[4] =  (received_data[1] - 'I') + '1';
                    //    //name[5] = '\0';
                    //    send_usb_data(false, "In handle %d, old device is %s, new is %s, m_target=%s, strcmp=%d", h, devices[h].name, name, m_target_periph_name, strncmp(name, m_target_periph_name,5) == 0);
                    //    if(strncmp(devices[h].name, m_target_periph_name,5) == 0){
                    //        if(devices[h].peri.type == 1){
                    //            relay_info.disc_type1++;
                    //        }
                    //        if(devices[h].peri.is_sending_on_req){
                    //            send_tlv_from_c(h, tlv_nameParams, 5, "Reply with name ID");
                    //            if(devices[h].peri.batch_number == -1){
                    //                //tlv_resumeData[2] = 0;
                    //            }else{
                    //                //tlv_resumeData[2] = (uint8_t) devices[h].peri.batch_number;
                    //            }
                    //            if(devices[h].peri.last == -1){
                    //                //tlv_resumeData[3] = 0;
                    //            }else{
                    //                //tlv_resumeData[3] = (uint8_t) devices[h].peri.last;
                    //            }
                    //            char type[30];
                                
                    //            sprintf(type, "Restart on b:%d, p:%d", devices[h].peri.batch_number, devices[h].peri.last);
                    //            send_tlv_from_c(h, tlv_resumeData, 1, type);
                    //            //send_tlv_from_c(h, tlv_resumeData, 4, type);
                    //            //if(devices[PHONE_HANDLE].connected){
                    //            //    calculate_on_request_params(count_onreq_not_phone);
                    //            //    update_conn_params(PHONE_HANDLE, on_request_conn_params);
                    //            //    calculate_peri_to_relay_on_request_params(count_onreq_not_phone);
                    //            //    update_onreq_conn(h);
                    //            //}
                    //            ////update_conn_params(h, on_request_conn_params);
                    //            //calibrate_conn_params(h, 10);

                    //            //m_target_periph_names[devices[h].id-1] = "     ";
                    //            remove_peri_from_scan_list(devices[h].name);
                    //            send_usb_data(false, "Peri has name: %s, ID:%c, isPeriodic=%d, isRelay=%d", devices[h].name, devices[h].id,devices[h].peri.isPeriodic, devices[h].peri.isRelay);
                    //            //send_usb_data(false, "New scanning list is: %s %s %s %s %s %s", m_target_periph_names[0], m_target_periph_names[1], m_target_periph_names[2], m_target_periph_names[3], m_target_periph_names[4], m_target_periph_names[5]);
                    //        }else{
                    //            send_tlv_from_c(h, tlv_reqData, 2, "Sent start Sending req data");
                    //        }

                    //        scanning_for_on_req = false;
                    //        devices[h].peri.is_sending_on_req = true;
                    //        //if(devices[PHONE_HANDLE].connected){
                    //        //    //calculate_on_request_params(count_onreq_not_phone);
                    //        //    update_conn_params(PHONE_HANDLE, on_request_conn_params);
                    //        //    calculate_peri_to_relay_on_request_params(count_onreq_not_phone);
                    //        //    update_onreq_conn(h);
                    //        //}
                    //        //update_conn_params(h, on_request_conn_params);
                    //        //calibrate_conn_params(h, 10);
                    //    }
                    //}else{//RELAY
                        //New device connecting
                        remove_peri_from_scan_list(devices[h].name);

                        send_tlv_from_c(h, tlv_nameParams, 5, "Reply with name ID");
                        
                            
                            //send_usb_data(true, "Start conn update timer with ph wait params");
                            //conn_update_timer_params = conn_params_conn_phone_wait;//periodic_params;
                            //conn_update_timer_handle = h;
                            //conn_update_timer_start();
                        
                        //if(ble_conn_state_central_conn_count() < NRF_SDH_BLE_CENTRAL_LINK_COUNT)
                        //{   
                        //    //if(devices[h].nameNum < 6 && devices[h].nameNum != 255){
                        //        //m_target_periph_name[4] = (char) '1' + devices[h].nameNum;
                        //        //send_usb_data(false, "Try to scan for %s", m_target_periph_name);
                        //        //if(scan_filter_update(m_target_periph_name)){
                        //            // Resume scanning.
                        //            //bsp_board_led_on(CENTRAL_SCANNING_LED);


                        //            //start_scan_wait_conn_param_update = true;
                        //            //scan_wait_handle_update = h;
                        //            scan_start();


                        //            //send_usb_data(false, "Start to scan for %s", m_target_periph_name);
                        //        //}
                        //    //}
                        //    // Resume scanning.
                        //    //bsp_board_led_on(CENTRAL_SCANNING_LED);
                        //    //scan_start();
                        //}
                    //}
                    ret_code_t err_code = sd_ble_gap_rssi_start(h, 1, 0);
                    sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_CONN, h, tx_range[default_tx_idx]);
                    send_usb_data(true, "INIT Own TP to %s:%d to %d dBm", devices[h].name, h, devices[h].tp.tx_range[default_tx_idx]);
                    devices[h].tp.own_idx = default_tx_idx;
                    //change_conn_tx(tx_range[default_tx_idx], handle, (int8_t) devices[handle].tp.ema_rssi, (int8_t) devices[handle].tp.last_rssi);
                    tlv_rssiChange[0] = 36;
                    tlv_rssiChange[1] = default_tx_idx;
                    //tlv_rssiChange[2] = 0;
                    //tlv_rssiChange[3] = 0;
                    //if(i != devices[h].tp.requested_tx_idx && devices[h].tp.requested_tx_idx == -1){
                        //devices[h].tp.requested_tx_idx = i;
                        //if(h < NRF_SDH_BLE_CENTRAL_LINK_COUNT){
                            send_tlv_from_c(h, tlv_rssiChange, 2, "INIT TX pow");

                    //if(ble_conn_state_central_conn_count() <= NRF_SDH_BLE_CENTRAL_LINK_COUNT && conn_update_timer_handle != h)
                    //{   
                        send_usb_data(true, "Start conn update timer with ph wait params");
                        set_other_device_op_mode(h, 0);
                        ////if(!isPhone){
                        ////    conn_update_timer_params = devices[h].idle_params;//periodic_params;
                        ////}else{
                        ////    conn_update_timer_params = devices[h].periodic_params;
                        ////}

                        //if(devices[h].peri.type == 3){
                            //conn_update_timer_handle = h;
                            //start_scan_wait_conn_param_update = true;
                            ////update_conn_params(h, conn_params_conn_phone_wait);
                            //update_conn_params(h, devices[h].conn_params);

                            conn_update_timer_handle = h;
                            start_scan_wait_conn_param_update = true;
                            copy_conn_params(&conn_update_timer_params.params, &devices[h].conn_params);//devices[h].periodic_params;
                            
                            //memcpy(conn_update_timer_params.tlv, tlv_start_data, 2);
                            //conn_update_timer_params.len = 2;
                            //conn_update_timer_params.do_tlv = true;
                            conn_update_timer_start(700);

                        //    //copy_conn_params(&conn_update_timer_params.params, &devices[h].conn_params);//devices[h].periodic_params;
                        //    //conn_update_timer_handle = h;
                        //    //memcpy(conn_update_timer_params.tlv, tlv_start_data, 2);
                        //    //conn_update_timer_params.len = 2;
                        //    //conn_update_timer_params.do_tlv = true;
                        //    //conn_update_timer_start(1000);
                        //}else{
                        //    if(conn_update_timer_handle == 20){
                        //        copy_conn_params(&conn_update_timer_params.params, &devices[h].conn_params);//devices[h].periodic_params;
                        //        conn_update_timer_handle = h;
                        //        memcpy(conn_update_timer_params.tlv, empty_tlv, 0);
                        //        conn_update_timer_params.len = 0;
                        //        conn_update_timer_params.do_tlv = false;
                        //        conn_update_timer_start(600);
                        //    }else{
                        //        update_conn_params(h, devices[h].conn_params);
                        //    }
                        //}

                        //if(conn_update_timer_handle != 20){
                        //    copy_conn_params(&conn_update_timer_params.params, &devices[h].conn_params);//devices[h].periodic_params;
                        //    conn_update_timer_handle = h;
                        //    memcpy(conn_update_timer_params.tlv, empty_tlv, 1);
                        //    conn_update_timer_params.len = 0;
                        //    conn_update_timer_params.do_tlv = false;
                        //    conn_update_timer_start(1000);
                        //}else{
                        //    update_conn_params(h, devices[h].conn_params);
                        //}
                    //}
                    break;
            }
            else if(received_len == 2 && received_data[0] == 26){//RELAY
                if(!PERIS_DO_TP){
                    devices[h].tp.tx_idx = received_data[1];
                }
                devices[h].tp.other_tx_idx = received_data[1];
                devices[h].tp.requested_tx_idx = -1;
                if(devices[h].tp.received_counter != -1){
                    devices[h].tp.received_counter = 5; 
                    devices[h].tp.wait_avg_cnt = 5;
                    devices[h].tp.skip = 5;
                    //devices[h].tp.do_first_ema = true;
                    devices[h].tp.ema_rssi = 0;
                }
                //devices[h].tp.do_first_ema = true;
                send_usb_data(true, "INIT OTHERS %s:%d to ME to %d dBm, idx:%d", devices[h].name, h, devices[h].tp.tx_range[devices[h].tp.tx_idx], devices[h].tp.tx_idx);
                devices[h].tp.do_without_pdr = true;
            } 
            else if(received_len == 3 && received_data[0] == 91 && devices[h].connected){//RELAY
                int tx_idx = devices[h].tp.tx_idx;
                if(PERIS_DO_TP){
                    tx_idx = devices[h].tp.other_tx_idx;
                }

                if(tx_idx == received_data[1]){
                    return;
                }
                send_usb_data(true, "TIMEOUT INCREASE %s:%d to ME to %d dBm from %d dBm (old tp from peri = %d), idx:%d", devices[h].name, h, tx_range[received_data[1]], devices[h].tp.tx_range[devices[h].tp.tx_idx], tx_range[received_data[2]], devices[h].tp.tx_idx);
                //devices[h].tp.tx_idx = received_data[1];
                if(PERIS_DO_TP){
                    devices[h].tp.other_tx_idx = received_data[1];
                }
                devices[h].tp.requested_tx_idx = -1;
                if(!devices[h].tp.received_counter == -1){
                    devices[h].tp.received_counter = 3;
                    devices[h].tp.wait_avg_cnt = 3;
                }
                instruct_tp_change(h, received_data[1]);//devices[h].tp.tx_idx);
                devices[h].tp.tx_idx = received_data[1];
                //reset_pdr_timer(h);
                reset_tpc(h);
                //devices[h].tp.do_first_ema = true;
                
            }
            else if(received_len == 4){// FOR CHANGING CENT TP FROM PERI//RELAY
                int8_t rssi = 0;
                uint8_t channel = 0;
            

                ret_code_t err_code = sd_ble_gap_rssi_get(h, &rssi, &channel);
                
                if(received_data[0] == 16){
                    int8_t rec_tx = received_data[1];
                    if(devices[h].tp.own_idx != rec_tx){
                        //send_usb_data(true, "Got Request own TP to %s:%d to %d dBm from %d dBm; ema=%d, rssiLast=%d", devices[h].name, h, tx_range[rec_tx], tx_range[devices[h].tp.own_idx], (int8_t) received_data[2], (int8_t) received_data[3]);
                        change_conn_tx(tx_range[rec_tx], h, (int8_t) devices[h].tp.ema_rssi, (int8_t) devices[h].tp.last_rssi);
                        
                    }
                    break;
                }else if (received_data[0] == 17){//RELAY
                    int tx_idx = devices[h].tp.tx_idx;
                    if(PERIS_DO_TP){
                        tx_idx = devices[h].tp.other_tx_idx;
                    }
                    if (received_data[1] != tx_idx){
                        uint8_t tl[5] = {7, 0, devices[h].id, received_data[1], DEVICE_ID};//, devices[h].tp.own_idx, received_data[2], received_data[3]};
                        if(received_data[1] > tx_idx){
                            //send_usb_data(true, "%s:%d Increased TP to %d dBm from %d dBm; ema=%d, rssiLast=%d",devices[h].name, h, devices[h].tp.tx_range[received_data[1]], tx_range[tx_idx], (int8_t) received_data[2], (int8_t) received_data[3]);
                            
                            ////send_usb_data(true, "Here to %s, emaOld=%d, emaNew=%d",devices[h].name, devices[h].tp.ema_rssi, rssi);
                            ////devices[h].tp.ema_rssi = devices[h].tp.ema_rssi + abs(devices[h].tp.tx_range[received_data[1]] - devices[h].tp.tx_range[devices[h].tp.tx_idx]);
                            ////devices[h].tp.ema_rssi = (float) rssi * 1.0;
                            ////if(devices[h].tp.received_counter >= 3){
                            ////    devices[h].tp.received_counter -= 3;
                            ////}
                            if(WAIT_FOR_TP_CONFIRMATION && devices[h].tp.requested_tx_idx != -1){
                                devices[h].tp.received_counter = 0;//devices[h].tp.received_counter/2;
                            }
                        }
                        else if(received_data[1] < tx_idx){
                            //send_usb_data(true, "%s:%d Decreased TP to %d dBm from %d dBm; ema=%d, rssiLast=%d",devices[h].name, h, devices[h].tp.tx_range[received_data[1]], tx_range[tx_idx], (int8_t) received_data[2], (int8_t) received_data[3]);
                            
                            ////send_usb_data(true, "Here to %s, emaOld=%d, emaNew=%d",devices[h].name, devices[h].tp.ema_rssi, rssi);
                            ////devices[h].tp.ema_rssi = devices[h].tp.ema_rssi - abs(devices[h].tp.tx_range[received_data[1]] - devices[h].tp.tx_range[devices[h].tp.tx_idx]);
                            ////devices[h].tp.ema_rssi = (float) rssi * 1.0;
                            ////if(devices[h].tp.received_counter >= 3){
                            //    //devices[h].tp.received_counter -= 3;
                            ////}
                            if(WAIT_FOR_TP_CONFIRMATION && devices[h].tp.requested_tx_idx != -1){
                                devices[h].tp.received_counter = 0;//devices[h].tp.received_counter/2;
                            }
                        }
                        if(connectedToPhone && (sending_stored_tlv || stored_bytes_sent >= stored_bytes) && !devices[h].peri.askedAdvPhone){
                            send_tlv_from_p(PHONE_HANDLE, tl, 5); // never happens
                        }
                        devices[h].tp.tx_idx = received_data[1];
                        if(WAIT_FOR_TP_CONFIRMATION){
                            //devices[h].tp.tx_idx = received_data[1];
                            devices[h].tp.requested_tx_idx = -1;
                        }
                        devices[h].tp.other_tx_idx = received_data[1];
                    }
                    break;
                }
            }
            else if(received_len <= 3 && received_data[0] == 12){//RELAY
                uint8_t tlv_p[3] = {12, 12, devices[h].id};
                if(received_len == 1){ // start periodic
                    set_other_device_op_mode(h, 2);
                    devices[h].peri.chosenRelay = true;
                    update_conn_params(h, devices[h].conn_params);
                    //if(devices[PHONE_HANDLE].connected){
                    //    send_tlv_from_p(PHONE_HANDLE, tlv_p, 3);
                    //}
                }else if(received_len == 3 && received_data[1] == 12){ // NONONOdc to start periodic to rel, relay to phone
                    //int relay_h = h;
                    //if((char) received_data[2] != devices[h].id){//relay sends rel data
                        //devices[relay_h].is_relaying = true;
                        //h = find_idx_of_relayed((char) received_data[2]);
                        devices[h].relayed = true;
                        devices[h].peri.sending_periodic = true;
                        //devices[i].peri.sending_to_phone = true;
                        //devices[h].id = (char) received_data[2];
                        //devices[h].relay_h = relay_h;
                        //devices[h].peri.missed[1] = (char) received_data[2];
                    //}
                    if(!devices[h].peri.mode == 2){//!devices[h].peri.type==3){
                        set_other_device_op_mode(h, 2);
                        update_conn_params(h, devices[h].conn_params);
                    }
                    if(devices[PHONE_HANDLE].connected){
                        send_tlv_from_p(PHONE_HANDLE, received_data, received_len);
                    }
                }
                //else if(received_data[1] == 12){ // dc to start periodic to rel
                //    disconnect_handle(h);
                //}
                bool all_stored_finished = true;
                for(int i = 0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
                // check if there are any peris askAdvPhone not done yet
                    //if(!(devices[i].peri.sending_periodic || devices[i].peri.chosenRelay != DEVICE_ID))
                    if(!devices[i].peri.sending_periodic && devices[i].peri.chosenRelay)
                    {
                        all_stored_finished = false;
                        break;
                    }
                }
                if(all_stored_finished){
                    tlv_p[3] = DEVICE_ID;
                    send_tlv_from_p(PHONE_HANDLE, tlv_p, 3);
                }
            }
            else{//RELAY
                //#if RELAY_PERIS_CHOOSE
                //  if(received_len == 1){
                //      if(received_data[0] == 0){// Peri not using this relay anymore
                //          if(devices[h].peri.chosenRelay){
                //              devices[h].peri.chosenRelay = false;
                //              periodicRelayCounter--;
                //              devices[h].peri.last = -1;
                //              devices[h].peri.batch_number = 0;
                //          }
                //          return;
                //      }else if(received_data[0] == 1){ // Request to relay with this device, use it?
                //          return;
                //      }
                //  }
                //#endif

                if(received_batch == devices[h].peri.batch_number+1 && received_len > 5){
                    devices[h].peri.batch_number = received_batch;
                    devices[h].peri.last = -1;
                    devices[h].peri.missed[0] = 2;
                    devices[h].peri.missed[1] = 0;
                    devices[h].peri.received_all_count = 0;
                    //send_usb_data(true, "%s: GOT new batch %d, pref %d, last %d, wait %d", devices[h].name, received_batch,received_prefix, devices[h].peri.last, devices[h].peri.waiting_retransmit);
                }


                if(received_data[0] == 255 && received_data[1] == 255 
                          //&& received_data[4] == devices[h].id
                          && !devices[h].peri.allDataSentFromPeri 
                          && received_len == 5){
                    devices[h].peri.allDataSentFromPeri = true;
                    send_usb_data(true, "Handle:%d -> Got ALL DATA from %s",h,devices[h].name);

                    memcpy(package_storage[package_head].data, received_data,received_len);
                    package_storage[package_head].length = received_len;
                    package_head = (package_head+1)%package_storage_size;
                    package_count++;

                    //update_conn_params(h, power_saving_conn_params);
                    devices[h].peri.is_sending_on_req = false;
                    devices[h].tp.use_pdr = false;
                    //reset_device_peri(h, false);
                    break;
                }

                //if(received_batch!= 255){
                if(received_prefix > devices[h].peri.last){
                    //#if RELAY_PERIS_CHOOSE
                    //    if(!devices[h].peri.chosenRelay){
                    //        devices[h].peri.chosenRelay = true;

                    //    }
                    //#endif
                 
                    if(package_count >= package_storage_size - 15)
                    {   
                        if(devices[h].peri.stopped_pref == -1){
                            send_usb_data(true, "Handle:%d -> Error: Not enough space in global buffer to store received data. this Pref:%d, last:%d", h, received_prefix, devices[h].peri.last);
                            send_tlv_usb(false, "STOP TLV orig", tlv_stopData, 3,3,h);
                            if(devices[h].peri.last >= 0 && devices[h].peri.last <= 255){
                                tlv_stopData[2] = (uint8_t) devices[h].peri.last;
                                if(devices[h].peri.batch_number < 0){
                                    tlv_stopData[1] = 0;
                                }else{
                                    tlv_stopData[1] = devices[h].peri.batch_number;
                                }
                            }//else 
                            if(devices[h].peri.last < 0){
                                tlv_stopData[2] = 0;
                              if(devices[h].peri.batch_number < 0){
                                  tlv_stopData[1] = 0;
                              }
                            }
                            //tlv_stopData[1] = 0;
                            send_tlv_usb(false, "STOP TLV with last", tlv_stopData, 3,3,h);
                            tlv_stopData[0] = 4;
                            //tlv_stopData[1] = 0;
                            send_tlv_usb(false, "STOP TLV final", tlv_stopData, 3,3,h);
                            devices[h].peri.stopped = true;
                            devices[h].tp.use_pdr = false;
                            devices[h].tp.stopped_pdr = true;

                            send_tlv_from_c(h,tlv_stopData,(uint16_t) 3, "STOP");
                            there_are_stopped = true;
                            devices[h].peri.stopped_pref = received_prefix;
                        }else{
                            return;
                        }
                        //pause_chunks = true;
                    }
                    else
                    {   
                        if(devices[h].peri.stopped){
                            devices[h].tp.use_pdr = false;
                            if(devices[h].tp.stopped_pdr){
                                devices[h].tp.use_pdr = true;
                                devices[h].tp.stopped_pdr = false;
                                reset_pdr(h);
                            }
                        }

                        if(devices[h].tp.use_pdr){
                            devices[h].tp.cnt_pack[devices[h].tp.evt_head]++;
                            if(devices[h].tp.pdr_wait_count == -1){
                                devices[h].tp.pdr_wait_count = 0;
                            }
                        }

                        if(received_prefix % 50 == 0){
                            ////char* msg = get_time();
                            //send_usb_data(true, "%s sent up to B:%d, P:%d", devices[h].name, received_batch, received_prefix);
                            ////free(msg);
                            ////msg = NULL;
                        }
                        sending_relay_tlv = true;
                        //if(devices[h].peri.no_space >= received_prefix){
                        devices[h].peri.stopped_pref = -1;
                        //}
                        // Missed done in phone not relay
                        //if(received_prefix > (devices[h].peri.last+1) && !(received_batch != 255 && received_prefix != 255 && received_len == 5)){
                        //    for(uint8_t a = devices[h].peri.last+1; a<received_prefix; a++){
                        //        devices[h].peri.missed[devices[h].peri.missed[0]] = a;
                        //        devices[h].peri.missed[0]++;
                        //    }
                        //    //send_tlv_usb(false, "Added missed ", devices[h].peri.missed, devices[h].peri.missed[0],devices[h].peri.missed[0], h);
                        //}
                        // OPTIONAL: package circular buffer
                        //send_usb_data(false, "Handle:%d -> Received b:%d, p%d, l:%d", h,received_batch,received_prefix, received_len);
                        memcpy(package_storage[package_head].data, received_data,received_len);
                        package_storage[package_head].length = received_len;
                        //send_usb_data(false, "Handle:%d -> Received b:%d, p%d, l:%d, Head:%d", h,package_storage[package_head].data[0],package_storage[package_head].data[1], package_storage[package_head].length,package_head);
                        package_head = (package_head+1)%package_storage_size;
                        package_count++;
                        // OPTIONAL: save for usb
                        //save_tlv_as_string(received_data, received_len);

                        devices[h].peri.all_received[devices[h].peri.all_received_counter] = received_prefix;
                        devices[h].peri.all_received_counter++;

                        devices[h].peri.last = received_prefix;
                        devices[h].peri.batch_number = received_batch;
                        //OPTIONAL: send relay data
                        if(sending_periodic_tlv){
                            //CHOSEN_HANDLE = PHONE_HANDLE;
                            send_periodic_tlv();//(m_nus, PHONE_HANDLE); 
                        }
                        //for (uint8_t i=0; i<NRF_SDH_BLE_PERIPHERAL_LINK_COUNT; i++){
                        //  if(m_conn_handles[i] != BLE_CONN_HANDLE_INVALID){
                        //    //send_usb_data(false, "Sending Chunky NUS msg hndl:%d, idx:%d", m_conn_handles[i],i);
                        //    send_relay_tlv(m_nus, m_conn_handles[i]);
                        //  }
                        //}
                    }
                }

            }

            }//END IS RELAY
            else if(isPhone){//IS PHONE

            if(p_ble_nus_evt->data_len >=1){
                received_data = p_ble_nus_evt->p_data;
                received_len = p_ble_nus_evt->data_len;
                received_batch = received_data[0];
                if(p_ble_nus_evt->data_len >=2){
                    received_prefix = received_data[1];
                }
            }else{
                return;
            }
            if(!(received_len==5 && received_data[0] == 3)){
                if(received_len < 11){
                    if(received_len == devices[h].last_len){
                        if(memcmp(received_data,devices[h].last_rec,received_len) == 0){
                            return;
                        }
                    }
                    devices[h].last_len = received_len;
                    memcpy(devices[h].last_rec, received_data, received_len);
                }else{
                    devices[h].last_len = 0;
                }
            }
            if(received_len == 2 && received_data[0] == 199 && received_data[1]==199){
                for(int i = 0; i < NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
                    if(devices[i].connected){
                        disconnect_handle(i);
                        send_usb_data(false, "ENDDDDDDDDDDDDDDDDDDDDD");
                        bsp_board_led_on(led_BLUE);
                    }
                }
            }
            //send_usb_data(false, "conn_min=%d ms, mode = %d", devices[h].conn_params.min_conn_interval*1250/1000, devices[h].peri.mode);
            if(received_len<14){
                send_tlv_usb(true, "GOT to PHONE: ", p_ble_nus_evt->p_data, p_ble_nus_evt->data_len, p_ble_nus_evt->data_len, h);
            }
            //if(devices[h].curr_interval != 0 && !PERIS_DO_TP && !PERI_TP_TIMER){
            //    tp_timer_start(&devices[h]);
            //}
            if(received_data[0] == 255 && received_len >= 4 && received_data[1] == 255 && received_data[2] == 255 && received_data[3] == 255){
                send_tlv_usb(false, "USBerr", received_data,4,received_len,h);
                break;
            }
            else if(received_len <= 7 && received_data[0] == 8){// && devices[h].id == 'C'){//PHONE
                send_usb_data(false, "rec peris scan from %s(%c), is relay?=%d", devices[h].name, devices[h].id, devices[h].id == 'C');
                send_tlv_usb(true, "Rec Peris to scan for by Relay", received_data, 1, received_len, h);
                if(received_len == 1){
                    
                    //if(!start_scan_wait_conn_param_update){
                    if(!doStar){
                        remove_all_from_scan_list();
                        updated_phone_conn = true;
                        set_other_device_op_mode(h, 1);
                        send_usb_data(true, "did opmode");
                        updating_relay_conns = update_all_relay_conn_tlv(tlv_start_data, 2);//(fast_tlv, 1);
                        scan_stop();
                        no_scan = true;
                        bsp_board_led_on(led_GREEN);
                    }
                }else{
                    int scan_idx = 1;
                    relay_sent_peris = true;
                    remove_all_from_scan_list();
                    for(scan_idx; scan_idx < received_len; scan_idx++){
                        add_peri_to_scan_list(getNameFromId((char) received_data[scan_idx]));
                    }
                    scan_start();
                }
            }
            else if(received_len == 1 && received_data[0] == 11){//PHONE
                // All relayed peris are ready
                //if(!devices[h].peri.chosenRelay){
                    //devices[h].peri.chosenRelay = true;
                    if(!doStar && relay_sent_peris){
                        bsp_board_led_on(led_GREEN);
                        no_scan = true;
                        relay_sent_peris = false;
                        scan_stop();
                        set_other_device_op_mode(h,1);
                        updated_phone_conn = true;
                        updating_relay_conns = update_all_relay_conn_tlv(fast_tlv, 1);
                    }
                //}
            }
            //else if(received_len == 1 && received_data[0] == 12){
            //    // Peri will start sending periodic here
            //    if(!devices[h].peri.chosenRelay){
            //        devices[h].peri.chosenRelay = true;
            //        set_other_device_op_mode(h,2);
            //        update_conn_params(h, devices[h].conn_params);
            //    }
            //}
            else if (received_len==5 && received_data[0] == 3){// SENT NAME//PHONE
                    char name[6];
                    //send_tlv_usb(true, "Got peri name ", received_data, 2, 3, h);
                    if(received_data[1] == devices[h].id){
                        if(devices[h].connected){
                            return;
                        }else if(devices[h].peri.is_sending_on_req && !devices[h].peri.finished){
                            //DEVICE LOST CONNECTION MID SENDING
                            //TODO: send resume with last prefix
                            send_usb_data(false, "%s connected again after being disconnected mid sending", devices[h].name);
                        }
                    }

                    set_other_device_sim_params(h, received_data);
                    clear_updates_for_handle(h);
                    devices[h].peri.chosenRelay = true;
                    //if(devices[h].id != 255 && scanning_for_on_req && h == on_req_queue->handle){//not really used, for dc-ed device connecting again
                    //    send_usb_data(false, "In handle %d, old device is %s, new is %s, m_target=%s, strcmp=%d", h, devices[h].name, name, m_target_periph_name, strncmp(name, m_target_periph_name,5) == 0);
                    //    if(strncmp(devices[h].name, m_target_periph_name,5) == 0){
                    //        if(devices[h].peri.type == 1){
                    //            relay_info.disc_type1++;
                    //        }
                    //        if(devices[h].peri.is_sending_on_req){
                    //            send_tlv_from_c(h, tlv_nameParams, 3, "Reply with name ID");
                    //            if(devices[h].peri.batch_number == -1){
                    //                //tlv_resumeData[2] = 0;
                    //            }else{
                    //                //tlv_resumeData[2] = (uint8_t) devices[h].peri.batch_number;
                    //            }
                    //            if(devices[h].peri.last == -1){
                    //                //tlv_resumeData[3] = 0;
                    //            }else{
                    //                //tlv_resumeData[3] = (uint8_t) devices[h].peri.last;
                    //            }
                    //            char type[30];
                                
                    //            sprintf(type, "Restart on b:%d, p:%d", devices[h].peri.batch_number, devices[h].peri.last);
                    //            send_tlv_from_c(h, tlv_resumeData, 1, type);
                    //            //send_tlv_from_c(h, tlv_resumeData, 4, type);
                    //            //if(devices[PHONE_HANDLE].connected){
                    //            //    calculate_on_request_params(count_onreq_not_phone);
                    //            //    update_conn_params(PHONE_HANDLE, on_request_conn_params);
                    //            //    calculate_peri_to_relay_on_request_params(count_onreq_not_phone);
                    //            //    update_onreq_conn(h);
                    //            //}
                    //            ////update_conn_params(h, on_request_conn_params);
                    //            //calibrate_conn_params(h, 10);

                    //            //m_target_periph_names[devices[h].id-1] = "     ";
                    //            remove_peri_from_scan_list(devices[h].name);
                    //            send_usb_data(false, "Peri has name: %s, ID:%c, isPeriodic=%d, isRelay=%d", devices[h].name, devices[h].id,devices[h].peri.isPeriodic, devices[h].peri.isRelay);
                    //            //send_usb_data(false, "New scanning list is: %s %s %s %s %s %s", m_target_periph_names[0], m_target_periph_names[1], m_target_periph_names[2], m_target_periph_names[3], m_target_periph_names[4], m_target_periph_names[5]);
                    //        }else{
                    //            send_tlv_from_c(h, tlv_reqData, 2, "Sent start Sending req data");
                    //        }

                    //        scanning_for_on_req = false;
                    //        devices[h].peri.is_sending_on_req = true;
                    //        if(devices[PHONE_HANDLE].connected){
                    //            //calculate_on_request_params(count_onreq_not_phone);
                    //            update_conn_params(PHONE_HANDLE, on_request_conn_params);
                    //            calculate_peri_to_relay_on_request_params(count_onreq_not_phone);
                    //            update_onreq_conn(h);
                    //        }
                    //        //update_conn_params(h, on_request_conn_params);
                    //        //calibrate_conn_params(h, 10);
                    //    }
                    //}else{
                        //New device connecting
                        remove_peri_from_scan_list(devices[h].name);
                        send_tlv_from_c(h, tlv_nameParams, 3, "Reply with name ID");
                            //if(ble_conn_state_central_conn_count() <= NRF_SDH_BLE_CENTRAL_LINK_COUNT)
                            //{   
                                send_usb_data(true, "Start conn update timer with ph wait params, peri count=%d",ble_conn_state_central_conn_count());
                                //if(ble_conn_state_central_conn_count() <= NRF_SDH_BLE_CENTRAL_LINK_COUNT){
                                //    set_other_device_op_mode(h, 1);
                                //}else{
                                    //if(devices[h].peri.type == 2 || devices[h].peri.type == 3){
                                    //    set_other_device_op_mode(h, 1);
                                    //}else{
                                        set_other_device_op_mode(h, 4);
                                    //}
                                //}
                                //if(devices[h].peri.type == 3){
                                //    conn_update_timer_handle = h;
                                //    start_scan_wait_conn_param_update = true;
                                //    update_conn_params(h, conn_params_conn_phone_wait);
                                //    //update_conn_params_send_tlv(h, devices[h].conn_params, tlv_start_data, 2, true);

                                //    //copy_conn_params(&conn_update_timer_params.params, &devices[h].conn_params);//devices[h].periodic_params;
                                //    //conn_update_timer_handle = h;
                                //    //memcpy(conn_update_timer_params.tlv, tlv_start_data, 2);
                                //    //conn_update_timer_params.len = 2;
                                //    //conn_update_timer_params.do_tlv = true;
                                //    //conn_update_timer_start(1000);
                                //}else{
                                    //if(conn_update_timer_handle == 20){
                                        copy_conn_params(&conn_update_timer_params.params, &devices[h].conn_params);//devices[h].periodic_params;
                                        conn_update_timer_handle = h;
                                        //if(devices[h].id == 'C' && !doStar){
                                        //    uint8_t t[3] = {0, DEVICE_ID, DEVICE_ID};
                                        //    memcpy(conn_update_timer_params.tlv, t, 3);
                                        //    conn_update_timer_params.len = 3;
                                        //    conn_update_timer_params.do_tlv = true;
                                        //}else{
                                            memcpy(conn_update_timer_params.tlv, tlv_start_data, 2);
                                            conn_update_timer_params.len = 2;
                                            conn_update_timer_params.do_tlv = true;
                                        //}
                                        conn_update_timer_start(400);//1000);
                                    //}else{
                                    //    update_conn_params_send_tlv(h, devices[h].conn_params, tlv_start_data, 2, true);
                                    //}
                                //}
                            //}
                            //send_usb_data(true, "Start conn update timer with ph wait params");
                            //conn_update_timer_params = conn_params_conn_phone_wait;//periodic_params;
                            //conn_update_timer_handle = h;
                            //conn_update_timer_start();
                        //send_usb_data(true, "Do nothing. Set test rssi params.Peri count = %d", ble_conn_state_central_conn_count());
                            
                    //}
                    ret_code_t err_code = sd_ble_gap_rssi_start(h, 1, 0);
                    sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_CONN, h, tx_range[default_tx_idx]);
                    send_usb_data(true, "INIT Own TP to %s:%d to %d dBm", devices[h].name, h, devices[h].tp.tx_range[default_tx_idx]);
                    devices[h].tp.own_idx = default_tx_idx;
                    tlv_rssiChange[0] = 36;
                    tlv_rssiChange[1] = default_tx_idx;
                    send_tlv_from_c(h, tlv_rssiChange, 2, "INIT TX pow");
                    break;
            }
            if(received_len == 2 && received_data[0] == 199 && received_data[1]==199){
                for(int i = 0; i < NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
                    if(devices[i].connected){
                        disconnect_handle(i);
                        send_usb_data(false, "ENDDDDDDDDDDDDDDDDDDDDD");
                        bsp_board_led_on(led_BLUE);
                    }
                }
            }
            else if(received_len == 2 && received_data[0] == 26){//PHONE
                //if(!PERIS_DO_TP){
                    devices[h].tp.tx_idx = received_data[1];
                //}
                devices[h].tp.other_tx_idx = received_data[1];
                devices[h].tp.requested_tx_idx = -1;
                //if(devices[h].tp.received_counter != -1){
                    //devices[h].tp.received_counter = 5; 
                    //devices[h].tp.wait_avg_cnt = 5;
                    devices[h].tp.skip = 2;
                    //devices[h].tp.do_first_ema = true;
                    devices[h].tp.ema_rssi = devices[h].tp.ideal;
                //}
                //devices[h].tp.do_first_ema = true;
                
                send_usb_data(true, "INIT OTHERS %s:%d to ME to %d dBm, idx:%d", devices[h].name, h, devices[h].tp.tx_range[devices[h].tp.tx_idx], devices[h].tp.tx_idx);
                //devices[h].tp.do_without_pdr = true;
            } else if(received_len == 3 && received_data[0]==74){
                 send_usb_data(true, "%s finished og stored.",devices[h].name); 
            }
            else if(received_len == 3 && received_data[0] == 91 && devices[h].connected){ //PHONE
                int tx_idx = devices[h].tp.tx_idx;
                if(PERIS_DO_TP){
                    tx_idx = devices[h].tp.other_tx_idx;
                }

                if(tx_idx == received_data[1]){
                    return;
                }
                send_usb_data(true, "TIMEOUT INCREASE %s:%d to ME to %d dBm from %d dBm (old tp from peri = %d), idx:%d", devices[h].name, h, tx_range[received_data[1]], devices[h].tp.tx_range[devices[h].tp.tx_idx], tx_range[received_data[2]], devices[h].tp.tx_idx);
                //devices[h].tp.tx_idx = received_data[1];
                if(PERIS_DO_TP){
                    devices[h].tp.other_tx_idx = received_data[1];
                }
                devices[h].tp.requested_tx_idx = -1;
                if(!devices[h].tp.received_counter == -1){
                    devices[h].tp.received_counter = 3;
                    devices[h].tp.wait_avg_cnt = 3;
                }
                instruct_tp_change(h, received_data[1]);//devices[h].tp.tx_idx);
                devices[h].tp.tx_idx = received_data[1];
                //reset_pdr_timer(h);
                reset_tpc(h);
                //devices[h].tp.do_first_ema = true;
                
            }else if(received_len <= 3 && received_data[0] == 12){//PHONE
                if(received_len == 1){ // start periodic
                    if(((devices[h].id == 'C' && doStar) || devices[h].id != 'C')){// && !devices[h].peri.type==3){
                        set_other_device_op_mode(h, 2);
                        update_conn_params(h, devices[h].conn_params);
                    }else{
                        devices[h].tp.use_pdr = false;
                        //devices[h].peri.sending_periodic = true;
                    }
                }else if(received_len == 2 && received_data[1] == 12){ // dc to start periodic to rel
                    disconnect_handle(h);
                }else if(received_len == 3 && received_data[1] == 12){ // dc to start periodic to rel
                    int relay_h = h;
                    if((char) received_data[2] != devices[h].id){//relay sends rel data
                        devices[relay_h].is_relaying = true;
                        h = find_idx_of_relayed((char) received_data[2]);
                        devices[h].relayed = true;
                        devices[h].peri.sending_periodic = true;
                        devices[h].id = (char) received_data[2];
                        devices[h].relay_h = relay_h;
                        devices[h].peri.missed[1] = (char) received_data[2];
                    }else{
                        devices[h].peri.sending_periodic = true;
                        set_other_device_op_mode(relay_h, 2);
                        update_conn_params(relay_h, devices[relay_h].conn_params);
                    }
                    
                }
            }
            //else if(devices[h].peri.isPeriodic){
            //    if(received_len == 2 && received_data[0] == 0){// && received_data[1] == 0
            //        // Peri not using this relay anymore
            //        if(devices[h].peri.chosenRelay){
            //            devices[h].peri.chosenRelay = false;
            //            periodicRelayCounter--;
            //            devices[h].peri.last = -1;
            //            devices[h].peri.batch_number = 0;
            //        }
            //        return;
            //    }
            //    if((received_prefix > devices[h].peri.last && received_batch == devices[h].peri.batch_number) || received_batch == (devices[h].peri.batch_number + 1)%255){
            //        // Peri sent Periodic data
            //        if(!devices[h].peri.chosenRelay){
            //            devices[h].peri.chosenRelay = true;
            //            periodicRelayCounter++;
            //        }
            //        memcpy(package_storage[package_head].data, received_data,received_len);
            //        package_storage[package_head].length = received_len;
            //        package_head = (package_head+1)%package_storage_size;
            //        package_count++;

            //        devices[h].peri.last = received_prefix;
            //        devices[h].peri.batch_number = received_batch;

            //        if(sending_relay_tlv){
            //            //sending_relay_tlv = true;
            //            //CHOSEN_HANDLE = PHONE_HANDLE;
            //            send_periodic_tlv();//(m_nus, PHONE_HANDLE);
            //        }
            //    }
            //}
            else{//data packet//PHONE
                int check;
                int relay_h = h;
                bool relayed = false;
                if((char) received_data[2] != devices[h].id){//relay sends rel data
                    devices[relay_h].is_relaying = true;
                    h = find_idx_of_relayed((char) received_data[2]);
                    if(devices[h].id == 255){
                        uint8_t kot[5] = {3, received_data[2], 0,0,0};
                        set_other_device_sim_params(h, kot);
                    }
                    devices[h].relayed = true;
                    devices[h].id = (char) received_data[2];
                    devices[h].relay_h = relay_h;
                    relayed = true;
                    devices[h].peri.missed[1] = (char) received_data[2];
                    //devices[h].peri.missed[0] = 10;
                    //devices[h].peri.missed[1] = h;
                    //if(devices[h].peri.miss_ctr == 0){
                    //    devices[h].peri.miss_b_idx = 2;
                    //    devices[h].peri.new_miss_idx = 2;
                    //    devices[h].peri.miss_idx = devices[h].peri.miss_b_idx + 2; 
                    //}
                }
                if(received_data[0] == 255 && received_data[1] == 255 
                          //&& received_data[4] == devices[h].id
                          && !devices[h].peri.allDataSentFromPeri 
                          && received_len == 5){
                    
                    
                    devices[h].peri.final_batch = received_data[3];
                    devices[h].peri.final_pref = received_data[4];
                    //if(devices[h].peri.last == devices[h].peri.final_pref){
                        devices[h].peri.allDataSentFromPeri = true;
                    uint8_t no_miss[4] = {10, received_data[2], 255, 255};
                    send_usb_data(false, "%s finished sending stored");
                    send_tlv_from_c(relay_h, no_miss, 2, "No missed prefs");
                    //} 
                    //check = checkMissed(h, devices[h].peri.final_pref, devices[h].peri.final_batch, true);

                    //if(devices[h].peri.miss_idx > 4){
                    //    if(check >= MISSED && devices[h].peri.missed[devices[h].peri.miss_idx] == devices[h].peri.final_batch && devices[h].peri.missed[devices[h].peri.miss_idx+1] >= devices[h].peri.final_batch){
                    //        devices[h].peri.missed[devices[h].peri.miss_idx] = 255;
                    //        devices[h].peri.missed[devices[h].peri.miss_idx+1] = 255;
                    //    }
                    //    devices[h].peri.missed[0] = 10;
                    //    devices[h].peri.missed[1] = devices[h].id;
                    //    //first_packet_timer_start(&devices[h]);
                    //    send_tlv_from_c(relay_h, devices[h].peri.missed, devices[h].peri.miss_idx + 2, "sending missed prefs");
                    //    for(int l=devices[h].peri.miss_idx; l < 256; l++){
                    //        devices[h].peri.missed[l] = 0;
                    //    }

                    //    devices[h].peri.sending_missed = true;
                    //    devices[h].peri.miss_b_idx = 2;
                    //    devices[h].peri.new_miss_idx = 2;
                    //    if(devices[h].tp.use_pdr){
                    //        devices[h].tp.use_pdr = false;
                    //        devices[h].tp.stopped_pdr = true;
                    //    }
                    //    devices[h].peri.missed[0] = 255;
                    //}else{
                    //    memset(devices[h].peri.missed, 0, 256);//reset missed
                    //    devices[h].peri.missed[0] = 10;
                    //    devices[h].peri.missed[1] = devices[h].id;
                    //    //first_packet_timer_start(&devices[h]);
                    //    send_tlv_from_c(relay_h, devices[h].peri.missed, 2, "No missed prefs");
                    //}
                }

                //if(devices[h].peri.sending_missed && !devices[h].peri.miss_sent){
                //    int b_idx = devices[h].peri.miss_b_idx;
                //    int idx = b_idx+2;
                //    int m_idx;
                //    bool check_all_missed = (received_data[0] == 254 && received_data[1] == 255);
                //    bool finished_sending_missed = check_all_missed;
                //    if(received_len > 3 && (received_prefix > devices[h].peri.last || received_batch > devices[h].peri.batch_number)){
                //        return;
                //    }
                    
                //    while(true){
                //        if(devices[h].peri.missed[0] == 255){
                //            devices[h].peri.missed[0] = devices[h].peri.missed[b_idx+1];
                //        }
                //        if(devices[h].peri.missed[b_idx] == received_batch && !finished_sending_missed){
                //            for(idx; idx <= b_idx + 1 + devices[h].peri.missed[b_idx+1]; idx++){
                //                if(devices[h].peri.missed[idx] == received_prefix){
                //                    devices[h].peri.missed[idx] = 255;
                //                    devices[h].peri.missed[0]--;
                //                    //if(devices[h].peri.missed[0] == 0){
                //                    //    devices[h].peri.missed[b_idx+1] = 0;
                //                    //}
                //                    if(idx == devices[h].peri.miss_idx -1){//all missed array parsed
                //                        //devices[h].peri.missed[0] = 10;
                //                        devices[h].peri.missed[1] = h;
                //                        // check if all missed received and send
                //                        check_all_missed = true;
                //                        break;
                //                    }else{
                //                        return;
                //                    }
                //                }
                //            }
                //        }
                        
                //        if(devices[h].peri.missed[b_idx] < received_batch || check_all_missed){
                //            m_idx = devices[h].peri.new_miss_idx;
                //            if(devices[h].peri.missed[0] > 1){ // there are still missed packets in current batch
                //                devices[h].peri.missed[m_idx] = devices[h].peri.missed[b_idx];
                //                devices[h].peri.missed[m_idx+1] = devices[h].peri.missed[0];
                //                m_idx += 2;
                                
                //                for(int k = m_idx; k <= b_idx + 1 + devices[h].peri.missed[b_idx+1]; k++){ // copy left over missed from batch
                //                    if(devices[h].peri.missed[k] != 255){
                //                        devices[h].peri.missed[m_idx] = devices[h].peri.missed[k];
                //                        m_idx++;
                //                    }
                //                }
                //                devices[h].peri.new_miss_idx = m_idx;
                //            }
                //            b_idx += 2 + devices[h].peri.missed[b_idx+1]; 

                //            if(devices[h].peri.missed[b_idx] == 0 || check_all_missed){// no other missed batch
                //                if(finished_sending_missed){
                //                    devices[h].peri.miss_sent = false;
                //                    if(m_idx == 2){//rec all missed
                //                        memset(devices[h].peri.missed, 0, 256);//reset missed
                //                        devices[h].peri.missed[0] = 10;
                //                        devices[h].peri.missed[1] = devices[h].id;
                //                        //first_packet_timer_start(&devices[h]);
                //                        send_tlv_from_c(relay_h, devices[h].peri.missed, 2, "received all missed prefs");

                //                        devices[h].peri.sending_missed = false;
                //                        devices[h].peri.miss_idx = 4;

                //                        if(devices[h].tp.stopped_pdr){
                //                            devices[h].tp.use_pdr = true;
                //                            devices[h].tp.stopped_pdr = false;
                //                            reset_pdr(h);
                //                        }
                                   
                //                    }else{//resend left over missed
                //                        devices[h].peri.missed[0] = 10;
                //                        devices[h].peri.missed[m_idx] = devices[h].peri.missed[devices[h].peri.miss_idx];
                //                        devices[h].peri.missed[m_idx+1] = devices[h].peri.missed[devices[h].peri.miss_idx+1];
                //                        devices[h].peri.missed[1] = devices[h].id;
                //                        //first_packet_timer_start(&devices[h]);
                //                        send_tlv_from_c(relay_h, devices[h].peri.missed, m_idx+2, "send left over missed prefs");
                                    
                //                        for(int l=m_idx; l < 256; l++){
                //                            devices[h].peri.missed[l] = 0;
                //                        }
                //                        devices[h].peri.missed[0] = 255;
                //                        devices[h].peri.miss_idx = m_idx; 
                //                    }
                //                    devices[h].peri.miss_b_idx = 2;
                //                    devices[h].peri.new_miss_idx = 2;
                //                }
                //                return;
                //            }else{// continue with next batch of missed
                //                devices[h].peri.miss_b_idx = b_idx;
                //                idx = b_idx + 2;
                //                devices[h].peri.missed[0] = 255;
                //            }
                //        }else{
                //            return;
                //        }
                //    }
                //}
                //else if (devices[h].peri.sending_missed && devices[h].peri.miss_ready && devices[h].peri.miss_ctr_extra != devices[h].peri.miss_idx){
                //    if(devices[h].peri.missed_extra[devices[h].peri.miss_ctr_extra-1] != received_prefix){
                //        devices[h].peri.missed_extra[devices[h].peri.miss_ctr_extra] = received_prefix;
                //        devices[h].peri.miss_ctr_extra++;
                //    }
                //    return;
                //}
                
                check = checkMissed(h, received_prefix, received_batch, false);
                if(check != ALREADY_REC){
                    if(received_prefix % 50 == 0){
                        send_usb_data(false, "%s Sent to b:%d, pref%d", devices[h].name, received_batch,received_prefix);
                    }
                    if(devices[relay_h].tp.use_pdr){
                        devices[relay_h].tp.cnt_pack[devices[relay_h].tp.evt_head]++;
                        //if(devices[relay_h].tp.cnt_pack[devices[relay_h].tp.evt_head] >=2){
                        //    send_usb_data(false, "Doing pdr in idx:%d, count=%d, pdr=%s",devices[relay_h].tp.evt_head,devices[relay_h].tp.cnt_pack[devices[relay_h].tp.evt_head], devices[relay_h].tp.pdr_str);
                        //}
                        if(devices[relay_h].tp.pdr_wait_count == -1){
                            devices[relay_h].tp.pdr_wait_count = 0;
                        }
                    }
                }
                //if(!devices[h].peri.sending_periodic){
                //    if(check == MISSED_FULL){
                //        devices[h].peri.missed[0] = 10;
                //        devices[h].peri.missed[1] = devices[h].id;
                //        //first_packet_timer_start(&devices[h]);
                //        send_tlv_from_c(relay_h, devices[h].peri.missed, devices[h].peri.miss_idx+2, "send left over missed prefs");
                //        for(int l=devices[h].peri.miss_idx; l < 256; l++){
                //            devices[h].peri.missed[l] = 0;
                //        }

                //        devices[h].peri.sending_missed = true;
                //        devices[h].peri.miss_b_idx = 2;
                //        devices[h].peri.new_miss_idx = 2;
                //        if(devices[h].tp.use_pdr){
                //            devices[h].tp.use_pdr = false;
                //            devices[h].tp.stopped_pdr = true;
                //        }
                //        devices[h].peri.missed[0] = 255;
                //    }
                //}
                
                ////if(received_batch!= 255){
                //if(received_prefix > devices[h].peri.last && !devices[h].peri.sending_missed){
                //        //if(devices[h].peri.stopped){// not used for phone
                //        //    devices[h].tp.use_pdr = false;
                //        //    if(devices[h].tp.stopped_pdr){
                //        //        devices[h].tp.use_pdr = true;
                //        //        devices[h].tp.stopped_pdr = false;
                //        //        reset_pdr(h);
                //        //    }
                //        //}

                //        if(devices[relay_h].tp.use_pdr){
                //            devices[relay_h].tp.cnt_pack[devices[relay_h].tp.evt_head]++;
                //            if(devices[relay_h].tp.pdr_wait_count == -1){
                //                devices[relay_h].tp.pdr_wait_count = 0;
                //            }
                //        }

                //        //if(received_prefix % 50 == 0){
                //        //    ////char* msg = get_time();
                //        //    send_usb_data(true, "%s sent up to B:%d, P:%d", devices[h].name, received_batch, received_prefix);
                //        //    ////free(msg);
                //        //    ////msg = NULL;
                //        //}

                //        //sending_relay_tlv = true;
                //        //if(devices[h].peri.no_space >= received_prefix){
                //        devices[h].peri.stopped_pref = -1;
                //        //}
                //        if(received_prefix > (devices[h].peri.last+1) && !(received_batch != 255 && received_prefix != 255 && received_len == 5)){
                //            if(devices[h].peri.missed[devices[h].peri.miss_b_idx + 1] == 0){
                //                devices[h].peri.missed[devices[h].peri.miss_b_idx] = received_batch;
                //            }
                //            for(uint8_t a = devices[h].peri.last+1; a<received_prefix; a++){
                //                devices[h].peri.missed[devices[h].peri.miss_idx] = a;
                //                devices[h].peri.missed[devices[h].peri.miss_b_idx+1]++;
                                
                //                devices[h].peri.miss_idx++;

                //                if(devices[h].peri.miss_idx == 242){
                //                    if(a == received_prefix - 1){
                //                        devices[h].peri.missed[243] = devices[h].peri.last;
                //                    }else{
                //                        devices[h].peri.missed[243] = a+1;
                //                    }
                //                    devices[h].peri.missed[242] = devices[h].peri.batch_number;
                //                    send_tlv_from_c(relay_h, devices[h].peri.missed, 244, "sending missed prefs");
                //                    devices[h].peri.sending_missed = true;
                //                    devices[h].peri.miss_b_idx = 2;
                //                    devices[h].peri.new_miss_idx = 2;
                //                    if(devices[h].tp.use_pdr){
                //                        devices[h].tp.use_pdr = false;
                //                        devices[h].tp.stopped_pdr = true;
                //                    }
                //                    devices[h].peri.missed[0] = 255;
                //                }
                //            }
                //            //send_tlv_usb(false, "Added missed ", devices[h].peri.missed, devices[h].peri.missed[0],devices[h].peri.missed[0], h);
                //        }
                //}

            }


            }//end for phone
        }break;

        case BLE_NUS_C_EVT_DISCONNECTED:
            //send_usb_data(true, "Disconnected NUS from %s.",devices[p_ble_nus_evt->conn_handle].name);
            if(devices[p_ble_nus_evt->conn_handle].id!=255){
                //if(devices[p_ble_nus_evt->conn_handle].peri.chosenRelay){
                //    periodicRelayCounter--;
                //}
                //send_usb_data(false, "Erased handle %d info, name:%s", p_ble_nus_evt->conn_handle, devices[p_ble_nus_evt->conn_handle].name);
                //reset_device_full(p_ble_nus_evt->conn_handle);
            }
            //reset_device_full(p_ble_nus_evt->conn_handle);
            //scan_start();
            break;
    }
}
#endif // NRF_MODULE_ENABLED(BLE_NUS_C)
