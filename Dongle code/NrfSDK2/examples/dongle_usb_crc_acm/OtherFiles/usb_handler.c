#include "usb_handler.h"
#include "nrf_log.h"
#include "boards.h"
#include "common.h"
#include "app_usbd.h"
//#include "nus_handler.h"
#include "app_usbd_cdc_acm.h"
#include "app_usbd_serial_num.h"

#include <stdlib.h>
#include <stdarg.h>
#include "device_conn.h"





// Queue tlv data to centrals
int package_own_counter = 0;
int package_head = 0;
int package_tail = 0;
int package_tail_periodic = 0;
int package_count = 0;
bool package_same_idx_is_ok = true;
int package_max_size = MAX_PACKAGE_SIZE;
int package_storage_size = 256;
package_storage_t package_storage[256];

bool connToRpi = false;
// Queue for sending usb data
typedef struct {
    char data[NRF_DRV_USBD_EPSIZE];
    uint8_t length;
}usb_storage_t;

usb_storage_t usb_storage[300];
int usb_max_size = 300;
int usb_head = 0;
int usb_tail = 0;
int usb_count = 0;

// Flags and helper variables
bool tx_buffer_free = 1;
bool more_data_left = false;
int large_counter = -1;
bool large_received = 0;
bool first_adv_start = true;
size_t remaining_nus_data_length = 0;
size_t file_size;
bool large_requested = false;
static bool receivingL = false;
size_t received_size;

static char m_rx_buffer[READ_SIZE];
static char m_tx_buffer[NRF_DRV_USBD_EPSIZE];


// For sending whole file (data from usb)
bool sending_file = false;
static char *current_string = NULL;  // Pointer to the current large string


// For sending large string with nus chunks; Not used now
static size_t remaining_length = 0;        // Remaining length of the string to be sent
static bool sending_chunks = 0;
uint32_t offset = 0;                  // Current offset in the string


uint64_t total_ticks = 0;  // Total ticks including overflows
uint32_t last_counter = 0; 

void rtc_init(void) {
    // Start the LFCLK source
    NRF_CLOCK->LFCLKSRC = CLOCK_LFCLKSRC_SRC_Xtal;  // Use external crystal
    NRF_CLOCK->TASKS_LFCLKSTART = 1;

    // Wait for the LFCLK to start
    while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0);
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;  // Clear the event after starting

    // Configure the RTC
    NRF_RTC0->PRESCALER = 0;  // No prescaler, 32768 Hz tick frequency
    NRF_RTC0->INTENSET = RTC_INTENSET_OVRFLW_Msk;  // Enable overflow interrupt
    NRF_RTC0->TASKS_CLEAR = 1;  // Clear the RTC counter
    NRF_RTC0->TASKS_START = 1;  // Start the RTC

    //NVIC_SetPriority(RTC0_IRQn, 1);  // Set lower priority if needed
    //NVIC_EnableIRQ(RTC0_IRQn);  // Enable RTC interrupt in NVIC
}

//volatile uint32_t rtc_overflow_count = 0;

//void RTC0_IRQHandler(void) {
//    if (NRF_RTC0->EVENTS_OVRFLW) {
//        send_usb_data(true,"Time overflow after");
//        NRF_RTC0->EVENTS_OVRFLW = 0;  // Clear the overflow event
//        rtc_overflow_count++;
//        send_usb_data(true,"Time overflow after");
//    }
//}

//char* get_time(){
//    uint32_t counter = NRF_RTC0->COUNTER;
//    uint64_t total_ticks = ((uint64_t)rtc_overflow_count << 24) | counter; // prevent overflow after every 512s
//    uint64_t time_ms = (total_ticks * 1000) / 32768; // RTC_FREQUENCY;

//    uint32_t minutes = time_ms / 60000;  // Calculate minutes
//    uint32_t seconds = (time_ms % 60000) / 1000;  // Calculate seconds
//    uint32_t milliseconds = time_ms % 1000;  // Calculate milliseconds
    
//    char* msg = (char*)malloc(30 * sizeof(char));
//    sprintf(msg, "[%02lu:%02lu.%03lu]\n", minutes, seconds, milliseconds);
//    return msg;
//}


void error_from_usb(char* type, int len){
    if(!connToRpi){
        return;
    }
    if(PHONE_HANDLE!=NRF_SDH_BLE_TOTAL_LINK_COUNT){
        uint8_t errTlv[len+10];
        uint8_t primer[4] = {255,255,255,255};
        memcpy(errTlv, primer, 4);
        memcpy(errTlv+4, type, len+1);
        uint16_t length = sizeof(errTlv);
        //send_tlv_from_c(-1, errTlv, length, "USB Err");
        //send_tlv_from_p(-1, errTlv, length);
        // OPTIONAL: package circular buffer
        //send_usb_data(false, "Handle:%d -> Received b:%d, p%d, l:%d", h,received_batch,received_prefix, received_len);
        memcpy(package_storage[package_head].data, errTlv,len);
        package_storage[package_head].length = len;
        //send_usb_data(false, "Handle:%d -> Received b:%d, p%d, l:%d, Head:%d", h,package_storage[package_head].data[0],package_storage[package_head].data[1], package_storage[package_head].length,package_head);
        package_head = (package_head+1)%package_storage_size;
        package_count++;
        //OPTIONAL: send relay data
        //if(PHONE_HANDLE!=255){
        send_periodic_tlv();
    }
}



void request_usb_data(void){
    if(!large_requested && !large_received){
        //memset(circ_buf.buffer, 0, sizeof(circ_buf.buffer));
        memset(m_rx_buffer, 0, 64);   
        package_head = 0;
        package_tail = 0;
        package_count = 0;
        for(int i=0; i < package_storage_size; i++){
            if(package_storage[i].length > 0){
                memset(package_storage[i].data, 0, package_storage[i].length);
                package_storage[i].length = 0;
            }
        }
        large_requested = true;
        large_counter++;
        //chunk_sent = 0;
        app_usbd_cdc_acm_read_any(&m_app_cdc_acm,
                                                m_rx_buffer,
                                                READ_SIZE);
        
        send_usb_data(true, "Give me data %d %d", large_received, package_max_size);//if large_received = false, send data
        
    }
}

void get_all_handles(void){
    char msg[400] = "Centrals: ";
    int pos = 10;
    //for (uint8_t i = NRF_SDH_BLE_CENTRAL_LINK_COUNT; i < NRF_SDH_BLE_TOTAL_LINK_COUNT; i++) {
        
    //}
    for (uint8_t i = NRF_SDH_BLE_CENTRAL_LINK_COUNT; i < NRF_SDH_BLE_TOTAL_LINK_COUNT; i++) {
        if (devices[i].id != 255) {
            if(devices[i].connected){
                pos += sprintf(msg+pos, "%s:%d ", devices[i].name, i);
            }else{
                pos += sprintf(msg+pos, "%s:%d-NOT_CONN ", devices[i].name, i);
            }
        }
    }
    pos += sprintf(msg+pos, "\n.  Peripherals: ");
    for (uint8_t i = 0; i < NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++) {
        if (devices[i].id != 255) {
            if(devices[i].connected){
                pos += sprintf(msg+pos, "%s:%d ", devices[i].name, i);
            }else{
                pos += sprintf(msg+pos, "%s:%d-NOT_CONN ", devices[i].name, i);
            }
        }
    }
    send_usb_data(true, msg);
    send_usb_data(false, "sd_power_dcdc_mode_set enabled=%d", (NRF_POWER->DCDCEN || NRF_POWER->DCDCEN0));
}

