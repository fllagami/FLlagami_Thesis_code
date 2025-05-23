#ifndef   DEVICE_CONN_H
#define DEVICE_CONN_H
#include "sdk_config.h"
#include "ble_gap.h"
#include "ble_nus_c_common_types.h"
#include "devices_conn_params.h"
#include "devices_tp.h"
#include "boards.h"
#include "app_timer.h"
#include <stdbool.h>


// Presets of command tlv messages
extern uint8_t tlv_nameParams[5];
extern uint8_t empty_tlv[1];
extern uint8_t fast_tlv[1];
extern uint8_t tlv_start_data[2];
extern uint8_t starting_periodic_tlv[2];
extern uint8_t tlv_recAll[2];
extern uint8_t tlv_rssiChange[4];
extern uint8_t tlv_reqData[2];
extern uint8_t tlv_chooseCent[5];
extern uint8_t tlv_chooseCentResponse[13];
extern uint8_t tlv_chooseCentResponseSecond[7];
extern uint8_t tlv_chooseCentFinal[3];
extern uint8_t tlv_stopData[3];
extern uint8_t tlv_resumeData[1];
extern uint8_t tlv_allDataSent[5];
extern uint8_t tlv_advertiseSink[4];

extern int ALREADY_REC;
extern int NO_MISSED;
extern int MISSED;
extern int MISSED_FULL;
int checkMissed(int h, int received_prefix, int received_batch, bool finished);

//#define RSSI_WINDOW_SIZE  10
typedef struct {
    uint8_t data[244];
    uint16_t length;
}tlv_storage_t;
extern int total_pckg_count;

void add_to_device_tlv_strg(int h, uint8_t* tlv, uint16_t length);
void process_stored_tlv(void);

ret_code_t send_specific_tlv_to_peripheral(uint16_t h, int tail);
ret_code_t send_specific_tlv_to_central(uint16_t h, int tail);
int count_peri_from_scan_list(void);
void remove_all_from_scan_list(void);
typedef struct {
    int last;
    int last_missing_rec;
    int sent_missed_msg_cnt;
    int received_last_msg_cnt;
    int received_all_count;
    int left_to_rec;
    
    int new_miss_idx; 

    bool stopped;
    int stopped_pref;

    int first_miss_rec;
    uint8_t missed[256];

    uint8_t send_finish_p[5];
    uint8_t miss_idx;
    uint8_t miss_b_idx;
    uint8_t missed_extra[256];
    bool waiting_retransmit;
    bool finished;
    bool ensure_send;
    uint8_t all_received[500];//[256];
    int all_received_counter;
    uint8_t tlv_ensure[256];
    uint16_t ensure_len;
    uint8_t ensure_counter;
    int batch_number;
    bool allDataSentFromPeri;
    bool recPhoneMissData;
    bool chosenRelay;
    bool is_sending_on_req;
    bool isPeriodic;
    bool isRequest;
    bool isRelay;
    uint8_t type;
    bool connToPhone;
    bool firstAskAdv;
    bool askedAdvPhone;
    ble_nus_c_t* nus;
    int miss_ctr;
    int miss_ctr_extra;

    bool use_miss;
    int miss_base;
    int mode;
    bool sending_missed;
    bool sending_idle;
    bool sending_to_phone;
    bool sending_stored;
    bool sending_periodic;
    bool sending_stored_waiting;
    int final_miss_idx;

    int final_pref;
    int final_batch;

    bool sent_start_send;
    bool sent_send_fast;
    bool miss_sent;
    bool miss_ready;
    bool miss_rec;
} peri_relay_info_t;

typedef struct{
    bool ensure_send;
    uint8_t tlv_ensure[256];
    uint16_t ensure_len;
    uint8_t ensure_counter;
    bool chooseCentDone;
    bool gotFinalCent;
    bool connecting;
    bool connToPhone;
    bool isNotPaused;
    bool isPhone;
    bool chosenRelay;
    bool notifiedRpi;
    int16_t last_pref_sent;

    int16_t stoppedBatch;
    int16_t stoppedPrefix;

    bool start_send_after_conn_update;

}central_send_info_t;

typedef  struct{
    //int rssi; // rssi from cent
    int own_idx;
    //int incr_counter;
    //int decr_counter;
    int tx_idx; // idx for tx_range
    int8_t tx_range[10];//tx from this device //= {0, -12, -20, -40};//-40dBm, -20dBm, -16dBm, -12dBm, -8dBm, -4dBm, 0dBm
    int max_tx_idx;
    int min_tx_idx;
    int max_tx_idx_curr;
    int min_tx_idx_curr;
    int8_t rssi_window[2];//[RSSI_WINDOW_SIZE];
    int sum_rssi;
    int head_rssi;
    int8_t avg_rssi;
    int signal_to_lower_counter;
    float ema_rssi;
    int received_counter;
    //int min_tx;
    //int max_tx;
    int skip;
    int min_rssi_limit;
    int max_rssi_limit;
    int default_tx_idx;
    int ideal;
    int last_rssi;
    int requested_tx_idx;
    int own_max_tx_idx;
    int window;
    bool do_first_ema;
    float coefficient;
    int other_tx_idx;

    bool get_new_rssi;
    bool use_pdr;
    bool stopped_pdr;
    int pdr_wait_count;
    float pdr;
    char pdr_str[6];

    int wait_avg_cnt;

    bool doing_const_test;
    
    // for tpc with pdr
    bool first_evt_skip;
    bool docountpack;
    int cnt_pack[MAX_EVENT_WINDOW];
    int evt_head;
    int cnt_evt;
    int evt_max_p;
    bool do_without_pdr;

} tx_rssi_info_t; 

