#ifndef BLE_NUS_C_COMMON_TYPES
#define BLE_NUS_C_COMMON_TYPES

#include <stdint.h>
#include <stdbool.h>
//#include "ble.h"
//#include "ble_gatt.h"
//#include "ble_db_discovery.h"
#include "ble_srv_common.h"
#include "nrf_ble_gq.h"
//#include "nrf_sdh_ble.h"
//#include "common.h"
//#include "sdk_config.h"

/**@brief   Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */
//#if defined(NRF_SDH_BLE_GATT_MAX_MTU_SIZE) && (NRF_SDH_BLE_GATT_MAX_MTU_SIZE != 0)
//    #define BLE_NUS_MAX_DATA_LEN (NRF_SDH_BLE_GATT_MAX_MTU_SIZE - OPCODE_LENGTH - HANDLE_LENGTH)
//#else
//    #define BLE_NUS_MAX_DATA_LEN (BLE_GATT_MTU_SIZE_DEFAULT - OPCODE_LENGTH - HANDLE_LENGTH)
//    #warning NRF_SDH_BLE_GATT_MAX_MTU_SIZE is not defined.
//#endif


/**@brief NUS Client event type. */
typedef enum
{
    BLE_NUS_C_EVT_DISCOVERY_COMPLETE,   /**< Event indicating that the NUS service and its characteristics were found. */
    BLE_NUS_C_EVT_NUS_TX_EVT,           /**< Event indicating that the central received something from a peer. */
    BLE_NUS_C_EVT_DISCONNECTED          /**< Event indicating that the NUS server disconnected. */
} ble_nus_c_evt_type_t;

/**@brief Handles on the connected peer device needed to interact with it. */
typedef struct
{
    uint16_t nus_tx_handle;      /**< Handle of the NUS TX characteristic, as provided by a discovery. */
    uint16_t nus_tx_cccd_handle; /**< Handle of the CCCD of the NUS TX characteristic, as provided by a discovery. */
    uint16_t nus_rx_handle;      /**< Handle of the NUS RX characteristic, as provided by a discovery. */
} ble_nus_c_handles_t;

/**@brief Structure containing the NUS event data received from the peer. */
typedef struct
{
    ble_nus_c_evt_type_t evt_type;
    uint16_t             conn_handle;
    uint16_t             max_data_len;
    uint8_t            * p_data;
    uint16_t             data_len;
    ble_nus_c_handles_t  handles;     /**< Handles on which the Nordic UART service characteristics were discovered on the peer device. This is filled if the evt_type is @ref BLE_NUS_C_EVT_DISCOVERY_COMPLETE.*/
} ble_nus_c_evt_t;

// Forward declaration of the ble_nus_t type.
typedef struct ble_nus_c_s ble_nus_c_t;

/**@brief   Event handler type.
 *
 * @details This is the type of the event handler that is to be provided by the application
 *          of this module to receive events.
 */
typedef void (* ble_nus_c_evt_handler_t)(ble_nus_c_t * p_ble_nus_c, ble_nus_c_evt_t const * p_evt);

/**@brief NUS Client structure. */
struct ble_nus_c_s
{
    uint8_t                   uuid_type;      /**< UUID type. */
    uint16_t                  conn_handle;    /**< Handle of the current connection. Set with @ref ble_nus_c_handles_assign when connected. */
    ble_nus_c_handles_t       handles;        /**< Handles on the connected peer device needed to interact with it. */
    ble_nus_c_evt_handler_t   evt_handler;    /**< Application event handler to be called when there is an event related to the NUS. */
    ble_srv_error_handler_t   error_handler;  /**< Function to be called in case of an error. */
    nrf_ble_gq_t            * p_gatt_queue;   /**< Pointer to BLE GATT Queue instance. */
};

/**@brief NUS Client initialization structure. */
typedef struct
{
    ble_nus_c_evt_handler_t   evt_handler;    /**< Application event handler to be called when there is an event related to the NUS. */
    ble_srv_error_handler_t   error_handler;  /**< Function to be called in case of an error. */
    nrf_ble_gq_t            * p_gatt_queue;   /**< Pointer to BLE GATT Queue instance. */
} ble_nus_c_init_t;

#endif // BLE_NUS_C_COMMON_TYPES