void add_msg_to_package(char* msg){
    if(tx_buffer_free && strlen(msg) < NRF_DRV_USBD_EPSIZE){
        tx_buffer_free = 0;
        memset(m_tx_buffer, 0, sizeof(m_tx_buffer));  
        strcpy(m_tx_buffer, msg);
        ret_code_t ret = app_usbd_cdc_acm_write(&m_app_cdc_acm, m_tx_buffer, strlen(msg));
        if (ret != NRF_SUCCESS) 
        {
            // Handle error
            NRF_LOG_ERROR("Failed to send data, error code: %d", ret);
            char errStr[200] = {};
            int16_t errLen = sprintf(errStr, "%s got USB error:%d at send_usb_data(false, )",DEVICE_NAME,ret);
            error_from_usb(errStr, errLen);

            memset(errStr, 0, sizeof(errStr));
            errLen = sprintf(errStr, "USBerr MSG: %s; msgTail:%d , head:%d",m_tx_buffer, usb_tail, usb_head);
            error_from_usb(errStr, errLen); 
        }
    }else{
        //tx_buffer_free = 0;
        int msgLen = strlen(msg);
        if(msgLen < NRF_DRV_USBD_EPSIZE){
            memcpy(usb_storage[usb_head].data, msg, msgLen);
            usb_storage[usb_head].length = msgLen;
            usb_head = (usb_head + 1) % usb_max_size;
            if(usb_count < usb_max_size){
                usb_count++;
            }
        }else{
            int already_sent = 0;
            while(msgLen - already_sent > 0){
                int length = msgLen - already_sent;
                if(msgLen - already_sent >= NRF_DRV_USBD_EPSIZE){
                    length = NRF_DRV_USBD_EPSIZE;
                }
                memcpy(usb_storage[usb_head].data, msg + already_sent, length);
                usb_storage[usb_head].length = length;
                usb_head = (usb_head + 1) % usb_max_size;
                already_sent = already_sent + length;
                if(usb_count < usb_max_size){
                    usb_count++;
                }
                //msgLen = msgLen -length;
            }
        }
        if(tx_buffer_free){
            strcpy(m_tx_buffer, " ");
            ret_code_t ret = app_usbd_cdc_acm_write(&m_app_cdc_acm, m_tx_buffer, 1);
            tx_buffer_free = 0;
        }
    }
}

char* format_string_va(const char* format, va_list args) {
    va_list args_copy;
    va_copy(args_copy, args);  // Copy args to safely calculate size

    // Calculate the size needed for the formatted string
    size_t needed_size = vsnprintf(NULL, 0, format, args_copy) + 1;  // +1 for null terminator

    va_end(args_copy);

    // Allocate memory for the formatted string
    char *message = malloc(needed_size);
    if (message == NULL) {
        add_msg_to_package("!!!!malloc format_va is NULL");
        free(message);
        message = NULL;
        return NULL;  // Handle memory allocation failure
    }

    // Write the formatted string into the allocated memory
    vsprintf(message, format, args);

    return message;  // Caller is responsible for freeing the memory
}

char* format_string(const char *format, ...){
    va_list args;
    va_start(args, format);
    return format_string_va(format, args);
}



void send_usb_data(bool time, const char *format, ...) {
    // Check if the previous transmission has completed
    if(!connToRpi){
        return;
    }
    va_list args;
    va_start(args, format);
    char* msg1 = format_string_va(format, args);
    if(msg1 == NULL){
        return;
    }
    va_end(args);
    size_t len = strlen(msg1);
    
    //char* msg = (char*)malloc((40+len) * sizeof(char));
    char msg[400];
    //size_t len = 0;
    //if(msg == NULL){
    //    add_msg_to_package("!!!!malloc msg is NULL");
    //    return;
    //}
    if(time){
        //char* msg_time = (char*)malloc((30) * sizeof(char));
        //msg_time = get_time();

        //uint32_t counter = NRF_RTC0->COUNTER;
        uint32_t current_counter = NRF_RTC0->COUNTER;
        // Check for overflow
        bool isOver = false;
        if (current_counter < last_counter) {
            total_ticks += (1ULL << 24);  // Add maximum ticks (2^24) to total_ticks
            isOver = true;
        }

        // Update total_ticks with the current counter value
        total_ticks = (total_ticks & ~(0xFFFFFF)) | current_counter;
        last_counter = current_counter;


        //uint64_t total_ticks = ((uint64_t)rtc_overflow_count << 24) | counter; // prevent overflow after every 512s
        uint64_t time_ms = (total_ticks * 1000) / 32768; // RTC_FREQUENCY;

        uint32_t min = time_ms / 60000;  // Calculate minutes
        uint32_t sec = (time_ms % 60000) / 1000;  // Calculate seconds
        uint32_t milli = time_ms % 1000;  // Calculate milliseconds

        //len = sprintf(msg, "\n[%02lu:%02lu.%03lu]\n.  %s\n.", min, sec, milli, msg1);
        if(isOver){
            len = snprintf(msg, sizeof(msg)-1, "\n[%02lu:%02lu.%03lu] OVER\n.  %s\n.", min, sec, milli, msg1);
            //len = sprintf(msg, "\n[%02lu:%02lu.%03lu] OVER\n.  %s\n.", min, sec, milli, msg1);
        }else{
            len = snprintf(msg, sizeof(msg)-1, "\n[%02lu:%02lu.%03lu]\n.  %s\n.", min, sec, milli, msg1);
            //len = sprintf(msg, "\n[%02lu:%02lu.%03lu]\n.  %s\n.", min, sec, milli, msg1);
        }
        //len = sprintf(msg, "\n%s.  %s\n.", msg_time, msg1);
        //free(msg_time);
        //msg_time = NULL;
    }else{
        //msg = strdup(msg1);
        //len = sprintf(msg, "\nReceived:\n.  %s\n.", msg1);
        len = snprintf(msg, sizeof(msg)-1, "\nReceived:\n.  %s\n.", msg1);

        //len = sprintf(msg, "Received:\n->  %s", msg1);
    }
    //if (len > 0 && msg[len - 1] == '\n') {
    //        msg[len - 1] = '\0';
    //}


    //if (len > 0 && msg[len - 1] != '\n') {
    //    msg[len] = '\n';   // Add the newline character
    //    msg[len + 1] = '\0';  // Add the null terminator
    //}

    //char* msg;
    
    //if (len > 0 && msg1[len - 1] != '\n') {
    //    msg = (char*)malloc(len + 2);
    //    strcpy(msg, msg1);  // Copy the original string
    //    msg[len] = '\n';   // Add the newline character
    //    msg[len + 1] = '\0';  // Add the null terminator
    //}else{
    //    msg = (char*)malloc(len);
    //    strcpy(msg, msg1);  // Copy the original string
    //}
    free(msg1);
    msg1=NULL;

    if (len >= sizeof(msg)) {
        add_msg_to_package("MSG length exceeded.");
        return;
    }

    if(tx_buffer_free && strlen(msg) < NRF_DRV_USBD_EPSIZE){
        tx_buffer_free = 0;
        memset(m_tx_buffer, 0, sizeof(m_tx_buffer));  
        strcpy(m_tx_buffer, msg);
        ret_code_t ret = app_usbd_cdc_acm_write(&m_app_cdc_acm, m_tx_buffer, strlen(msg));
        if (ret != NRF_SUCCESS) 
        {
            // Handle error
            NRF_LOG_ERROR("Failed to send data, error code: %d", ret);
            char errStr[200] = {};
            int16_t errLen = sprintf(errStr, "%s got USB error:%d at send_usb_data(false, )",DEVICE_NAME,ret);
            error_from_usb(errStr, errLen);

            memset(errStr, 0, sizeof(errStr));
            errLen = sprintf(errStr, "USBerr MSG: %s; msgTail:%d , head:%d",m_tx_buffer, usb_tail, usb_head);
            error_from_usb(errStr, errLen); 

            memset(errStr, 0, sizeof(errStr));
            errLen = sprintf(errStr, "USBerr MSG: %s; msg=%s",m_tx_buffer, usb_tail, usb_head);
            error_from_usb(errStr, errLen); 
        }
    }else{
        //tx_buffer_free = 0;
        int msgLen = strlen(msg);
        if(msgLen < NRF_DRV_USBD_EPSIZE){
            memcpy(usb_storage[usb_head].data, msg, msgLen);
            usb_storage[usb_head].length = msgLen;
            usb_head = (usb_head + 1) % usb_max_size;
            if(usb_count < usb_max_size){
                usb_count++;
            }
        }else{
            int already_sent = 0;
            while(msgLen - already_sent > 0){
                int length = msgLen - already_sent;
                if(msgLen - already_sent >= NRF_DRV_USBD_EPSIZE){
                    length = NRF_DRV_USBD_EPSIZE;
                }
                memcpy(usb_storage[usb_head].data, msg + already_sent, length);
                usb_storage[usb_head].length = length;
                usb_head = (usb_head + 1) % usb_max_size;
                already_sent = already_sent + length;
                if(usb_count < usb_max_size){
                    usb_count++;
                }
                //msgLen = msgLen -length;
            }
        }
        if(tx_buffer_free){
            strcpy(m_tx_buffer, " ");
            ret_code_t ret = app_usbd_cdc_acm_write(&m_app_cdc_acm, m_tx_buffer, 1);
            tx_buffer_free = 0;
        }
    }
    //free(msg);
    //msg = NULL;
}