typedef struct{
    ble_gap_conn_params_t params;
    uint8_t tlv[256];
    int len;
    bool do_tlv;
}triggered_conn_update_t;
extern triggered_conn_update_t conn_update_timer_params;
typedef struct{
    peri_relay_info_t peri;
    central_send_info_t cent;
    tx_rssi_info_t tp;
    char name[6];
    char id;
    int curr_interval;
    int curr_sl;
    int cnt_no_rssi;
    //uint8_t nameNum;
    bool connected;
    bool updated_conn;
    int last_test_pref;
    ble_gap_conn_params_t conn_params;
    bool params_queued;
    triggered_conn_update_t current_updating_conn_params;
    triggered_conn_update_t relay_all_upd_params;
    triggered_conn_update_t queued_conn_params;
    ble_gap_conn_params_t idle_params;
    ble_gap_conn_params_t stored_params;
    ble_gap_conn_params_t periodic_params;
    ble_gap_conn_params_t phone_params;
    ble_gap_conn_params_t wait_params;
    bool waiting_conn_update;
    int handle;
    app_timer_id_t tp_timer;
    int time_passed;
    tlv_storage_t tlvs[15];
    int head;
    int tail;
    int count;
    bool sending_alot;
    int evt_len;
    bool first_packet;
    bool is_relaying;
    bool relayed;
    int relay_h;

    uint8_t last_rec[256];
    uint8_t last_len;
} device_info_t;
void first_packet_timer_start(void);
typedef struct{
    int count_type1;
    int count_type1_sink;
    int count_type2;
    int count_type3;

    int disc_type1;
    int disc_type1_sink;
    int disc_type2;
    int disc_type3;

    int pref_min_rssi_limit;
    int pref_max_rssi_limit;
    int pref_ideal_rssi;
    int pref_ema_window_size;

} relay_info_t;
extern relay_info_t relay_info;

void tp_timer_start(device_info_t *device);
void tp_timer_stop(device_info_t *device);
void tp_timer_handler(void *p_context);
char* getNameFromId(char id);
int find_idx_of_relayed(char id);
// For storing information about connected devices
extern device_info_t devices[];

extern bool sentAllOwnStoredData;

// Handles for connection to send on request or periodic data
extern uint16_t PHONE_HANDLE;
extern uint16_t CHOSEN_HANDLE;
extern uint16_t RELAY_HANDLE;
extern uint16_t RELAY2_HANDLE;

// Helper variables for state of self device
extern bool connectedToPhone;
extern uint8_t periodicRelayCounter;
extern bool connected_peris[];
extern int8_t peri_relay_count;
extern bool there_are_stopped;

// To reset the device structs
void reset_device_base(int id);
void reset_device_cent(int id);
void reset_device_peri(int id, bool all);
void reset_device_tp(int id);
void reset_device_full(int id);
void change_rssi_margins_peri_specific(int h, int min, int max, int ideal);

void register_device_info(uint16_t handle, uint8_t id, bool isRequest);
void set_other_device_op_mode(int h, int mode);
void set_this_device_sim_params(void);
void set_other_device_sim_params(int h, uint8_t* info);
void set_peri_window_size(char id, int h);
// For managing the connections



//#if RELAY_REQ_ON_QUEUE
// Queue of peripherals to be relayed on request
typedef struct on_req_queue_t {
    int handle;
    char nameId;
    //bool connected;
    struct on_req_queue_t *next;
} on_req_queue_t;
extern on_req_queue_t* on_req_queue;
extern on_req_queue_t* last_on_req_queue;
//#endif


// For keeping track of scanning
extern char m_target_periph_name[];
extern char spec_target_peri[];
extern char *m_target_periph_names[];
void remove_peri_from_scan_list(char* name);
void add_peri_to_scan_list(char* name);
void show_scan_list(void);
extern bool scanning_for_on_req;


extern bool done_onreq_adv_for_phone;
extern bool got_adv_phone;
extern int count_onreq;
extern int count_onreq_not_phone;
extern int count_periodic;




// Function pointers from other files
//extern void (*usb_data_sender)(const char *format, ...);
//extern void (*tlv_periodic_sender)(void);
//extern int (*tlv_from_p_sender)(int16_t h, bool rssi, uint8_t* tlv, uint16_t length);
//extern int (*tlv_from_c_sender)(int h, bool rssi, uint8_t* tlv, uint16_t length, char* type);

void start_test_rssi(void);
extern bool send_test_rssi;
void send_test_rssi_tlv(void);
extern int8_t last_rssi_test;
extern uint8_t last_pref_rssi_test;

extern int bytes_per_period;
extern int stored_bytes;
extern int stored_bytes_sent;
extern bool finished_stored;
extern bool finished_missing;
extern int periodic_waiting_bytes;
extern uint8_t batch_counter;
extern uint8_t pref_counter;
extern uint8_t dummy_data[244];

#endif // DEVICE_CONN_H