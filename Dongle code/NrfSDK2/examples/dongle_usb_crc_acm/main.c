#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_timer.h"
#include "bsp_btn_ble.h"
#include "nordic_common.h"
//#include "nrf.h"

#include "app_error.h"
//#include "ble.h"
//#include "ble_err.h"
//#include "ble_hci.h"
//#include "ble_srv_common.h"
//#include "ble_advdata.h"
//#include "ble_advertising.h"
//#include "ble_bas.h"
//#include "ble_hrs.h"
//#include "ble_dis.h"
//#include "ble_conn_params.h"
//#include "sensorsim.h"
//#include "nrf_sdh.h"
//#include "nrf_sdh_ble.h"
//#include "nrf_sdh_soc.h"

//#include "bsp_btn_ble.h"
//#include "peer_manager.h"
//#include "peer_manager_handler.h"
//#include "fds.h"
//#include "nrf_ble_gatt.h"
//#include "nrf_ble_lesc.h"
//#include "nrf_ble_qwr.h"
//#include "ble_conn_state.h"
//#include "nrf_pwr_mgmt.h"

//#include "nrf_drv_usbd.h"
#include "nrf_drv_clock.h"
//#include "nrf_gpio.h"
//#include "nrf_delay.h"
//#include "nrf_drv_power.h"
//#include "app_util.h"
//#include "app_usbd_core.h"
#include "app_usbd.h"
//#include "app_usbd_string_desc.h"
#include "app_usbd_cdc_acm.h"
#include "app_usbd_serial_num.h"

#include "boards.h"
#include "bsp.h"
//#include "bsp_cli.h"
//#include "nrf_cli.h"
//#include "nrf_cli_uart.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
//#include "nrf_log_default_backends.h"
#include "usb_handler.h"
//#include "hrs_handler.h"
#include "common.h"
#include "nus_handler.h"


#if NRF_CLI_ENABLED
/**
 * @brief CLI interface over UART
 */
NRF_CLI_UART_DEF(m_cli_uart_transport, 0, 64, 16);
NRF_CLI_DEF(m_cli_uart,
            "uart_cli:~$ ",
            &m_cli_uart_transport.transport,
            '\r',
            4);
#endif

//asdasdasd
static int read_int = -1;
static int rec_number;
//size_t file_size;
static char back[READ_SIZE];

//char file_data[MAX_FILE_SIZE];
//char m_rx_buffer[READ_SIZE];
//static char rec_buffer[READ_SIZE];
//char m_tx_buffer[NRF_DRV_USBD_EPSIZE];
//bool m_data_ready = false;  // Flag to indicate data is ready to be processed
static bool m_send_flag = 0;
static int ctr_flag = 0;
//static size_t m_rx_size = 0;       // Size of the received data
//m_rx_size = 0; 
#define RAM_START2       0x20000000
// BLE definitions

// USB code
static void bsp_event_callback(bsp_event_t ev)
{
    switch ((unsigned int)ev)
    {
        case CONCAT_2(BSP_EVENT_KEY_, BTN_CDC_DATA_SEND):
        {
            m_send_flag = 1;
            ctr_flag++;
            break;
        }
        
        case BTN_CDC_DATA_KEY_RELEASE :
        {
            m_send_flag = 0;
            break;
        }

        case CONCAT_2(BSP_EVENT_KEY_, BTN_CDC_NOTIFY_SEND):
        {
            ret_code_t ret = app_usbd_cdc_acm_serial_state_notify(&m_app_cdc_acm,
                                                                  APP_USBD_CDC_ACM_SERIAL_STATE_BREAK,
                                                                  false);
            UNUSED_VARIABLE(ret);
            break;
        }

        default:
            return; // no implementation needed
    }
}

static void init_bsp(void)
{
    ret_code_t ret;
    ret = bsp_init(BSP_INIT_BUTTONS, bsp_event_callback);
    APP_ERROR_CHECK(ret);
    
    UNUSED_RETURN_VALUE(bsp_event_to_button_action_assign(BTN_CDC_DATA_SEND,
                                                          BSP_BUTTON_ACTION_RELEASE,
                                                          BTN_CDC_DATA_KEY_RELEASE));
    
    /* Configure LEDs */
    bsp_board_init(BSP_INIT_LEDS);
}