void send_tlv_usb(bool time, char* type, uint8_t *tlv, int last_num_idx, int last_idx, int handle) {
    ////size_t len = tlv[0];
    //// Create a buffer to hold the hexadecimal string representation of the TLV data

    char hex_string[((last_num_idx) * 4) + strlen(type) + 1 + last_idx - last_num_idx+2+5];
    size_t pos = sprintf(hex_string, "%s ", type);

    //char hex_string[((last_num_idx) * 4) + strlen(type) + 1 + last_idx - last_num_idx+5];
    //strcpy(hex_string, type);
    //size_t pos = strlen(hex_string);


    ////strcpy(hex_string, type);
    ////size_t pos = strlen(hex_string);
    ////hex_string[pos] = " ";
    ////pos++;
    for (size_t i = 0; i < last_num_idx; i++) {
        int num_chars = sprintf(hex_string + pos, "%-4d", tlv[i]);  // Convert each byte to a 2-character hex string
        pos += num_chars;
    }
    if (last_idx > last_num_idx) {
        sprintf(hex_string + pos, "%c", tlv[last_num_idx]); 
        pos++;
        // There are additional elements after last_num_idx, append them as raw characters
        //strncat(hex_string, (char *)(tlv + last_num_idx), last_idx-last_num_idx);
        memcpy(hex_string + pos, (char *)(tlv + last_num_idx+1), last_idx-last_num_idx-1);
        pos += last_idx-last_num_idx-1;
    }
    hex_string[pos] = '\n';
    hex_string[pos+1] = '\0';
    // Now send the hexadecimal string via USB
    //send_usb_data(false, "Handle:%d -> %s", handle, hex_string);
    if(memcmp(devices[handle].name, "     ", sizeof(devices[handle].name))){//handle >= 0 && (devices[handle].name[0] == 'P' || devices[handle].name[0] == 'C')){
        send_usb_data(time, "%s:%d -> %s", devices[handle].name, handle, hex_string);
    }else{
         send_usb_data(time, "Handle:%d -> %s", handle, hex_string);   
    }
}


void send_next_chunk(void) { 
    size_t chunk_size = (remaining_length < NRF_DRV_USBD_EPSIZE) ? remaining_length : NRF_DRV_USBD_EPSIZE;

    // Copy the current chunk to m_tx_buffer
    memset(m_tx_buffer, 0, sizeof(m_tx_buffer));  
    memcpy(m_tx_buffer, current_string + offset, chunk_size);

    // Send the chunk via USB
    ret_code_t ret = app_usbd_cdc_acm_write(&m_app_cdc_acm, (uint8_t *)m_tx_buffer, chunk_size);

    if(ret != 0){
        send_usb_data(false, "Error sending usb :%d",ret);
        char errStr[200] = {};
        int16_t errLen = sprintf(errStr, "%s got USB error:%d at send_next_chunk",DEVICE_NAME,ret);
        error_from_usb(errStr, errLen); 

        memset(errStr, 0, sizeof(errStr));
        //errLen = sprintf(errStr, "USBerr MSG: %s; msgStSz:%d",m_tx_buffer, messageStorage.stored);
        errLen = sprintf(errStr, "USBerr MSG: %s; msgTail:%d , head:%d",m_tx_buffer, usb_tail, usb_head);
        error_from_usb(errStr, errLen); 
    }

    // Update the offset and remaining length
    offset += chunk_size;
    remaining_length -= chunk_size;

    if (remaining_length <= 0) {
        sending_chunks = 0;
        if(sending_file){
            remaining_length--;
            sending_file = false;
        }
    }
}

void send_large_string_usb(const char *large_string) {
    if(current_string != NULL){
        free(current_string);
    }
    current_string = (char  *)malloc(strlen(large_string) + 1);
    strcpy(current_string,large_string);
    remaining_length = strlen(current_string);
    offset = 0;
    sending_chunks = 1;
    // Send the first chunk
    send_next_chunk();
}



int send_file_counter = 0;
void send_file_usb(void) {
    //OPTIONAL: Package storage
    //if(!sending_file){
    //    sending_file=true;
    //    //send_file_counter = (package_head+1)%package_storage_size;
    //    send_file_counter = 0;
    //    send_usb_data(false, "Package head is %d", package_head);
    //}
    if(usb_count + 10 < usb_max_size){
        while(package_storage[send_file_counter].length == 0 && send_file_counter < package_storage_size){
            send_file_counter++;
        }
        if(send_file_counter == package_storage_size){
            sending_file = false;
            send_usb_data(false, "Finished early send file at idx %d", send_file_counter);
            send_file_counter = 0;
            return;
        }

        if(package_storage[send_file_counter].length != 0){
            char pckg[12];
            sprintf(pckg, "Pckg %d: ", send_file_counter);
            send_tlv_usb(false, pckg, package_storage[send_file_counter].data, 2, package_storage[send_file_counter].length-4,-1);
            //send_usb_data(false, " \n");
        }
    
        send_file_counter++;
        //send_file_counter = (send_file_counter+1)%package_storage_size;
        if(send_file_counter == package_storage_size){//package_head){
            sending_file = false;
            send_usb_data(false, "Finished send file at idx %d", send_file_counter);
            send_file_counter = 0;
        }
    }
    //int last_pckg = package_head-1;
    //send_usb_data(false, "Package head is %d", package_head);
    //if(package_head > 255){
    //    last_pckg = 255;
    //    send_usb_data(false, "Package head is %d", package_head);
    //}
    //send_tlv_usb(false, "First pckg; ", package_storage[0].data, 2, package_storage[0].length,-1);
    //send_usb_data(false, " \n");
    //send_tlv_usb(false, "Last pckg; ", package_storage[last_pckg].data, 2, package_storage[last_pckg].length,-1);
    //send_usb_data(false, " \n");
}

void start_send_file_usb(void){
    if(!sending_file){
        sending_file=true;
        //send_file_counter = (package_head+1)%package_storage_size;
        send_file_counter = 0;
        send_usb_data(false, "Package head is %d", package_head);
        send_file_usb();
    }
}



