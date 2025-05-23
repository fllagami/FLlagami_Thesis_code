#ifndef BLE_NUS_C2_H__
#define BLE_NUS_C2_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_gatt.h"
#include "ble_db_discovery.h"
#include "ble_srv_common.h"
#include "nrf_ble_gq.h"
#include "nrf_sdh_ble.h"
//#include "common.h"
#include "sdk_config.h"
#include "ble_nus_c_common_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**@brief   Macro for defining a ble_nus_c instance.
 *
 * @param   _name   Name of the instance.
 * @hideinitializer
 */
#define BLE_NUS_C_DEF(_name)                                                                        \
static ble_nus_c_t _name;                                                                           \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                                                                 \
                     BLE_NUS_C_BLE_OBSERVER_PRIO,                                                   \
                     ble_nus_c_on_ble_evt, &_name)

/** @brief Macro for defining multiple ble_nus_c instances.
 *
 * @param   _name   Name of the array of instances.
 * @param   _cnt    Number of instances to define.
 * @hideinitializer
 */
#define BLE_NUS_C_ARRAY_DEF(_name, _cnt)                 \
static ble_nus_c_t _name[_cnt];                          \
NRF_SDH_BLE_OBSERVERS(_name ## _obs,                     \
                      BLE_NUS_C_BLE_OBSERVER_PRIO,       \
                      ble_nus_c_on_ble_evt, &_name, _cnt)


/**@brief     Function for initializing the Nordic UART client module.
 *
 * @details   This function registers with the Database Discovery module
 *            for the NUS. The Database Discovery module looks for the presence
 *            of a NUS instance at the peer when a discovery is started.
 *            
 * @param[in] p_ble_nus_c      Pointer to the NUS client structure.
 * @param[in] p_ble_nus_c_init Pointer to the NUS initialization structure that contains the
 *                             initialization information.
 *
 * @retval    NRF_SUCCESS If the module was initialized successfully.
 * @retval    err_code    Otherwise, this function propagates the error code
 *                        returned by the Database Discovery module API
 *                        @ref ble_db_discovery_evt_register.
 */
uint32_t ble_nus_c_init(ble_nus_c_t * p_ble_nus_c, ble_nus_c_init_t * p_ble_nus_c_init);


/**@brief Function for handling events from the Database Discovery module.
 *
 * @details This function handles an event from the Database Discovery module, and determines
 *          whether it relates to the discovery of NUS at the peer. If it does, the function
 *          calls the application's event handler to indicate that NUS was
 *          discovered at the peer. The function also populates the event with service-related
 *          information before providing it to the application.
 *
 * @param[in] p_ble_nus_c Pointer to the NUS client structure.
 * @param[in] p_evt       Pointer to the event received from the Database Discovery module.
 */
 void ble_nus_c_on_db_disc_evt(ble_nus_c_t * p_ble_nus_c, ble_db_discovery_evt_t * p_evt);


/**@brief     Function for handling BLE events from the SoftDevice.
 *
 * @details   This function handles the BLE events received from the SoftDevice. If a BLE
 *            event is relevant to the NUS module, the function uses the event's data to update
 *            internal variables and, if necessary, send events to the application.
 *
 * @param[in] p_ble_evt     Pointer to the BLE event.
 * @param[in] p_context     Pointer to the NUS client structure.
 */
void ble_nus_c_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);


/**@brief   Function for requesting the peer to start sending notification of TX characteristic.
 *
 * @details This function enables notifications of the NUS TX characteristic at the peer
 *          by writing to the CCCD of the NUS TX characteristic.
 *
 * @param   p_ble_nus_c Pointer to the NUS client structure.
 *
 * @retval  NRF_SUCCESS If the operation was successful. 
 * @retval  err_code 	Otherwise, this API propagates the error code returned by function @ref nrf_ble_gq_item_add.
 */
uint32_t ble_nus_c_tx_notif_enable(ble_nus_c_t * p_ble_nus_c);


/**@brief Function for sending a string to the server.
 *
 * @details This function writes the RX characteristic of the server.
 *
 * @param[in] p_ble_nus_c Pointer to the NUS client structure.
 * @param[in] p_string    String to be sent.
 * @param[in] length      Length of the string.
 *
 * @retval NRF_SUCCESS If the string was sent successfully. 
 * @retval err_code    Otherwise, this API propagates the error code returned by function @ref nrf_ble_gq_item_add.
 */
uint32_t ble_nus_c_string_send(ble_nus_c_t * p_ble_nus_c, uint8_t * p_string, uint16_t length);


/**@brief Function for assigning handles to this instance of nus_c.
 *
 * @details Call this function when a link has been established with a peer to
 *          associate the link to this instance of the module. This makes it
 *          possible to handle several links and associate each link to a particular
 *          instance of this module. The connection handle and attribute handles are
 *          provided from the discovery event @ref BLE_NUS_C_EVT_DISCOVERY_COMPLETE.
 *
 * @param[in] p_ble_nus_c    Pointer to the NUS client structure instance to associate with these
 *                           handles.
 * @param[in] conn_handle    Connection handle to associated with the given NUS Instance.
 * @param[in] p_peer_handles Attribute handles on the NUS server that you want this NUS client to
 *                           interact with.
 *
 * @retval    NRF_SUCCESS    If the operation was successful.
 * @retval    NRF_ERROR_NULL If a p_nus was a NULL pointer.
 * @retval    err_code       Otherwise, this API propagates the error code returned 
 *                           by function @ref nrf_ble_gq_item_add.
 */
uint32_t ble_nus_c_handles_assign(ble_nus_c_t *               p_ble_nus_c,
                                  uint16_t                    conn_handle,
                                  ble_nus_c_handles_t const * p_peer_handles);

// nus handler
void ble_nus_c_evt_handler(ble_nus_c_t * p_ble_nus_c, ble_nus_c_evt_t const * p_ble_nus_evt);

// Sending functions
//int send_tlv_from_c(int h, bool rssi, uint8_t* tlv, uint16_t length, char* type);

// Queue tlv messages to centrals
typedef struct {
    uint8_t data[244];
    uint16_t length;
    uint16_t handle;
    char type[300];
}peri_tlv_storage_t;

extern int peri_tlv_head;
extern int peri_tlv_tail;

extern int peri_tlv_max_size;
extern int peri_tlv_storage_size;
extern peri_tlv_storage_t peri_tlv_storage[];

#ifdef __cplusplus
}
#endif

#endif // BLE_NUS_C2_H__

/** @} */