#if NRF_CLI_ENABLED
static void init_cli(void)
{
    ret_code_t ret;
    ret = bsp_cli_init(bsp_event_callback);
    APP_ERROR_CHECK(ret);
    nrf_drv_uart_config_t uart_config = NRF_DRV_UART_DEFAULT_CONFIG;
    uart_config.pseltxd = TX_PIN_NUMBER;
    uart_config.pselrxd = RX_PIN_NUMBER;
    uart_config.hwfc    = NRF_UART_HWFC_DISABLED;
    ret = nrf_cli_init(&m_cli_uart, &uart_config, true, true, NRF_LOG_SEVERITY_INFO);
    APP_ERROR_CHECK(ret);
    ret = nrf_cli_start(&m_cli_uart);
    APP_ERROR_CHECK(ret);
}
#endif

int main(void)
{
    ret_code_t ret;
    //static const app_usbd_config_t usbd_config = {
    //    .ev_state_proc = usbd_user_ev_handler
    //};

    ret = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(ret);


    ret = nrf_drv_clock_init();
    APP_ERROR_CHECK(ret);
    
    nrf_drv_clock_lfclk_request(NULL);

    while(!nrf_drv_clock_lfclk_is_running())
    {
        /* Just waiting */
    }

    //ret = app_timer_init();
    //timers_init();
    APP_ERROR_CHECK(ret);

    init_bsp();
#if NRF_CLI_ENABLED
    init_cli();
#endif
   
    usb_init();
    //app_usbd_serial_num_generate();

    //ret = app_usbd_init(&usbd_config);
    //APP_ERROR_CHECK(ret);
    //NRF_LOG_INFO("USBD CDC ACM example started.");

    //app_usbd_class_inst_t const * class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&m_app_cdc_acm);
    //ret = app_usbd_class_append(class_cdc_acm);
    //APP_ERROR_CHECK(ret);
    
    bool erase_bonds = false;
    //if (USBD_POWER_DETECTION)
    //{
    //    ret = app_usbd_power_events_enable();
    //    APP_ERROR_CHECK(ret);
    //}
    //else
    //{
    //    NRF_LOG_INFO("No USB power detection enabled\r\nStarting USB now");

    //    app_usbd_enable();
    //    app_usbd_start();
    //}
    static int rec_data =1;
    size_t size;
    bool send_chck = 0;
    
    uint32_t ram_start = 0;

    //buttons_leds_init(&erase_bonds);
    //set_tx_free(0);
    //fds_init_start();
    ////wait_for_fds_ready();
    //fds_stat_t stat = {0};

    //ret = fds_stat(&stat);
    //APP_ERROR_CHECK(ret);

    //send_usb_data(false, "Found %d valid records.%s", stat.valid_records,fds_err_str(ret));
    //send_usb_data(false, "Found %d dirty records (ready to be garbage collected).", stat.dirty_records);


    
    static int cnt = 1;
    //set_tx_free(1);
    //char errStr1[200] = {};
    //int16_t errLen = 0;
    //errLen = sprintf(errStr1, "%s USB error:%d kot para no test ram",DEVICE_NAME,50);
    //error_from_usb(errStr1, errLen); 
    bool noTestRam = true;//true;
    bool testAll = false;//false;
    if(noTestRam){
        //bsp_board_led_on(led_RED);
        timers_init();
        power_management_init();
        stHandles();
        
        ble_stack_init();// both
        //rtc_init();
        //sys_timer_start();
        //#if IS_RELAY
        scan_init();// c
        //#endif
        

        gap_params_init();
        gatt_init(); // both
        conn_params_init();
        //#if IS_RELAY
        db_discovery_init();// c
        //#endif
        peer_manager_init();
        //#if IS_RELAY
        nus_c_init();// c
        //#endif
        services_init();
        advertising_init();
        //if(AUTO_START){
        //    if(DEVICE_ID == 'C'){
        //        scan_start();
        //    }else{
        //        adv_start(false);
        //    }
        //}
        //adv_start(false);
        bsp_board_led_on(led_RED);
        
        //bsp_board_led_on(LED_GREEN); // 0=yellowGreen (right), 1=RED (left), 2=GREEN (left), 3=BLUE (left), 4=NOTHING, 
        
    }
    //bsp_board_led_on(led_RED);
    while (true)
    {   
        usb_processs();
        //while(app_usbd_event_queue_process()) 
        //{
        //    // wait
        //} 
        if(noTestRam || (testAll && cnt >= 13)){
            idle_state_handle();
        }
        
        if(m_send_flag)//
        {
            if(cnt%2==0){
                bsp_board_led_on(led_RED);
            }else{
                bsp_board_led_on(led_GREEN);
            }
            m_send_flag = 0;
            if(noTestRam){
                if(cnt >= 1){
                    if(large_received){
                      //send_large_nus_call(file_data);
                      send_usb_data(false, "cnt is %d", cnt);
                      large_received = 0;
                    }else{
                      send_usb_data(false, "cnt is %d", cnt);
                      //send_nus_data(-1,"P-2 llo World %d!!!", cnt);
                      //#if IS_RELAY
                      //send_nus_data_c("P-2 llo World %d!!!", cnt);
                      //#endif

                      cnt++;
                    }
                }
            }else if (testAll){
              if (cnt == 1){
                timers_init();
                send_usb_data(false, "timers_init");
                cnt++;
              }else if (cnt == 2){
                ret = power_management_init();
                send_usb_data(false, "power_management_init %d", ret);
                stHandles();
                send_usb_data(false, "stHandles");
                ble_stack_init();
                cnt++;
                send_usb_data(false, "ble_stack_init");
              }else if (cnt == 3){
                ret =  gap_params_init();
                send_usb_data(false, "gap_params_init %d", ret);
                cnt++;
              
              }else if (cnt == 4){
              //#if IS_RELAY
                ret = gatt_init();
              //#endif
                send_usb_data(false, "gatt_init %d", ret);
                cnt++;  
              }else if (cnt == 5){
                ret = conn_params_init();
                send_usb_data(false, "conn_params_init %d", ret);
                cnt++;
              }else if (cnt == 6){
                ret = db_discovery_init();
                send_usb_data(false, "db_discovery_init %d", ret);
                cnt++;
              }else if (cnt == 7){
                ret = peer_manager_init();
                send_usb_data(false, "peer_manager_init %d", ret);
                cnt++;
              }else if (cnt == 8){
                register_vendor_uuid();
                send_usb_data(false, "register_vendor_uuid %d", ret);
                cnt++;
              }else if (cnt == 9){
              //#if IS_RELAY
                ret = scan_init();
              //#endif
                send_usb_data(false, "scan_init %d", ret);
                cnt++;
              }else if (cnt == 10){
              //#if IS_RELAY
                ret = nus_c_init();
              //#endif
                send_usb_data(false, "nus_c_init %d", ret);
                cnt++;
              }else if (cnt == 11){
                ret = services_init();
                send_usb_data(false, "services_init %d", ret);
                cnt++;
              }else if (cnt == 12){
                ret = advertising_init();
                send_usb_data(false, "advertising_init %d", ret);
                cnt++;
              }
              //if (cnt == 1){
              //  ret = power_management_init();
              //  send_usb_data(false, "power_management_init %d", ret);
              //  cnt++;
              //}else if (cnt == 2){
              //  stHandles();
              //  send_usb_data(false, "stHandles");
              //  cnt++;
              //  register_vendor_uuid();
              //  send_usb_data(false, "register_vendor_uuid");
              //}else if (cnt == 3){
              //  ret = ble_stack_init();
              //  send_usb_data(false, "ble_stack_init %d", ret);
              //  cnt++;
              
              //}else if (cnt == 4){
              //#if IS_RELAY
              //  ret = scan_init();
              //#endif
              //  send_usb_data(false, "scan_init %d", ret);
              //  cnt++;  
              //}else if (cnt == 5){
              //  ret = gap_params_init();
              //  send_usb_data(false, "gap_params_init %d", ret);
              //  cnt++;
              //}else if (cnt == 6){
              //  ret = gatt_init();
              //  send_usb_data(false, "gatt_init %d", ret);
              //  cnt++;
              //}else if (cnt == 7){
              //  ret = conn_params_init();
              //  send_usb_data(false, "conn_params_init %d", ret);
              //  cnt++;
              //}else if (cnt == 8){
              //#if IS_RELAY
              //  ret = db_discovery_init();
              //#endif
              //  send_usb_data(false, "db_discovery_init %d", ret);
              //  cnt++;
              //}else if (cnt == 9){
              //  ret = peer_manager_init();
              //  send_usb_data(false, "peer_manager_init %d", ret);
              //  cnt++;
              //}else if (cnt == 10){
              //#if IS_RELAY
              //  ret = nus_c_init();
              //#endif
              //  send_usb_data(false, "nus_c_init %d", ret);
              //  cnt++;
              //}else if (cnt == 11){
              //  ret = services_init();
              //  send_usb_data(false, "services_init %d", ret);
              //  cnt++;
              //}else if (cnt == 12){
              //  ret = advertising_init();
              //  send_usb_data(false, "advertising_init %d", ret);
              //  cnt++;
              //}
            
            }else{
              if (cnt == 1){
                ret = power_management_init();
                ret = nrf_sdh_enable_request();
                send_usb_data(false, "nrf_sdh_enable_request", ret);
                cnt++;
              }
              else if (cnt == 2){
                 //Configure the BLE stack using the default settings.
                // Fetch the start address of the application RAM.
                ret = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
                send_usb_data(false, "nrf_sdh_ble_default_cfg_set, err:%d, rm_st:0x%x\r\n", ret, ram_start);
                //size_t size = sprintf(m_tx_buffer, "Init ble stack, def cfg set: %u, err:%d, rm_st:0x%x\r\n", cnt,ret, ram_start);
                //ret = app_usbd_cdc_acm_write(&m_app_cdc_acm, m_tx_buffer, size);
                //APP_ERROR_CHECK(ret);
                cnt++;
              }else if (cnt == 3){
                uint32_t const * ram_start_check = &ram_start;
                uint32_t const app_ram_start_link = ram_start;

                ret_code_t ret_code = sd_ble_enable(&ram_start);
                bool a2 = ram_start > app_ram_start_link;
                uint32_t ram_total_size      = NRF_FICR->INFO.RAM * 1024;
                uint32_t end_addr = RAM_START2 + ram_total_size - (ram_start);

                if (ram_start > app_ram_start_link)
                {
                    NRF_LOG_WARNING("Insufficient RAM allocated for the SoftDevice.");

                    NRF_LOG_WARNING("Change the RAM start location from 0x%x to 0x%x.",
                                    app_ram_start_link, ram_start);
                    NRF_LOG_WARNING("Maximum RAM size for application is 0x%x.",
                                    end_addr);
                }
                else
                {
                    NRF_LOG_DEBUG("RAM starts at 0x%x", app_ram_start_link);
                    if (ram_start != app_ram_start_link)
                    {
                        NRF_LOG_DEBUG("RAM start location can be adjusted to 0x%x.", ram_start);

                        NRF_LOG_DEBUG("RAM size for application can be adjusted to 0x%x.",
                                      end_addr);
                    }
                }
                send_usb_data(false, "Ea1=%d, a2=%d, stl:0x%x ,st:0x%x ,end:0x%x, err:%d, chck:%d\r\n", 
                        ram_start > app_ram_start_link, ram_start != app_ram_start_link, app_ram_start_link, ram_start, end_addr,ret_code, ram_start_check);
        
                send_usb_data(false,  "Ea1=%d, a2=%d, stl:0x%x ,st:0x%x ,end:0x%x, err:%d, chck:%d\r\n",ram_start > app_ram_start_link, ram_start != app_ram_start_link, app_ram_start_link, ram_start, end_addr,ret_code, ram_start_check);
                cnt++;
              }
            }
            //////////////////////////////
            //else if (cnt == 4){
            //  ret = ble_stack_init();
            //  send_usb_data(false, "ble_stack_init", ret);
            //  cnt++;
            //}else if (cnt == 5){
            //#if IS_RELAY
            //  ret = scan_init();
            //#endif
            //  send_usb_data(false, "scan_init", ret);
            //  cnt++;
            //}else if (cnt == 6){
            //  ret = gap_params_init();
            //  send_usb_data(false, "gap_params_init", ret);
            //  cnt++;
            //}else if (cnt == 7){
            //  ret = gatt_init();
            //  send_usb_data(false, "gatt_init", ret);
            //  cnt++;
            //}else if (cnt == 8){
            //  ret = conn_params_init();
            //  send_usb_data(false, "conn_params_init", ret);
            //  cnt++;
            //}else if (cnt == 9){
            //#if IS_RELAY
            //  ret = db_discovery_init();
            //#endif
            //  send_usb_data(false, "db_discovery_init", ret);
            //  cnt++;
            //}else if (cnt == 10){
            //  ret = peer_manager_init();
            //  send_usb_data(false, "peer_manager_init", ret);
            //  cnt++;
            //}else if (cnt == 11){
            //#if IS_RELAY
            //  ret = nus_c_init();
            //#endif
            //  send_usb_data(false, "nus_c_init", ret);
            //  cnt++;
            //}else if (cnt == 12){
            //  ret = services_init();
            //  send_usb_data(false, "services_init", ret);
            //  cnt++;
            //}else if (cnt == 13){
            //  ret = advertising_init();
            //  send_usb_data(false, "advertising_init", ret);
            //  cnt++;
            //}else if (cnt == 14){
            //  ret = adv_scan_start(erase_bonds);
            //  send_usb_data(false, "adv_scan_start", ret);
            //  cnt++;
            //}
            
            //if(cnt == -1){cnt = 0;}
            //else if(cnt >= 11){
            //    if(large_received){
            //      send_large_nus_call(file_data);
            //      large_received = 0;
            //    }else{
            //      send_nus_data("Hello World %d!!!", cnt);
            //      cnt++;
            //    }
            //}
            //else if(cnt >= 9){ 
            //    cnt++;
            //    send_usb_data(false, "cnt is %d",cnt);
            //}
            

            //if (cnt == 0){
            //    //append_log_to_flash("Logu par fareee %d, 0x%d", 400, 600);
            //    cnt = 9;
            //    send_usb_data(false, "cnt is %d",cnt);
            //}
        }
        //if(tx_buffer_free){
        //  if(cnt == 0){
        //    ret = power_management_init();
        //    check_error("power_management_init SECOND", ret);
        //    cnt++;
        //  }
        //  else if (cnt == 1){
        //    ret = ble_stack_init();
        //    check_error("ble_stack_init", ret);
        //    cnt++;
        //  }else if (cnt == 2){
        //    ret = gap_params_init();
        //    check_error("gap_params_init", ret);
        //    cnt++;
        //  }else if (cnt == 3){
        //    ret = gatt_init();
        //    check_error("gatt_init", ret);
        //    cnt++;
        //  }else if (cnt == 4){
        //    ret = services_init();
        //    check_error("services_init", ret);
        //    cnt++;
        //  }else if (cnt == 5){
        //    ret = advertising_init();
        //    check_error("advertising_init", ret);
        //    cnt++;
        //  }else if (cnt == 6){
        //    ret = conn_params_init();
        //    check_error("conn_params_init", ret);
        //    cnt++;
        //  }else if (cnt == 7){
        //    //ret = advertising_start(erase_bonds);
        //    check_error("advertising_start", ret);
        //    cnt++;
        //  }
        //}
        //if(cnt<8 && cnt != -1){
          
        //}
#if NRF_CLI_ENABLED
        nrf_cli_process(&m_cli_uart);
#endif

        UNUSED_RETURN_VALUE(NRF_LOG_PROCESS());
        /* Sleep CPU only if there was no interrupt since last loop processing */
        __WFE();
    }
}
/** @} */