void check_error(const char* type, int err){
  if(err!=0){
  int16_t errLen = 0;
  char errStr[200] = {};
    uint8_t msg[100] = {0};
    size_t size = sprintf(msg, "%s err:%d", type, err);
    //app_usbd_cdc_acm_write(&m_app_cdc_acm, m_tx_buffer, size);
    send_usb_data(false, "%s ERR:%d\n", type, err);
    errLen = sprintf(errStr, "%s got USB error:%d-> %s",DEVICE_NAME,0, msg);
    error_from_usb(errStr, errLen); 
  }
}




void usbd_user_ev_handler(app_usbd_event_type_t event)
{
    int16_t errLen = 0;
    switch (event)
    {
        case APP_USBD_EVT_DRV_SUSPEND:{
            //bsp_board_led_off(LED_USB_RESUME);
            char errStr[200] = {};
            errLen = sprintf(errStr, "%s got USB error:%d at APP_USBD_EVT_DRV_SUSPEND, tx_buff=%s",DEVICE_NAME,0, m_tx_buffer);
            error_from_usb(errStr, errLen); 
            break;
        }case APP_USBD_EVT_DRV_RESUME:
            //bsp_board_led_on(LED_USB_RESUME);
            break;
        case APP_USBD_EVT_STARTED:
            break;
        case APP_USBD_EVT_STOPPED:
            app_usbd_disable();
            bsp_board_leds_off();
            char errStr1[200] = {};
            errLen = sprintf(errStr1, "%s got USB error:%d at APP_USBD_EVT_STOPPED",DEVICE_NAME,0);
            error_from_usb(errStr1, errLen); 
            break;
        case APP_USBD_EVT_POWER_DETECTED:
            NRF_LOG_INFO("USB power detected");

            if (!nrf_drv_usbd_is_enabled())
            {
                app_usbd_enable();
            }
            break;
        case APP_USBD_EVT_POWER_REMOVED:
            NRF_LOG_INFO("USB power removed");
            app_usbd_stop();
            break;
        case APP_USBD_EVT_POWER_READY:
            NRF_LOG_INFO("USB ready");
            app_usbd_start();
            break;
        default:
            break;
    }
}

void usb_init(void){
    ret_code_t ret;
    const app_usbd_config_t usbd_config = {
            .ev_state_proc = usbd_user_ev_handler
        };

    app_usbd_serial_num_generate();
    char errStr1[200] = {};
    int16_t errLen = 0;
    ret = app_usbd_init(&usbd_config);
    if(ret != NRF_SUCCESS){
        
          errLen = sprintf(errStr1, "%s USB error:%d at app_usbd_init",DEVICE_NAME,ret);
          error_from_usb(errStr1, errLen); 
        bsp_board_led_on(led_GREEN);
    }
    APP_ERROR_CHECK(ret);
    

    app_usbd_class_inst_t const * class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&m_app_cdc_acm);
    ret = app_usbd_class_append(class_cdc_acm);
    if(ret != NRF_SUCCESS){
        errLen = sprintf(errStr1, "%s USB error:%d at app_usbd_class_append",DEVICE_NAME,ret);
        error_from_usb(errStr1, errLen); 
        bsp_board_led_on(led_GREEN);
    }
    APP_ERROR_CHECK(ret);
 
    if (false)//(USBD_POWER_DETECTION)
    {
        ret = app_usbd_power_events_enable();
        APP_ERROR_CHECK(ret);
    }
    else
    {
        NRF_LOG_INFO("No USB power detection enabled\r\nStarting USB now");

        app_usbd_enable();
        app_usbd_start();
    }
}



void usb_processs(void){
    while(app_usbd_event_queue_process()) 
    {
        // wait
    }  // This closing brace was likely missing
}


void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_cdc_acm_user_event_t event)
{
    app_usbd_cdc_acm_t const * p_cdc_acm = app_usbd_cdc_acm_class_get(p_inst);

    switch (event)
    {
        case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
        {
            //bsp_board_led_on(LED_CDC_ACM_OPEN);

            /* Setup first transfer */
            ret_code_t ret = app_usbd_cdc_acm_read_any(&m_app_cdc_acm,
                                                   m_rx_buffer,
                                                   READ_SIZE);
            UNUSED_VARIABLE(ret);
            break;
        }
        case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
            //bsp_board_led_off(LED_CDC_ACM_OPEN);
            break;
        case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
            //bsp_board_led_invert(LED_CDC_ACM_TX);
            memset(m_tx_buffer,0,64);
            //debug_ctr++;
            if(sending_chunks){
              send_next_chunk();
              //break;
            }else if(usb_tail != usb_head){//usb_tail != usb_head){//usb_count > 0){
                  ret_code_t err;
                  tx_buffer_free = false;

                  strncpy(m_tx_buffer, usb_storage[usb_tail].data, usb_storage[usb_tail].length);
                  //if(usb_storage[usb_tail].length > 15 && strncmp(m_tx_buffer, "Give me data", 12) != 0){
                  //    m_tx_buffer[usb_storage[usb_tail].length - 2] = (char)'0' + usb_count%10;
                  //    m_tx_buffer[usb_storage[usb_tail].length - 3] = (char)'0' + (usb_count%100)/10;
                  //    m_tx_buffer[usb_storage[usb_tail].length - 4] = (char)'0' + usb_count/100;
                  //    m_tx_buffer[usb_storage[usb_tail].length - 5] = 'c';
                  //    m_tx_buffer[usb_storage[usb_tail].length - 6] = (char)'0' + usb_head%10;
                  //    m_tx_buffer[usb_storage[usb_tail].length - 7] = (char)'0' + (usb_head%100)/10;
                  //    m_tx_buffer[usb_storage[usb_tail].length - 8] = (char)'0' + usb_head/100;
                  //    m_tx_buffer[usb_storage[usb_tail].length - 9] = 'h';
                  //    m_tx_buffer[usb_storage[usb_tail].length - 10] = (char)'0' + usb_tail%10;
                  //    m_tx_buffer[usb_storage[usb_tail].length - 11] = (char)'0' + (usb_tail%100)/10;
                  //    m_tx_buffer[usb_storage[usb_tail].length - 12] = (char)'0' + usb_tail/100;
                  //    m_tx_buffer[usb_storage[usb_tail].length - 13] = 't';
                  //}
                  err = app_usbd_cdc_acm_write(&m_app_cdc_acm, m_tx_buffer, usb_storage[usb_tail].length);

                  if(err == NRF_SUCCESS){
                      usb_tail = (usb_tail + 1) % usb_max_size;
                      usb_count--;
                  }else{
                      char errStr[200] = {};
                      int16_t errLen = sprintf(errStr, "%s got USB error:%d at APP_USBD_CDC_ACM_USER_EVT_TX_DONE",DEVICE_NAME,err);
                      error_from_usb(errStr, errLen); 

                      memset(errStr, 0, sizeof(errStr));
                      errLen = sprintf(errStr, "USBerr MSG: %s; msgTail:%d , head:%d",m_tx_buffer, usb_tail, usb_head);
                      error_from_usb(errStr, errLen); 
                  }
            }
              //OPTIONAL: Big message storage
              //if(messageStorage.stored != 0){
              //    ret_code_t err;
              //    tx_buffer_free = false;
              //    uint8_t messageLen;

              //    if(messageStorage.stored <= NRF_DRV_USBD_EPSIZE){
              //        messageLen = messageStorage.stored;
              //        if(messageStorage.breakoff - messageStorage.tail <= messageLen){
              //            messageLen = messageStorage.breakoff - messageStorage.tail;
              //            //messageStorage.breakoff = MessageStorageSize;
              //        }
              //        //else{
              //        //    messageLen = messageLen;
              //        //}
              //    }else{
              //        messageLen = NRF_DRV_USBD_EPSIZE;
              //        if(messageStorage.breakoff - messageStorage.tail <= messageLen){
              //            messageLen = messageStorage.breakoff - messageStorage.tail;
              //            //messageStorage.breakoff = MessageStorageSize;
              //        }
                      
              //    }
              //    memset(m_tx_buffer, 0, sizeof(m_tx_buffer));  
              //    strncpy(m_tx_buffer, messageStorage.data + messageStorage.tail, messageLen);
              //    err = app_usbd_cdc_acm_write(&m_app_cdc_acm, m_tx_buffer, messageLen);
              //    if(err == NRF_SUCCESS){
              //        if(messageLen + messageStorage.tail == messageStorage.breakoff){
              //            messageStorage.tail = 0;
              //            messageStorage.breakoff = MessageStorageSize;
              //        }else{
              //            messageStorage.tail += messageLen;
              //        }
              //        messageStorage.stored -= messageLen;
              //    }else{
              //        char errStr[200] = {};
              //        int16_t errLen = sprintf(errStr, "%s got USB error:%d at APP_USBD_CDC_ACM_USER_EVT_TX_DONE",DEVICE_NAME,err);
              //        error_from_usb(errStr, errLen); 

              //        memset(errStr, 0, sizeof(errStr));
              //        errLen = sprintf(errStr, "USBerr MSG: %s; msgStSz:%d",m_tx_buffer, messageStorage.stored);
              //        error_from_usb(errStr, errLen); 
              //    }
              //    //break;
              //}
              //OPTIONAL: OLD
              //if(head!=NULL){
              //    if(!head->isLarge){
              //        char *next_message = dequeue_message();
              //        strncpy(m_tx_buffer, next_message, strlen(next_message));
              //        ret_code_t ret = app_usbd_cdc_acm_write(&m_app_cdc_acm, m_tx_buffer, strlen(m_tx_buffer));
              //        free(next_message);
              //    }else{
              //        char *next_message = dequeue_message();
              //        send_large_string_usb(next_message);
              //        // Send the next message in the queue
              //        //send_usb_data(false, "%s", next_message);
              //        free(next_message);
              //    } 
              //}
              //else if(sending_file){
              //  send_file_usb();
              //}
            else{
                tx_buffer_free = 1;
                //if(sending_file){
                //    send_file_usb();
                //}
                if(large_received ){//&& cent_info_set){
                    sending_tlv_chunks = true;
                    //chunk_sent = 0;
                    large_received = 0;
                    //cent_info_set = false;
                    last = true; ////
                    //send_usb_data(false, "Got cent info 2 set = %d", cent_info_set);
                    send_chunk_tlv();//(cent_p_nus, cent_handle);
                }
                //break;
            }
              
            //}
            if(sending_file && usb_count + 10 < usb_max_size){
                //if(usb_tail > usb_head){
                //    if((usb_head + 10) % usb_max_size < usb_tail){
                    
                //    }
                //}else{
                //    if()
                //}
                send_file_usb();
            }
            //if(memcmp(m_rx_buffer,m_tx_buffer,sizeof(m_tx_buffer)) == 0){
            //    app_usbd_cdc_acm_read_any(&m_app_cdc_acm,
            //                                    m_rx_buffer,
            //                                    READ_SIZE);
            //}
            break;
        case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
         {
            /* Get amount of data transferred */
            size_t m_rx_size = app_usbd_cdc_acm_rx_size(p_cdc_acm);
            if (m_rx_size > 0)
            {
                connToRpi = true;

                if (strncmp(m_rx_buffer, "SIZE ", 5) == 0) // get max 256*241 = 61455 - 23 for custom start/end = 61432
                {   
                    //bsp_board_led_invert(LED_CDC_ACM_RX);
                    int data_left;
                    file_size = 0;
                    sscanf(m_rx_buffer, "SIZE %d %d", &file_size, &data_left);
                    //file_size = atoi(m_rx_buffer + 5)+24;
                    more_data_left = data_left;
                    //file_size = file_size + 12;
                    //remaining_nus_data_length = file_size + 11;
                    //if (file_size > MAX_FILE_SIZE) {
                    if (file_size > 241*256) {
                        // Handle error: file size exceeds maximum buffer size
                        send_usb_data(false, "File size exceeds maximum buffer size.");
                        break;
                    }
                    
                    //memset(circ_buf.buffer, 0, sizeof(circ_buf.buffer));
                    //circ_buf.head = 0;
                    //////memset(back, 0, 64);
                    //////memcpy(back, m_rx_buffer, strlen(m_rx_buffer));
                    //////app_usbd_cdc_acm_write(&m_app_cdc_acm, back, strlen(back));
                    //////const char *ack = "ACK\n";
                    //////app_usbd_cdc_acm_write(&m_app_cdc_acm, ack, strlen(ack));
                    memset(m_tx_buffer, 0, 64);

                    
                    //////send_usb_data(false, "GOT file_size:%d, data_left:%d, large_count:%d\n", file_size, data_left, large_counter);
                    received_size = 0;
                    //char* last_word = "!Start File!";
                    //memcpy(circ_buf.buffer + received_size, last_word, 12);
                    //received_size += 12;
                    receivingL = true;
                    //large_counter++;
                    send_usb_data(false, "GOT file_size:%d, data_left:%d, large_count:%d\n", file_size, data_left, large_counter);
                    send_usb_data(false, "ACK %s\n", m_rx_buffer);
                    //size_t size = sprintf(m_tx_buffer, "ACK %s\n", m_rx_buffer);
                    //app_usbd_cdc_acm_write(&m_app_cdc_acm, m_tx_buffer, size);
                }
                else if((strncmp(m_rx_buffer, "REQUEST DATA ", 13) == 0)){
                    send_usb_data(false, "Got request data from usb");
                    //app_usbd_cdc_acm_read_any(&m_app_cdc_acm,
                    //                            m_rx_buffer,
                    //                            READ_SIZE);
                    large_requested = false;
                    large_received = false;
                    //if(!large_requested && !large_received){
                    //    memset(circ_buf.buffer, 0, sizeof(circ_buf.buffer));
                    //    memset(m_rx_buffer, 0, 64);    
                    //    app_usbd_cdc_acm_read_any(&m_app_cdc_acm,
                    //                                            m_rx_buffer,
                    //                                            READ_SIZE);
                    //    large_requested = true;
                    //    large_counter++;
                    //    chunk_sent = 0;
                    //    send_usb_data(false, "Give me data %d", large_received);//if large_received = false, send data
        
                    //}
                    request_usb_data();
                }else if (strncmp(m_rx_buffer, "SEND KOT ", 9) == 0)
                {
                    int h;
                    sscanf(m_rx_buffer, "SEND KOT %d", &h);
                    send_usb_data(false, "Comm send kot");
                    uint8_t kot[] = {1, 4, 'K', 'O', 'T'};
                    send_tlv_from_c(h,kot,(uint16_t) 5, "Kot");
                    //send_usb_data(false, );
                }else if (strncmp(m_rx_buffer, "SET PERIOD ", 11) == 0)
                {
                    int p;
                    sscanf(m_rx_buffer, "SET PERIOD %d", &p);
                    sscanf(m_rx_buffer, "SET PERIOD %d", &p);
                    send_usb_data(false, "Comm set period to %d",p);
                    PERIODIC_TIMER_INTERVAL = p;
                    periodic_timer_stop();
                    periodic_timer_start();
                }else if (strncmp(m_rx_buffer, "SET PCKG MAX SIZE ", 18) == 0)
                {
                    int s;
                    sscanf(m_rx_buffer, "SET PCKG MAX SIZE %d", &s);
                    send_usb_data(false, "Comm set max package size to %d",s);
                    package_max_size = s;
                }else if (strncmp(m_rx_buffer, "SET TX PERI CENT ", 17) == 0)
                {
                    int peri;
                    int cent;
                    sscanf(m_rx_buffer, "SET TX PERI CENT %d %d", &peri, &cent);
                    send_usb_data(false, "Comm set max tx to peri %d to cent %d", peri, cent);
                    set_max_tx(peri, cent);
                }else if (strncmp(m_rx_buffer, "STAR 1 ", 6) == 0)
                {
                    doStar = true;
                }else if (strncmp(m_rx_buffer, "STAR 3 ", 6) == 0)
                {
                    count_peri_from_scan_list();
                    show_scan_list();
                }else if (strncmp(m_rx_buffer, "STAR 0 ", 6) == 0)
                {
                    doStar = false;
                    if(!doStar && isPhone){
                        remove_all_from_scan_list();
                        add_peri_to_scan_list("Calmr");
                    }
                    
                }else if (strncmp(m_rx_buffer, "ADD NM ", 6) == 0)
                {
                    remove_all_from_scan_list();
                        add_peri_to_scan_list("ViQtr");
                        add_peri_to_scan_list("MicrV");
                        //add_peri_to_scan_list("Calmr");
                        add_peri_to_scan_list("Glucs");
                        add_peri_to_scan_list("PhysL");
                        add_peri_to_scan_list("PhysR");
                        add_peri_to_scan_list("EpocX");
                        add_peri_to_scan_list("     ");
                        add_peri_to_scan_list("     ");
                        add_peri_to_scan_list("     ");
                        
                }else if (strncmp(m_rx_buffer, "ACTIVATE EMERGENCY ", 19) == 0)
                {
                    //TODO: ACTIVATE EMERGENCY
                    send_usb_data(false, "Comm ACTIVATE EMERGENCY");
                }else if (strncmp(m_rx_buffer, "DISCONNECT ", 11) == 0)
                {
                    //timers_init();
                    //ret_code_t ret = scan_on_interval_start();
                    //scan_stop();
                    uint16_t disc;
                    sscanf(m_rx_buffer, "DISCONNECT %d", &disc);
                    ret_code_t ret = disconnect_handle(disc);
                    send_usb_data(false, "Comm Scan stop, e:%d", ret);
                }else if (strncmp(m_rx_buffer, "SCAN STOP ", 10) == 0)
                {
                    //timers_init();
                    //ret_code_t ret = scan_on_interval_start();
                    scan_stop();
                    send_usb_data(false, "Comm Scan stop");
                }else if (strncmp(m_rx_buffer, "SCAN SPEC ", 10) == 0)
                {
                    //timers_init();
                    //ret_code_t ret = scan_on_interval_start();
                    sscanf(m_rx_buffer, "SCAN SPEC  %5s", spec_target_peri);
                    ret_code_t ret = scan_start();
                    send_usb_data(false, "Comm Scan Spec Start for %s e:%d", spec_target_peri, ret);
                }else if (strncmp(m_rx_buffer, "SCAN", 4) == 0)
                {
                    //timers_init();
                    //ret_code_t ret = scan_on_interval_start();
                    ret_code_t ret = scan_start();
                    send_usb_data(false, "Comm Scan e:%d", ret);
                }else if (strncmp(m_rx_buffer, "ADVTX", 5) == 0)
                {
                    int8_t power = 99;
                    power = atoi(m_rx_buffer + 6);
                    send_usb_data(false, "Comm change ADV tx to %d:",power);
                    change_adv_tx(power);
                    //send_usb_data(false, );
                }else if (strncmp(m_rx_buffer, "CONTX", 5) == 0)
                {
                    int8_t handle, power;
                    sscanf(m_rx_buffer, "CONTX %d %d", &handle, &power);
                    send_usb_data(false, "Comm change CONN:%d tx to %d:",handle, power);
                    change_conn_tx(power, handle, (int8_t) devices[handle].tp.ema_rssi, (int8_t) devices[handle].tp.last_rssi);
                    //send_usb_data(false, );
                }else if (strncmp(m_rx_buffer, "ADVS", 4) == 0)
                {
                    send_usb_data(false, "Comm stop adv");
                    stop_adv();
                    //send_usb_data(false, );
                }else if (strncmp(m_rx_buffer, "ADV START FOR PHONE", 19) == 0)
                {
                    send_usb_data(false, "Comm start adv");
                    adv_start(false);
                    if(first_adv_start){
                        send_usb_data(false, "Start thread");
                        first_adv_start = false;
                    }
                    send_usb_data(false, "Connected to dev ");
                }else if (strncmp(m_rx_buffer, "ADV", 3) == 0)
                {
                    send_usb_data(false, "Comm start adv from %s", DEVICE_NAME);
                    adv_start(false);
                    if(first_adv_start){
                        send_usb_data(false, "Start thread");
                        first_adv_start = false;
                    }
                }else if (strncmp(m_rx_buffer, "RSSI", 4) == 0)
                { 
                    uint16_t handle;
                    sscanf(m_rx_buffer, "RSSI %d", &handle);
                    send_usb_data(false, "Comm start adv %d",handle);
                    start_rssi(handle);
                    //send_usb_data(false, );
                }else if (strncmp(m_rx_buffer, "SENDREQ", 7) == 0)
                {
                    send_usb_data(false, "Comm send req");
                    send_tlv_from_c(-1,tlv_reqData,(uint16_t) 3, "Request");
                    //data_relay_in_process = true;
                    //send_usb_data(false, );
                }
                else if (strncmp(m_rx_buffer, "SEND RESET ", 11) == 0)
                {
                    reset_data_gather_ble_nus();
                    reset_on_req_queue();
                    for (uint8_t i = NRF_SDH_BLE_CENTRAL_LINK_COUNT; i < NRF_SDH_BLE_TOTAL_LINK_COUNT; i++) {
                        if(devices[i].id == 'P'){
                            disconnect_handle(i);
                        }
                    }
                    more_data_left = false;
                    large_counter = -1;
                    large_received = 0;
                    remaining_nus_data_length = 0;
                    large_requested = false;
                    receivingL = false;
                    send_usb_data(false, "Resetting on request data and phone");
                    //send_usb_data(false, );
                }else if (strncmp(m_rx_buffer, "SEND RESET ALL ", 15) == 0)
                {
                    reset_data_gather_ble_nus();
                    reset_on_req_queue();
                    for (uint8_t i = 0; i < NRF_SDH_BLE_TOTAL_LINK_COUNT; i++) {
                        disconnect_handle(i);
                    }
                    more_data_left = false;
                    large_counter = -1;
                    large_received = 0;
                    remaining_nus_data_length = 0;
                    large_requested = false;
                    receivingL = false;
                    send_usb_data(false, "Resetting everything");
                    //send_usb_data(false, );
                }
                else if (strncmp(m_rx_buffer, "HANDLES ", 8) == 0)
                {   
                    send_usb_data(false, "Comm get handles");
                    get_all_handles();
                }else if (strncmp(m_rx_buffer, "GET FILE", 8) == 0)
                {   
                    send_usb_data(false, "Comm get large file, pckg num:%d, received_size:%d", package_count,received_size);
                    //char first_100_chars[101];
                    //strncpy(first_100_chars, circ_buf.buffer, 100);
                    //first_100_chars[100] = '\0'; 
                    //send_usb_data(false, first_100_chars);
                    //char last_500_chars[501];
                    //strncpy(last_500_chars, circ_buf.buffer+circ_buf.head- 500, 500);
                    //last_500_chars[500] = '\0';
                    //send_usb_data(false, last_500_chars);
                    start_send_file_usb();
                }else if (strncmp(m_rx_buffer, "GET ALL FILE", 12) == 0)
                {   
                    //send_file_usb();
                    start_send_file_usb();
                }
                else if (strncmp(m_rx_buffer, "GET ALL FILE", 12) == 0)
                {   
                    //send_file_usb();
                    start_send_file_usb();
                }
                //else if (strncmp(m_rx_buffer, "GET RECS ", 9) == 0)
                //{   
                //    send_usb_data(false, "Comm get ALL rec chunks");
                //    send_all_rec_usb();
                //}
                else if (strncmp(m_rx_buffer, "DEL FILE", 8) == 0)
                {   
                    send_usb_data(false, "Comm delete ALL large file"); 
                    package_head = 0;
                    package_count = 0;
                    package_tail = 0;
                    //memset(circ_buf.buffer, 0, sizeof(circ_buf.buffer));

                }else if (strncmp(m_rx_buffer, "CHANGE SEND TYPE ", 17) == 0)
                {   
                    uint16_t type;
                    sscanf(m_rx_buffer, "CHANGE SEND TYPE %d", &type);
                    if(SEND_TYPE != type){
                        SEND_TYPE = type;
                        for (uint8_t i=NRF_SDH_BLE_CENTRAL_LINK_COUNT; i<NRF_SDH_BLE_TOTAL_LINK_COUNT; i++){
                            if(devices[i].connected){
                                 uint8_t tlv_changeType[2] = {10, type};
                                 send_tlv_from_p(i, tlv_changeType, 2);
                            }
                        }
                    }
                }
                else if (strncmp(m_rx_buffer, "CHANGE ISRELAY ", 13) == 0)
                {   
                    uint16_t relay;
                    sscanf(m_rx_buffer, "CHANGE ISRELAY %d", &relay);
                    if(IS_RELAY != relay){
                        IS_RELAY = relay;
                        SEND_TYPE = 0;
                        change_gap_params(false);
                    }
                }else if (strncmp(m_rx_buffer, "TEST RSSI ", 10) == 0)
                {   
                    send_usb_data(false, "Received TEST RSSI"); 
                    send_test_rssi = true;
                    change_gap_params_test_rssi();
                    start_test_rssi();
                    
                    char mrg[150] ="";
                    int min2 = -71;
                    int max2 = -64;
                    int ideal2 = -68;

                    
                    cent_min_rssi_og = -71;//for phone was 63
                    cent_min_rssi = cent_min_rssi_og;
                    //cent_min_rssi = -61;//for body

                    int oldmin2 = devices[0].tp.min_rssi_limit;
                    int oldmax2 = devices[0].tp.max_rssi_limit;
                    int oldideal2 =devices[0].tp.ideal; 
                    change_rssi_margins_peri(-61, -60, -64);
                    change_rssi_margins_cent(-61, -60, -64);//(-71, -64, -68);
                    //sscanf(m_rx_buffer, "CHANGE PERI MARG %d %d %d", &min, &max, &ideal);
                    char mrg2[150] ="";
                    sprintf(mrg2, "Old margins FOR PERIS min=%d, max=%d ideal=%d; New: min=%d, max=%d ideal=%d",  oldmin2, oldmax2, oldideal2, devices[0].tp.min_rssi_limit, devices[0].tp.max_rssi_limit, devices[0].tp.ideal);
                    send_usb_data(false, mrg2); 
                    
                    
                    
                    oldmin2 = devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.min_rssi_limit;
                    oldmax2 = devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.max_rssi_limit;
                    oldideal2 =devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.ideal; 
                    sprintf(mrg, "Old margins FOR CENTS min=%d, max=%d ideal=%d; New: min=%d, max=%d ideal=%d", oldmin2, oldmax2, oldideal2, devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.min_rssi_limit, devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.max_rssi_limit, devices[NRF_SDH_BLE_CENTRAL_LINK_COUNT].tp.ideal);
                    send_usb_data(false, mrg); 
                }else if (strncmp(m_rx_buffer, "CHANGE RELAY MODE ", 18) == 0)
                {   
                    uint16_t mode;
                    sscanf(m_rx_buffer, "CHANGE RELAY MODE %d", &mode);
                    if(RELAYING_MODE != mode){
                        RELAYING_MODE = mode;
                    }
                }
                else if (strncmp(m_rx_buffer, "INIT SEND TEST RSSI ", 20) == 0)
                {   
                    //if(ble_conn_state_central_conn_count() == NRF_SDH_BLE_CENTRAL_LINK_COUNT){
                    if(IS_RELAY){
                        uint8_t start_test_tlv[1] = {20};
                        send_tlv_from_c(-1, start_test_tlv, 1, "Tell peris to start test.");
                        send_usb_data(true, "GET READYYYYY");
                        send_usb_data(true, "GET READYYYYY");
                        send_usb_data(true, "GET READYYYYY");
                        send_usb_data(true, "GET READYYYYY");
                        send_usb_data(true, "GET READYYYYY");
                        send_usb_data(true, "GET READYYYYY");
                        send_usb_data(true, "GET READYYYYY");
                        send_usb_data(true, "GET READYYYYY");
                    }else{
                        start_test_rssi();
                    }
                    //}
                }
                 else if (strncmp(m_rx_buffer, "SKIP SEND TEST RSSI ", 20) == 0)
                {   
                     if(sending_rssi){
                        trssi_counter = max_trssi_packets - 10;
                        send_usb_data(true, "Trssi counter set to %d/%d", trssi_counter, max_trssi_packets);
                     }
                }
                else if (strncmp(m_rx_buffer, "CHANGE ISSENSOR ", 16) == 0)
                {   
                    uint16_t sensor;
                    sscanf(m_rx_buffer, "CHANGE ISSENSOR %d", &sensor);
                    if(IS_SENSOR_NODE != sensor){
                        IS_SENSOR_NODE = sensor;
                    }
                }
                //void change_ema_coefficient_window(int new_windowSize)
                else if (strncmp(m_rx_buffer, "CHANGE WIND SIZE ", 17) == 0)
                {   
                    int win;
                    char coef[10] ="";
                    sprintf(coef, "%d.%d%d", (int8_t)  ema_coefficient, ((int8_t)(ema_coefficient*10))%10, ((int8_t)(ema_coefficient*100))%10);
                    sscanf(m_rx_buffer, "CHANGE WIND SIZE %d", &win);
                    int oldWind = RSSI_starting_limit;
                    float newCoef = change_ema_coefficient_window(win);
                    char newco[10] ="";
                    sprintf(newco, "%d.%d%d", (int8_t)  ema_coefficient, ((int8_t)(ema_coefficient*10))%10, ((int8_t)(ema_coefficient*100))%10);
                    send_usb_data(false, "Window size was %d with coef=%s, now is size %d and coef=%s", oldWind, coef, RSSI_starting_limit, newco); 
                }else if (strncmp(m_rx_buffer, "CHANGE CONN BUFFERS ", 20) == 0)
                {   
                    int min_cel;
                    int no_min_max;
                    int both_max;
                    int buff_size;
                    sscanf(m_rx_buffer, "CHANGE CONN BUFFERS %d %d %d %d", &min_cel, &no_min_max, &both_max, &buff_size);
                    set_conn_time_buffers(min_cel == 1,no_min_max == 1, both_max == 1, buff_size);
                    send_usb_data(false, "Changed buffers: min_cel=%d, no_min_max=%d, both_max=%d, buffer_size", min_cel, no_min_max, both_max, buff_size); 
                }
                
                else if (strncmp(m_rx_buffer, "CHANGE PERI MARG ", 17) == 0)
                {   
                    char mrg[150] ="";
                    int min;
                    int max;
                    int ideal;
                    sscanf(m_rx_buffer, "CHANGE PERI MARG %d %d %d", &min, &max, &ideal);
                    sprintf(mrg, "Old margins min=%d, max=%d ideal=%d; New: min=%d, max=%d ideal=%d", devices[0].tp.min_rssi_limit, devices[0].tp.max_rssi_limit, devices[0].tp.ideal, min, max, ideal);
                    change_rssi_margins_peri(min, max, ideal);
                    send_usb_data(false, mrg); 
                }
                else if (strncmp(m_rx_buffer, "CHANGE SYSTEM MODE ", 19) == 0)
                {   
                    uint16_t system;
                    sscanf(m_rx_buffer, "CHANGE SYSTEM MODE %d", &system);
                    if(SYSTEM_COMM_MODE != system){
                        SYSTEM_COMM_MODE = system;
                    }
                }
                else if (received_size < file_size && receivingL)
                {
                    //send_usb_data(false, "USB REC len%d: %s", m_rx_size, m_rx_buffer);
                    //OPTIONAL: Use package storage for all
                    if(package_storage[package_head].length == 0){
                        memset(package_storage[package_head].data, 0, package_max_size);
                        package_storage[package_head].data[0] = large_counter;
                        package_storage[package_head].data[1] = package_head;
                        package_storage[package_head].data[2] = DEVICE_ID;
                        package_storage[package_head].length = 3;
                    }else if (package_storage[package_head].length == package_max_size){
                        package_head = (package_head+1)%package_storage_size;
                        package_count ++;
                        //send_usb_data(false, "Moved to pck head %d:", package_head);
                        if((package_head+255)%package_storage_size == package_storage_size-1){
                            large_received = 1;
                            receivingL = false;
                            large_requested = false;
                            send_usb_data(false, "Reached last package storage!!!");
                            //return;
                        }else{
                            memset(package_storage[package_head].data, 0, package_max_size);
                            package_storage[package_head].data[0] = large_counter;
                            package_storage[package_head].data[1] = package_head;
                            package_storage[package_head].data[2] = DEVICE_ID;
                            package_storage[package_head].length = 3;
                        }
                    } 

                    if(m_rx_size <= package_max_size - package_storage[package_head].length){

                        memcpy(package_storage[package_head].data + package_storage[package_head].length, m_rx_buffer, m_rx_size);
                        package_storage[package_head].length += m_rx_size;
                       
                        //send_usb_data(false, "USB 1 ON PCKG:%d len%d:", package_head, package_storage[package_head].length);
                        //send_tlv_usb(false, "DATA: ", package_storage[package_head].data, 2, package_storage[package_head].length, -1);

                    }else if(m_rx_size > package_max_size - package_storage[package_head].length){

                        int first_part_size = package_max_size - package_storage[package_head].length;
                        memcpy(package_storage[package_head].data + package_storage[package_head].length, m_rx_buffer, first_part_size);
                        package_storage[package_head].length += first_part_size;

                        //send_usb_data(false, "USB 2 ON PCKG:%d len%d:", package_head, package_storage[package_head].length);
                        //send_tlv_usb(false, "DATA: ", package_storage[package_head].data, 2, package_storage[package_head].length, -1);

                        package_head = (package_head+1)%package_storage_size;
                        package_count ++;
                        //send_usb_data(false, "Moved to pck head %d:", package_head);
                        if((package_head+255)%package_storage_size == package_storage_size-1){
                            large_received = 1;
                            receivingL = false;
                            large_requested = false;
                            send_usb_data(false, "Reached last package storage, data leftover %d!!!", m_rx_size-first_part_size);
                            //return;
                        }else{
                            memset(package_storage[package_head].data, 0, package_max_size);
                            package_storage[package_head].data[0] = large_counter;
                            package_storage[package_head].data[1] = package_head;
                            package_storage[package_head].data[2] = DEVICE_ID;
                            package_storage[package_head].length = 3;
                            memcpy(package_storage[package_head].data + package_storage[package_head].length, m_rx_buffer + first_part_size, m_rx_size - first_part_size);
                            
                            package_storage[package_head].length += (m_rx_size - first_part_size);
                            
                            //send_usb_data(false, "USB 3 ON PCKG:%d len%d: ", package_head, package_storage[package_head].length);
                            //send_tlv_usb(false, "DATA: ", package_storage[package_head].data, 2, package_storage[package_head].length, -1);
                        }
                    }
                    
                    received_size += m_rx_size;
                    if (received_size >= file_size)
                    {   
                        //char* last_word = " !End File!";
                        //////memcpy(file_data + received_size, last_word, 12);
                        //memcpy(circ_buf.buffer + received_size, last_word, strlen(last_word));
                        //received_size += strlen(last_word);
                        large_received = 1;
                        receivingL = false;
                        //circ_buf.buffer[received_size] = '\0';
                        remaining_nus_data_length = received_size;
                        //circ_buf.head = received_size;
                        //char first_100_chars[21];
                        //////strncpy(first_100_chars, file_data, 100);
                        //memcpy(first_100_chars, circ_buf.buffer, 20); 
                        //first_100_chars[20] = '\0'; 
                        //send_usb_data(false, first_100_chars);
                        //send_usb_data(false, circ_buf.buffer+received_size - 20);
                        
                        send_tlv_usb(false, "Got USB data first pref: ", package_storage[package_tail].data, 2, 25,-1);
                        send_usb_data(false, " \n");
                        send_tlv_usb(false, "Got USB data last pref: ", package_storage[package_head].data, 2, 25,-1);
                        if(package_head != 0 && package_storage[package_head].length > 3){
                            package_head = (package_head+1)%package_storage_size;
                            package_count++;
                        }
                        send_usb_data(true, "END received_size = %d", received_size);
                        large_requested = false;
                        //send_usb_data(false, "Got cent info set = %d", cent_info_set);
                        //if(cent_info_set && large_received){
                        //    send_usb_data(false, "Got cent info set = %d", cent_info_set);
                        //    sending_tlv_chunks = true;
                        //    chunk_sent = 0;
                        //    large_received = 0;
                        //    cent_info_set = false;
                        //    last = true;
                        //    send_chunk_tlv(cent_p_nus, cent_handle);
                        //    
                        //}
                    }else{
                        //char first_100_chars[m_rx_size+1];
                        //memcpy(first_100_chars, circ_buf.buffer+received_size-m_rx_size, m_rx_size); 
                        //first_100_chars[m_rx_size] = '\0'; 
                        //send_usb_data(false, "received_sizeee = %d, %s", received_size, m_rx_buffer);
                    }
                    
                }

                
            }
            memset(m_rx_buffer, 0, 64);    
            app_usbd_cdc_acm_read_any(&m_app_cdc_acm,
                                            m_rx_buffer,
                                            READ_SIZE);
            //bsp_board_led_invert(LED_CDC_ACM_RX);
            break;
        }
        default:
            break;
    }
}  

/* @brief CDC_ACM class instance
 * */
APP_USBD_CDC_ACM_GLOBAL_DEF(m_app_cdc_acm,
                            cdc_acm_user_ev_handler,
                            CDC_ACM_COMM_INTERFACE,
                            CDC_ACM_DATA_INTERFACE,
                            CDC_ACM_COMM_EPIN,
                            CDC_ACM_DATA_EPIN,
                            CDC_ACM_DATA_EPOUT,
                            APP_USBD_CDC_COMM_PROTOCOL_AT_V250
);