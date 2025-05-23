#include "device_conn.h"
#include "ble_conn_state.h"
#include "common.h"

// Different presets of connection parameters depending on situation
ble_gap_conn_params_t default_no_use_conn_params = {
    .min_conn_interval = MSEC_TO_UNITS(999, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    .max_conn_interval = MSEC_TO_UNITS(1999, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    .slave_latency = 0,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(20000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};

ble_gap_conn_params_t not_relayed_during_conn_params = {
    .min_conn_interval = MSEC_TO_UNITS(1300, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    .max_conn_interval = MSEC_TO_UNITS(1500, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    .slave_latency = 0,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(15000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};

ble_gap_conn_params_t power_saving_conn_params = {
    .min_conn_interval = MSEC_TO_UNITS(900, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    .max_conn_interval = MSEC_TO_UNITS(1000, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    .slave_latency = 0,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(15000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};

ble_gap_conn_params_t periodic_conn_params = {
    .min_conn_interval = MSEC_TO_UNITS(900, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    .max_conn_interval = MSEC_TO_UNITS(1000, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    .slave_latency = 0,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(15000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};

ble_gap_conn_params_t phone_default_conn_params = {
    .min_conn_interval = MSEC_TO_UNITS(300, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    .max_conn_interval = MSEC_TO_UNITS(400, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    .slave_latency = 0,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(5000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};

ble_gap_conn_params_t initial_conn_params = {
    //.min_conn_interval = MSEC_TO_UNITS(200, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    //.max_conn_interval = MSEC_TO_UNITS(250, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    .min_conn_interval = MSEC_TO_UNITS(90, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    .max_conn_interval = MSEC_TO_UNITS(100, UNIT_1_25_MS), 
    .slave_latency = 0,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(1500, UNIT_10_MS) //change from 15000         // Supervision timeout = 4000 * 10 ms = 40 seconds
};

ble_gap_conn_params_t on_request_power_saving_conn_params = {
    .min_conn_interval = MSEC_TO_UNITS(900, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    .max_conn_interval = MSEC_TO_UNITS(1000, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    .slave_latency = 0,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(15000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};

ble_gap_conn_params_t on_request_conn_params = {
    .min_conn_interval = MSEC_TO_UNITS(90, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms    80
    .max_conn_interval = MSEC_TO_UNITS(100, UNIT_1_25_MS),     // 200 * 1.25 ms = 250 ms
    .slave_latency = 0,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(15000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};

ble_gap_conn_params_t peri_to_relay_on_request_conn_params = {
    .min_conn_interval = MSEC_TO_UNITS(90, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms    80
    .max_conn_interval = MSEC_TO_UNITS(100, UNIT_1_25_MS),     // 200 * 1.25 ms = 250 ms
    .slave_latency = 0,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(15000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};

ble_gap_conn_params_t peri_request_to_phone_conn_params = {
    .min_conn_interval = MSEC_TO_UNITS(200, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    .max_conn_interval = MSEC_TO_UNITS(300, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    .slave_latency = 0,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(10000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};

ble_gap_conn_params_t phone_scan_for_peris_conn_params = {
    .min_conn_interval = MSEC_TO_UNITS(NRF_SDH_BLE_GAP_EVENT_LENGTH*10, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    .max_conn_interval = MSEC_TO_UNITS(NRF_SDH_BLE_GAP_EVENT_LENGTH*13, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    .slave_latency = 0,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(4000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};

ble_gap_conn_params_t phone_with_request_conn_params = {
    .min_conn_interval = MSEC_TO_UNITS(90, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms    80
    .max_conn_interval = MSEC_TO_UNITS(100, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms    100
    .slave_latency = 3,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(5000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};

ble_gap_conn_params_t phone_no_request_conn_params = {
    //.min_conn_interval = MSEC_TO_UNITS(300, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    //.max_conn_interval = MSEC_TO_UNITS(500, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    .min_conn_interval = MSEC_TO_UNITS(100, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    .max_conn_interval = MSEC_TO_UNITS(200, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    .slave_latency = 0,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(10000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};


// Change on_request_conn_params depending on how many devices will be relayed simultaneously
//void calculate_on_request_params_old(uint16_t num_req){//OLDDDDD
//    //if(num_req == 0){
//    num_req++;
//    //}
//    int relay_time_buffer = 0;
//    if(SEND_TYPE == 3){
//        relay_time_buffer = NRF_SDH_BLE_GAP_EVENT_LENGTH*(count_periodic + count_onreq_not_phone);
//    }
//    on_request_conn_params.min_conn_interval = MSEC_TO_UNITS(num_req*(NRF_SDH_BLE_GAP_EVENT_LENGTH+5)+relay_time_buffer, UNIT_1_25_MS);
//    on_request_conn_params.max_conn_interval = MSEC_TO_UNITS(num_req*(NRF_SDH_BLE_GAP_EVENT_LENGTH+15)+relay_time_buffer, UNIT_1_25_MS);
//    on_request_conn_params.slave_latency = 0;     // No slave latency
//    on_request_conn_params.conn_sup_timeout = MSEC_TO_UNITS((num_req*(NRF_SDH_BLE_GAP_EVENT_LENGTH+15)+relay_time_buffer)*10, UNIT_10_MS); //(num_req*150*15, UNIT_10_MS);     // Supervision timeout = 4000 * 10 ms = 40 seconds
//}

//void calculate_on_request_params(uint16_t num_req){
//    num_req++;
//    //int event_len_buffer = 5;
//    int cel_max = NRF_SDH_BLE_GAP_EVENT_LENGTH;

//    int is_rel_sn = 0;
//    if(IS_SENSOR_NODE && SEND_TYPE == 3){
//        is_rel_sn = 1;
//    }
    
//    conn_params_on_gather_type1.min_conn_interval = MSEC_TO_UNITS((2*num_req - is_rel_sn)*NRF_SDH_BLE_GAP_EVENT_LENGTH, UNIT_1_25_MS);//=100-120 ms
//    conn_params_on_gather_type1.max_conn_interval = MSEC_TO_UNITS((2*num_req - is_rel_sn)*(NRF_SDH_BLE_GAP_EVENT_LENGTH + event_len_buffer), UNIT_1_25_MS);
//    //=125-150ms
//    conn_params_on_gather_type1.slave_latency = 0;     // No slave latency
//    conn_params_on_gather_type1.conn_sup_timeout = MSEC_TO_UNITS((num_req*(NRF_SDH_BLE_GAP_EVENT_LENGTH+15)+relay_time_buffer)*10, UNIT_10_MS); 
//    //(num_req*150*15, UNIT_10_MS);     // Supervision timeout = 4000 * 10 ms = 40 seconds
//}

//bool incl_sink_conns = false;
//void calculate_relay_params2(){
//    //num_req++;
//    int cel_max = NRF_SDH_BLE_GAP_EVENT_LENGTH;

//    int is_rel_sn = 0;
//    if(IS_SENSOR_NODE && SEND_TYPE == 3){
//        is_rel_sn = 1;
//    }

//    int sink_events;
//    int type1_events;
//    if(incl_sink_conns){//+1 for type 2 and 3
//        sink_events = relay_info.count_type1 + 1;
//        //type1_events = relay_info.count_type1 - is_rel_sn + 1;
//    }else{
//        sink_events = relay_info.count_type1 - relay_info.count_type1_sink + 1;
//        //type1_events = relay_info.count_type1 - relay_info.count_type1_sink + 1 - is_rel_sn;
//    }
//    //type1_events = sink_events - is_rel_sn;
//    type1_events = sink_events; //no is_rel_sn to keep sink intervals nice 1sRs2s and not 1ss2s..1ss2s
//    int type2_events = relay_info.count_type2*(sink_events + type1_events);
    
//    // TYPE 1 conn params
//    conn_params_on_gather_type1.min_conn_interval = MSEC_TO_UNITS(
//            //(2*num_req - is_rel_sn)*NRF_SDH_BLE_GAP_EVENT_LENGTH, UNIT_1_25_MS);//=100-120 ms
//            (sink_events + type1_events)*NRF_SDH_BLE_GAP_EVENT_LENGTH, UNIT_1_25_MS);//=100-120 ms
//    conn_params_on_gather_type1.max_conn_interval = MSEC_TO_UNITS(
//            (sink_events + type1_events)*(NRF_SDH_BLE_GAP_EVENT_LENGTH + event_len_buffer), UNIT_1_25_MS);//=125-150ms
//    conn_params_on_gather_type1.slave_latency = 0;     // No slave latency
//    conn_params_on_gather_type1.conn_sup_timeout = MSEC_TO_UNITS(2000, UNIT_10_MS); 
//    //(num_req*150*15, UNIT_10_MS);     // Supervision timeout = 4000 * 10 ms = 40 seconds

//    // TYPE 2 conn params
//    conn_params_on_gather_type2.min_conn_interval = MSEC_TO_UNITS(
//            type2_events*NRF_SDH_BLE_GAP_EVENT_LENGTH, UNIT_1_25_MS);//=100-120 ms
//    conn_params_on_gather_type2.max_conn_interval = MSEC_TO_UNITS(
//            type2_events*(NRF_SDH_BLE_GAP_EVENT_LENGTH + event_len_buffer), UNIT_1_25_MS);//=125-150ms
//    conn_params_on_gather_type2.slave_latency = 0;     // No slave latency
//    conn_params_on_gather_type2.conn_sup_timeout = MSEC_TO_UNITS(3000, UNIT_10_MS); 

//    // Sink conn params
//    conn_params_on_gather_sink.min_conn_interval = MSEC_TO_UNITS(
//            2*NRF_SDH_BLE_GAP_EVENT_LENGTH, UNIT_1_25_MS);//=100-120 ms
//    conn_params_on_gather_sink.max_conn_interval = MSEC_TO_UNITS(
//            2*(NRF_SDH_BLE_GAP_EVENT_LENGTH + event_len_buffer), UNIT_1_25_MS);//=125-150ms
//    conn_params_on_gather_sink.slave_latency = 0;     // No slave latency
//    conn_params_on_gather_sink.conn_sup_timeout = MSEC_TO_UNITS(3000, UNIT_10_MS); 
//}

//void calculate_t1_sink_params2(uint16_t num_t1, bool incl_other_sink_conns){
//    //int event_len_buffer = 5;
//    int cel_max = NRF_SDH_BLE_GAP_EVENT_LENGTH;

//    int sink_events;
//    int type1_events;
//    if(incl_other_sink_conns){//+1 for type 2 and 3
//        sink_events = relay_info.count_type1 + 1;
//        //type1_events = relay_info.count_type1 - is_rel_sn + 1;
//    }else{
//        sink_events = relay_info.count_type1 - relay_info.count_type1_sink + 1;
//        //type1_events = relay_info.count_type1 - relay_info.count_type1_sink + 1 - is_rel_sn;
//    }
//    //type1_events = sink_events - is_rel_sn;
//    type1_events = sink_events; //no is_rel_sn to keep sink intervals nice 1sRs2s and not 1ss2s..1ss2s
    
//    // Sink conn params
//    conn_params_on_gather_sink.min_conn_interval = MSEC_TO_UNITS(
//            2*NRF_SDH_BLE_GAP_EVENT_LENGTH, UNIT_1_25_MS);//=100-120 ms
//    conn_params_on_gather_sink.max_conn_interval = MSEC_TO_UNITS(
//            2*(NRF_SDH_BLE_GAP_EVENT_LENGTH + event_len_buffer), UNIT_1_25_MS);//=125-150ms
//    conn_params_on_gather_sink.slave_latency = 0;     // No slave latency
//    conn_params_on_gather_sink.conn_sup_timeout = MSEC_TO_UNITS(3000, UNIT_10_MS); 
//}



void calculate_peri_to_relay_on_request_params(uint16_t num_req){
    //if(num_req == 0){
    num_req++;
    //}
    int relay_time_buffer =  NRF_SDH_BLE_GAP_EVENT_LENGTH*(count_periodic + count_onreq - count_onreq_not_phone);//0;
    //if(IS_RELAY){
        //relay_time_buffer = 5*count_periodic;
    //}
    peri_to_relay_on_request_conn_params.min_conn_interval = MSEC_TO_UNITS(num_req*(NRF_SDH_BLE_GAP_EVENT_LENGTH+5)*num_req + relay_time_buffer, UNIT_1_25_MS); 
    // 2 times *num_req because each onreq peri sends once for each onreq sends relay does to Phone
    peri_to_relay_on_request_conn_params.max_conn_interval = MSEC_TO_UNITS(num_req*(NRF_SDH_BLE_GAP_EVENT_LENGTH+15)*num_req + relay_time_buffer, UNIT_1_25_MS);
    peri_to_relay_on_request_conn_params.slave_latency = 0;     // No slave latency
    peri_to_relay_on_request_conn_params.conn_sup_timeout = MSEC_TO_UNITS((num_req*(NRF_SDH_BLE_GAP_EVENT_LENGTH+15)*num_req + relay_time_buffer)*10, UNIT_10_MS); //(num_req*150*15, UNIT_10_MS);     // Supervision timeout = 4000 * 10 ms = 40 seconds
}

// DEVICE SPECIFIC CONN PARAMS
//ble_gap_conn_params_t conn_params_test_rssi = {
//    .min_conn_interval = MSEC_TO_UNITS(400, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
//    .max_conn_interval = MSEC_TO_UNITS(400, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
//    .slave_latency = 0,     // No slave latency
//    .conn_sup_timeout = MSEC_TO_UNITS(10000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
//};

//ble_gap_conn_params_t conn_params_test_rssi = {
//    .min_conn_interval = MSEC_TO_UNITS(400, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
//    .max_conn_interval = MSEC_TO_UNITS(400, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
//    .slave_latency = 0,     // No slave latency
//    .conn_sup_timeout = MSEC_TO_UNITS(10000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
//};

//ble_gap_conn_params_t conn_params_test_rssi = {
//    .min_conn_interval = MSEC_TO_UNITS(400, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
//    .max_conn_interval = MSEC_TO_UNITS(400, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
//    .slave_latency = 0,     // No slave latency
//    .conn_sup_timeout = MSEC_TO_UNITS(10000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
//};

//ble_gap_conn_params_t conn_params_test_rssi = {
//    .min_conn_interval = MSEC_TO_UNITS(400, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
//    .max_conn_interval = MSEC_TO_UNITS(400, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
//    .slave_latency = 0,     // No slave latency
//    .conn_sup_timeout = MSEC_TO_UNITS(10000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
//};

// BELOW ARE WHAT ARE ACTUALLY USED
ble_gap_conn_params_t conn_params_BAD = {
    .min_conn_interval = MSEC_TO_UNITS(3000, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    .max_conn_interval = MSEC_TO_UNITS(0, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    .slave_latency = 0,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(0, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};

ble_gap_conn_params_t conn_params_test_rssi = {
    .min_conn_interval = MSEC_TO_UNITS(200, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms // 200 normal test
    .max_conn_interval = MSEC_TO_UNITS(200, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    .slave_latency = 0,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(20000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
    //.min_conn_interval = MSEC_TO_UNITS(100, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    //.max_conn_interval = MSEC_TO_UNITS(100, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    //.slave_latency = 0,     // No slave latency
    //.conn_sup_timeout = MSEC_TO_UNITS(20000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};

ble_gap_conn_params_t conn_params_periodic= {
    .min_conn_interval = MSEC_TO_UNITS(500, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    .max_conn_interval = MSEC_TO_UNITS(1000, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    .slave_latency = 0,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(20000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};

ble_gap_conn_params_t conn_params_idle= {
    .min_conn_interval = MSEC_TO_UNITS(500, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    .max_conn_interval = MSEC_TO_UNITS(1000, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    .slave_latency = 0,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(20000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};

ble_gap_conn_params_t conn_params_conn_phone_wait= {
    .min_conn_interval = MSEC_TO_UNITS(250, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    .max_conn_interval = MSEC_TO_UNITS(250, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    .slave_latency = 0,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(8000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};

ble_gap_conn_params_t conn_params_phone= {
    .min_conn_interval = MSEC_TO_UNITS(400, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    .max_conn_interval = MSEC_TO_UNITS(500, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    .slave_latency = 0,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(20000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};

ble_gap_conn_params_t conn_params_phone_init= {
    .min_conn_interval = MSEC_TO_UNITS(500, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    .max_conn_interval = MSEC_TO_UNITS(1000, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    .slave_latency = 0,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(8000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};

ble_gap_conn_params_t conn_params_stored= {
    .min_conn_interval = MSEC_TO_UNITS(500, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    .max_conn_interval = MSEC_TO_UNITS(1000, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    .slave_latency = 0,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(20000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};

ble_gap_conn_params_t conn_params_maintain_relay= {
    .min_conn_interval = MSEC_TO_UNITS(500, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    .max_conn_interval = MSEC_TO_UNITS(1000, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    .slave_latency = 0,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(20000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};
int conn_periodic_min_interval = 500;
int conn_periodic_max_interval = 2000;
int conn_periodic_sl = 4;
int conn_periodic_timeout = 20000;

ble_gap_conn_params_t conn_params_on_gather_type1= {
    .min_conn_interval = MSEC_TO_UNITS(20, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    .max_conn_interval = MSEC_TO_UNITS(80, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    .slave_latency = 2,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(10000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};

ble_gap_conn_params_t conn_params_on_gather_type1_sink= {
    .min_conn_interval = MSEC_TO_UNITS(20, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    .max_conn_interval = MSEC_TO_UNITS(80, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    .slave_latency = 2,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(10000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};

ble_gap_conn_params_t conn_params_on_gather_type2= {
    .min_conn_interval = MSEC_TO_UNITS(20, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    .max_conn_interval = MSEC_TO_UNITS(80, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    .slave_latency = 2,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(10000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};

ble_gap_conn_params_t conn_params_on_gather_type3= {
    .min_conn_interval = MSEC_TO_UNITS(500, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    .max_conn_interval = MSEC_TO_UNITS(1000, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    .slave_latency = 2,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(15000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};

ble_gap_conn_params_t conn_params_on_gather_sink= {
    .min_conn_interval = MSEC_TO_UNITS(70, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    .max_conn_interval = MSEC_TO_UNITS(110, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    .slave_latency = 2,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(10000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};

ble_gap_conn_params_t conn_params_relay_default= {
    .min_conn_interval = MSEC_TO_UNITS(350, UNIT_1_25_MS),    // 100 * 1.25 ms = 125 ms
    .max_conn_interval = MSEC_TO_UNITS(500, UNIT_1_25_MS),    // 200 * 1.25 ms = 250 ms
    .slave_latency = 0,     // No slave latency
    .conn_sup_timeout = MSEC_TO_UNITS(10000, UNIT_10_MS)          // Supervision timeout = 4000 * 10 ms = 40 seconds
};



bool incl_min_cel_buffer = false;
int event_len_buffer = 15;
bool no_cel_buffer = false;
bool same_cel_buffer = true;

bool once = true;
void get_desired_conn_params(ble_gap_conn_params_t* temp, int min, int max, int sl, int tout){
    //ble_gap_conn_params_t temp= {
        temp->min_conn_interval = MSEC_TO_UNITS(min, UNIT_1_25_MS);    // 100 * 1.25 ms = 125 ms
        temp->max_conn_interval = MSEC_TO_UNITS(max, UNIT_1_25_MS);    // 200 * 1.25 ms = 250 ms
        temp->slave_latency = sl;     // No slave latency
        temp->conn_sup_timeout = MSEC_TO_UNITS(tout, UNIT_10_MS);          // Supervision timeout = 4000 * 10 ms = 40 seconds
        if(once){
            send_usb_data(true, "Conn param min: int=%d, informula=%d, outunits=%d, outmsec=%d",min, (int) min*1.25, temp->min_conn_interval*1000/1000, temp->min_conn_interval * UNIT_1_25_MS / 1000);
            once = false;
        }
    //}; 
    //return temp;
}

void set_periodic_conn_params(int h){
    if(devices[h].peri.type == 1){
          devices[h].periodic_params.min_conn_interval = MSEC_TO_UNITS(480, UNIT_1_25_MS);    // 100 * 1.25 ms = 125 ms
          devices[h].periodic_params.max_conn_interval = MSEC_TO_UNITS(560, UNIT_1_25_MS);    // 200 * 1.25 ms = 250 ms
          devices[h].periodic_params.slave_latency = 4;     // No slave latency
          devices[h].periodic_params.conn_sup_timeout = MSEC_TO_UNITS(20000, UNIT_10_MS);          // Supervision timeout = 4000 * 10 ms = 40 seconds
    }else if(devices[h].peri.type == 2){
          devices[h].periodic_params.min_conn_interval = MSEC_TO_UNITS(1200, UNIT_1_25_MS);    // 100 * 1.25 ms = 125 ms
          devices[h].periodic_params.max_conn_interval = MSEC_TO_UNITS(1600, UNIT_1_25_MS);    // 200 * 1.25 ms = 250 ms
          devices[h].periodic_params.slave_latency = 4;     // No slave latency
          devices[h].periodic_params.conn_sup_timeout = MSEC_TO_UNITS(20000, UNIT_10_MS);          // Supervision timeout = 4000 * 10 ms = 40 seconds
    }
    else if(devices[h].peri.type == 3){
          conn_params_periodic.min_conn_interval = MSEC_TO_UNITS(1200, UNIT_1_25_MS);    // 100 * 1.25 ms = 125 ms
          conn_params_periodic.max_conn_interval = MSEC_TO_UNITS(1600, UNIT_1_25_MS);    // 200 * 1.25 ms = 250 ms
          conn_params_periodic.slave_latency = 4;     // No slave latency
          conn_params_periodic.conn_sup_timeout = MSEC_TO_UNITS(30000, UNIT_10_MS);          // Supervision timeout = 4000 * 10 ms = 40 seconds
    }
}

void set_own_periodic_conn_params(void){
    if(SEND_TYPE == 0){
          conn_params_periodic.min_conn_interval = MSEC_TO_UNITS(320, UNIT_1_25_MS);    // 100 * 1.25 ms = 125 ms
          conn_params_periodic.max_conn_interval = MSEC_TO_UNITS(400, UNIT_1_25_MS);    // 200 * 1.25 ms = 250 ms
          conn_params_periodic.slave_latency = 4;     // No slave latency
          conn_params_periodic.conn_sup_timeout = MSEC_TO_UNITS(20000, UNIT_10_MS);          // Supervision timeout = 4000 * 10 ms = 40 seconds
    }else if(SEND_TYPE == 1){
          conn_params_periodic.min_conn_interval = MSEC_TO_UNITS(480, UNIT_1_25_MS);    // 100 * 1.25 ms = 125 ms
          conn_params_periodic.max_conn_interval = MSEC_TO_UNITS(560, UNIT_1_25_MS);    // 200 * 1.25 ms = 250 ms
          conn_params_periodic.slave_latency = 4;     // No slave latency
          conn_params_periodic.conn_sup_timeout = MSEC_TO_UNITS(20000, UNIT_10_MS);          // Supervision timeout = 4000 * 10 ms = 40 seconds
    }else if(SEND_TYPE == 2){
          conn_params_periodic.min_conn_interval = MSEC_TO_UNITS(1200, UNIT_1_25_MS);    // 100 * 1.25 ms = 125 ms
          conn_params_periodic.max_conn_interval = MSEC_TO_UNITS(1600, UNIT_1_25_MS);    // 200 * 1.25 ms = 250 ms
          conn_params_periodic.slave_latency = 2;    // No slave latency
          conn_params_periodic.conn_sup_timeout = MSEC_TO_UNITS(20000, UNIT_10_MS);          // Supervision timeout = 4000 * 10 ms = 40 seconds
    }
}

void set_conn_time_buffers(bool use_min_cel_buffer, bool no_min_max_buffers, bool both_buffers_max, int buffer_size){
    incl_min_cel_buffer = use_min_cel_buffer;
    no_cel_buffer = no_min_max_buffers;
    same_cel_buffer = both_buffers_max;
    event_len_buffer = buffer_size;
}

void calculate_relay_params(){
    //num_req++;
    
    int min_buffer = 20;
    int new_event_len_buffer = 20;

    if(incl_min_cel_buffer){
        min_buffer = event_len_buffer;
        new_event_len_buffer = 2*event_len_buffer;
    }
    if(no_cel_buffer){
        min_buffer = 0;
        new_event_len_buffer = 0;
    }

    if(same_cel_buffer){
        min_buffer = event_len_buffer;
        new_event_len_buffer = event_len_buffer;
    }

    int cel_max = NRF_SDH_BLE_GAP_EVENT_LENGTH;

    //int is_rel_sn = 0;
    //if(IS_SENSOR_NODE && SEND_TYPE == 3){
    //    is_rel_sn = 1;
    //}

    int tot_events;
    //int rel_sink = 1;//for T2
    int t1_relay = relay_info.count_type1 - relay_info.count_type1_sink;
    //int t23_relay = 1;
    //if(relay_info.count_type2 == 0 && relay_info.count_type3==0){
    //    t23_relay = 0;
    //}
    int t1_sink = relay_info.count_type1_sink;
    int sn_relay = 1 + t1_relay;
    int rel_sink = sn_relay;
    sn_relay = sn_relay - t1_sink;
    if(sn_relay < 0){
        sn_relay = 0;
    }

    int min_tot_events = t1_sink + sn_relay + rel_sink;
    // 2+ t1, e.g. 2 to R and others to S:
    // Or 1 t1 use R other use S:
    // Fill up empty sink windows with direct T1s
    //                                     T1S, 
    //                                     T1R, RS, T1R, RS, T2R, RS
    //Change to:                           T1S,   , T1S,   , T1S   
    //                                     T1R, RS, T1R, RS, T2R, RS
    int empty_sink_windows = min_tot_events - rel_sink - t1_sink;
    int min_t1_sink_freq = min_tot_events;
    if(rel_sink > t1_sink && empty_sink_windows % t1_sink == 0){
        min_t1_sink_freq = min_tot_events / (1 + empty_sink_windows / t1_sink);
    }

    int min_rel_sink_freq = min_tot_events / rel_sink;
    //in case of 1 
    //in case of more than 2 t1
    //if(rel_sink != t1_sink)

    tlv_advertiseSink[0] = 9;
    tlv_advertiseSink[1] = min_t1_sink_freq;
    tlv_advertiseSink[2] = min_buffer;
    tlv_advertiseSink[3] = new_event_len_buffer;

    // TYPE 1 to Sink conn params
    conn_params_on_gather_type1_sink.min_conn_interval = MSEC_TO_UNITS(
            //min_t1_sink_freq*(cel_max + min_buffer), UNIT_1_25_MS);//=100-120 ms
            min_tot_events*(cel_max + min_buffer), UNIT_1_25_MS);//=100-120 ms
    conn_params_on_gather_type1_sink.max_conn_interval = MSEC_TO_UNITS(
            //min_t1_sink_freq*(cel_max + new_event_len_buffer), UNIT_1_25_MS);//=125-150ms
            min_tot_events*(cel_max + new_event_len_buffer), UNIT_1_25_MS);//=125-150ms
    conn_params_on_gather_type1_sink.slave_latency = 2;     // No slave latency
    conn_params_on_gather_type1_sink.conn_sup_timeout = MSEC_TO_UNITS(10000, UNIT_10_MS); 
    //(num_req*150*15, UNIT_10_MS);     // Supervision timeout = 4000 * 10 ms = 40 seconds
    send_usb_data(false, "T1-Sink params: min=%d, max=%d, latency=%d, timeout=%d",
                                conn_params_on_gather_type1_sink.min_conn_interval * UNIT_1_25_MS / 1000,
                                conn_params_on_gather_type1_sink.max_conn_interval * UNIT_1_25_MS / 1000,
                                conn_params_on_gather_type1_sink.slave_latency,
                                conn_params_on_gather_type1_sink.conn_sup_timeout * UNIT_10_MS / 1000);

    // TYPE 1 conn params
    conn_params_on_gather_type1.min_conn_interval = MSEC_TO_UNITS(
            min_tot_events*(cel_max + min_buffer), UNIT_1_25_MS);//=100-120 ms
    conn_params_on_gather_type1.max_conn_interval = MSEC_TO_UNITS(
            min_tot_events*(cel_max + new_event_len_buffer), UNIT_1_25_MS);//=125-150ms
    conn_params_on_gather_type1.slave_latency = 2;     // No slave latency
    conn_params_on_gather_type1.conn_sup_timeout = MSEC_TO_UNITS(10000, UNIT_10_MS); 
    send_usb_data(false, "Relay-T1 params: min=%d, max=%d, latency=%d, timeout=%d",
                                conn_params_on_gather_type1.min_conn_interval * UNIT_1_25_MS / 1000,
                                conn_params_on_gather_type1.max_conn_interval * UNIT_1_25_MS / 1000,
                                conn_params_on_gather_type1.slave_latency,
                                conn_params_on_gather_type1.conn_sup_timeout * UNIT_10_MS / 1000);
    // TYPE 2 conn params
    conn_params_on_gather_type2.min_conn_interval = MSEC_TO_UNITS(
            relay_info.count_type2*min_tot_events*(cel_max + min_buffer), UNIT_1_25_MS);//=100-120 ms
    conn_params_on_gather_type2.max_conn_interval = MSEC_TO_UNITS(
            relay_info.count_type2*min_tot_events*(cel_max + new_event_len_buffer), UNIT_1_25_MS);//=125-150ms
    conn_params_on_gather_type2.slave_latency = 0;     // No slave latency
    conn_params_on_gather_type2.conn_sup_timeout = MSEC_TO_UNITS(10000, UNIT_10_MS); 
    send_usb_data(false, "Relay-T2 params: min=%d, max=%d, latency=%d, timeout=%d",
                                conn_params_on_gather_type2.min_conn_interval * UNIT_1_25_MS / 1000,
                                conn_params_on_gather_type2.max_conn_interval * UNIT_1_25_MS / 1000,
                                conn_params_on_gather_type2.slave_latency,
                                conn_params_on_gather_type2.conn_sup_timeout * UNIT_10_MS / 1000);

    // TYPE 3 conn params
    //conn_params_on_gather_type3.min_conn_interval = MSEC_TO_UNITS(
    //        7*min_tot_events*(cel_max + min_buffer), UNIT_1_25_MS);//=100-120 ms
    //conn_params_on_gather_type3.max_conn_interval = MSEC_TO_UNITS(
    //        7*min_tot_events*(cel_max + new_event_len_buffer), UNIT_1_25_MS);//=125-150ms
    //conn_params_on_gather_type3.slave_latency = 2;     // No slave latency
    //conn_params_on_gather_type3.conn_sup_timeout = MSEC_TO_UNITS(15000, UNIT_10_MS); 
    //send_usb_data(false, "Relay-T3 params: min=%d, max=%d, latency=%d, timeout=%d",
    //                            conn_params_on_gather_type3.min_conn_interval * UNIT_1_25_MS / 1000,
    //                            conn_params_on_gather_type3.max_conn_interval * UNIT_1_25_MS / 1000,
    //                            conn_params_on_gather_type3.slave_latency,
    //                            conn_params_on_gather_type3.conn_sup_timeout * UNIT_10_MS / 1000);

    // Sink conn params
    conn_params_on_gather_sink.min_conn_interval = MSEC_TO_UNITS(
            min_rel_sink_freq*(cel_max + min_buffer), UNIT_1_25_MS);//=100-120 ms
    conn_params_on_gather_sink.max_conn_interval = MSEC_TO_UNITS(
            min_rel_sink_freq*(cel_max + new_event_len_buffer), UNIT_1_25_MS);//=125-150ms
    conn_params_on_gather_sink.slave_latency = 2;     // No slave latency
    conn_params_on_gather_sink.conn_sup_timeout = MSEC_TO_UNITS(10000, UNIT_10_MS); 
    send_usb_data(false, "Relay-Sink params: min=%d, max=%d, latency=%d, timeout=%d",
                                conn_params_on_gather_sink.min_conn_interval * UNIT_1_25_MS / 1000,
                                conn_params_on_gather_sink.max_conn_interval * UNIT_1_25_MS / 1000,
                                conn_params_on_gather_sink.slave_latency,
                                conn_params_on_gather_sink.conn_sup_timeout * UNIT_10_MS / 1000);

    for(int i = 0; i<NRF_SDH_BLE_TOTAL_LINK_COUNT; i++){
        if(devices[i].connected){
            if(devices[i].peri.type == 1){
                devices[i].conn_params = conn_params_on_gather_type1;
            }else if(devices[i].peri.type == 2){
                devices[i].conn_params = conn_params_on_gather_type2;
            }
            //else if(devices[i].peri.type == 3){
            //    devices[i].conn_params = conn_params_on_gather_type3;
            //}
            else if(devices[i].cent.isPhone){
                devices[i].conn_params = conn_params_on_gather_sink;
            }
        }
    }
}

void calculate_t1_sink_params(uint16_t min_freq, int min_buffer, int max_buffer){
    int cel_max = NRF_SDH_BLE_GAP_EVENT_LENGTH;
    
    // Sink conn params
    conn_params_on_gather_sink.min_conn_interval = MSEC_TO_UNITS(
            min_freq*(cel_max + min_buffer), UNIT_1_25_MS);//=100-120 ms
    conn_params_on_gather_sink.max_conn_interval = MSEC_TO_UNITS(
            min_freq*(cel_max + max_buffer), UNIT_1_25_MS);//=125-150ms
    conn_params_on_gather_sink.slave_latency = 0;     // No slave latency
    conn_params_on_gather_sink.conn_sup_timeout = MSEC_TO_UNITS(10000, UNIT_10_MS); 
    send_usb_data(false, "Relay-Sink params: min=%d, max=%d, latency=%d, timeout=%d",
                                conn_params_on_gather_sink.min_conn_interval * UNIT_1_25_MS / 1000,
                                conn_params_on_gather_sink.max_conn_interval * UNIT_1_25_MS / 1000,
                                conn_params_on_gather_sink.slave_latency,
                                conn_params_on_gather_sink.conn_sup_timeout * UNIT_10_MS / 1000);
}

uint16_t conn_update_timer_handle = 20;
triggered_conn_update_t conn_update_timer_params;

bool update_conn_params(uint16_t handle, ble_gap_conn_params_t params){
    
    send_usb_data(true, "Try connParam update to %s; h=%d: min=%d, max=%d, latency=%d, timeout=%d, current updating=%d", 
                                devices[handle].name, handle,
                                params.min_conn_interval * UNIT_1_25_MS / 1000,
                                params.max_conn_interval * UNIT_1_25_MS / 1000,
                                params.slave_latency,
                                params.conn_sup_timeout * UNIT_10_MS / 1000,
                                devices[handle].current_updating_conn_params.params.max_conn_interval*UNIT_1_25_MS/1000);

    //send_usb_data(true, "WaitingConnUpdate = %d: if Min=%d, if Max=%d, if Latency=%d, if Timeout=%d", 
    //                            devices[handle].waiting_conn_update,
    //                            devices[handle].current_updating_conn_params.max_conn_interval == params.max_conn_interval,
    //                            devices[handle].current_updating_conn_params.min_conn_interval == params.min_conn_interval,
    //                            devices[handle].current_updating_conn_params.slave_latency == params.slave_latency,
    //                            devices[handle].current_updating_conn_params.conn_sup_timeout == params.conn_sup_timeout);

    if(devices[handle].waiting_conn_update){
        //if(devices[handle].current_updating_conn_params.params.max_conn_interval == params.max_conn_interval ||
        //      devices[handle].current_updating_conn_params.params.min_conn_interval == params.min_conn_interval ||
        //      devices[handle].current_updating_conn_params.params.slave_latency == params.slave_latency)
        if(compare_conn_params(&devices[handle].current_updating_conn_params.params, &params))
        {
            send_usb_data(true, "Current updating are same, no need for update");
            return false;
        }else if (//devices[handle].queued_conn_params.params.max_conn_interval == params.max_conn_interval ||
              //devices[handle].queued_conn_params.params.min_conn_interval == params.min_conn_interval ||
              //devices[handle].queued_conn_params.params.slave_latency == params.slave_latency)
              compare_conn_params(&devices[handle].queued_conn_params.params, &params))
        {
            send_usb_data(true, "Current queued params are same, no need for change");
            return false;
        }else{
            send_usb_data(true, "Params queued.");
            copy_conn_params(&devices[handle].queued_conn_params.params, &params);
            devices[handle].params_queued = true;
            devices[handle].current_updating_conn_params.do_tlv = false;
            memcpy(devices[handle].queued_conn_params.tlv, empty_tlv,0);
            devices[handle].queued_conn_params.len = 0;
        }
    }else{
        devices[handle].waiting_conn_update = true;
        devices[handle].params_queued = false;
        devices[handle].queued_conn_params.do_tlv = false;
        //devices[handle].conn_params = params;
        //copy_conn_params(&devices[handle].conn_params, &params);
        memcpy(devices[handle].queued_conn_params.tlv, empty_tlv,0);
        devices[handle].queued_conn_params.len = 0;
        copy_conn_params(&devices[handle].current_updating_conn_params.params, &params);
        //devices[handle].current_updating_conn_params.params = params;
        send_usb_data(true, "Sending direct connParam Request to %s; h=%d: min=%d, max=%d, latency=%d, timeout=%d, told=%d", 
                                devices[handle].name, handle,
                                params.min_conn_interval * UNIT_1_25_MS / 1000,
                                params.max_conn_interval * UNIT_1_25_MS / 1000,
                                params.slave_latency,
                                params.conn_sup_timeout * UNIT_10_MS / 1000,
                                params.conn_sup_timeout);

        sd_ble_gap_conn_param_update(handle, &params);
        
    }
    return true;
}

bool tlvs_same(uint8_t * t1, int l1, uint8_t * t2, int l2){
    int l = l1;
    if(l2 != l1){
        return  false;
    }
    for(int i = 0; i < l; i++){
        if(t1[i] != t2[i]){
            return  false;
        }
    }
    return  true;
}
bool update_conn_params_send_tlv(uint16_t handle, ble_gap_conn_params_t params, uint8_t * tlv, int len, bool force){
    if(len == 0){
        send_usb_data(true, "Pass to no tlv len=%d", len);
        return update_conn_params(handle, params);
    }
    send_usb_data(true, "Try TLV connParam update to %s; h=%d: min=%d, max=%d, latency=%d, timeout=%d, current updating=%d", 
                                devices[handle].name, handle,
                                params.min_conn_interval * UNIT_1_25_MS / 1000,
                                params.max_conn_interval * UNIT_1_25_MS / 1000,
                                params.slave_latency,
                                params.conn_sup_timeout * UNIT_10_MS / 1000,
                                devices[handle].current_updating_conn_params.params.max_conn_interval*UNIT_1_25_MS/1000);
    send_tlv_usb(false, "request has tlv: ", tlv, len, len, handle);

    //send_usb_data(true, "WaitingConnUpdate = %d: if Min=%d, if Max=%d, if Latency=%d, if Timeout=%d", 
    //                            devices[handle].waiting_conn_update,
    //                            devices[handle].current_updating_conn_params.max_conn_interval == params.max_conn_interval,
    //                            devices[handle].current_updating_conn_params.min_conn_interval == params.min_conn_interval,
    //                            devices[handle].current_updating_conn_params.slave_latency == params.slave_latency,
    //                            devices[handle].current_updating_conn_params.conn_sup_timeout == params.conn_sup_timeout);

    if(devices[handle].waiting_conn_update){
        if(//(devices[handle].current_updating_conn_params.params.max_conn_interval == params.max_conn_interval ||
              //devices[handle].current_updating_conn_params.params.min_conn_interval == params.min_conn_interval ||
              //devices[handle].current_updating_conn_params.params.slave_latency == params.slave_latency) &&
              compare_conn_params(&devices[handle].current_updating_conn_params.params, &params) &&
              tlvs_same(devices[handle].current_updating_conn_params.tlv, devices[handle].current_updating_conn_params.len, tlv, len))
        {
            send_usb_data(true, "Current updating are same, no need for update");
            return false;
        }else if(force){
            if(devices[handle].params_queued){
                send_usb_data(true, "Current queued params FORCED replaced, min:%d, tlv?=%d",devices[handle].queued_conn_params.params.min_conn_interval*UNIT_1_25_MS/1000, devices[handle].queued_conn_params.do_tlv);
                if(devices[handle].queued_conn_params.do_tlv){
                    send_tlv_usb(false, "Replaced tlv: ", devices[handle].queued_conn_params.tlv, devices[handle].queued_conn_params.len, devices[handle].queued_conn_params.len, handle);
                    //send_tlv_usb(false, "New Queued tlv: ", tlv, len, len, handle);
                }
            }
            send_usb_data(true, "Params queued.");
            copy_conn_params(&devices[handle].queued_conn_params.params, &params);
            devices[handle].params_queued = true;
            memcpy(devices[handle].queued_conn_params.tlv, tlv,len);
            devices[handle].queued_conn_params.len = len;
            devices[handle].queued_conn_params.do_tlv = true;
            return true;
        }else if (//(devices[handle].queued_conn_params.params.max_conn_interval == params.max_conn_interval ||
              //devices[handle].queued_conn_params.params.min_conn_interval == params.min_conn_interval ||
              //devices[handle].queued_conn_params.params.slave_latency == params.slave_latency) &&
              compare_conn_params(&devices[handle].queued_conn_params.params, &params) &&
              tlvs_same(devices[handle].queued_conn_params.tlv, devices[handle].queued_conn_params.len, tlv, len))
        {
            send_usb_data(true, "Current queued params are same, no need for change");
            return false;
        }else{
            if(devices[handle].params_queued){
                send_usb_data(true, "Current queued params replaced, min:%d, tlv?=%d",devices[handle].queued_conn_params.params.min_conn_interval*UNIT_1_25_MS/1000, devices[handle].queued_conn_params.do_tlv);
                if(devices[handle].queued_conn_params.do_tlv){
                    send_tlv_usb(false, "Replaced tlv: ", devices[handle].queued_conn_params.tlv, devices[handle].queued_conn_params.len, devices[handle].queued_conn_params.len, handle);
                    //send_tlv_usb(false, "New Queued tlv: ", tlv, len, len, handle);
                }
            }
            send_usb_data(true, "Params queued.");
            //devices[handle].queued_conn_params.params = params;
            copy_conn_params(&devices[handle].queued_conn_params.params, &params);
            devices[handle].params_queued = true;
            memcpy(devices[handle].queued_conn_params.tlv, tlv,len);
            devices[handle].queued_conn_params.len = len;
            devices[handle].queued_conn_params.do_tlv = true;
        }
    }else{
        devices[handle].waiting_conn_update = true;
        devices[handle].params_queued = false;
        //devices[handle].conn_params = params;
        memcpy(devices[handle].current_updating_conn_params.tlv, tlv,len);
        devices[handle].current_updating_conn_params.len = len;
        devices[handle].current_updating_conn_params.do_tlv = true;
        copy_conn_params(&devices[handle].current_updating_conn_params.params, &params);
        send_usb_data(true, "Sending normally direct connParam Request to %s; h=%d: min=%d, max=%d, latency=%d, timeout=%d, told=%d", 
                                devices[handle].name, handle,
                                params.min_conn_interval * UNIT_1_25_MS / 1000,
                                params.max_conn_interval * UNIT_1_25_MS / 1000,
                                params.slave_latency,
                                params.conn_sup_timeout * UNIT_10_MS / 1000,
                                params.conn_sup_timeout);

        sd_ble_gap_conn_param_update(handle, &params);
        
    }
    return true;
}

void update_type_periodic_conn_params(uint16_t handle){
    if(devices[handle].peri.type == 1){
        update_conn_params(handle, conn_params_on_gather_type1);
    }else if(devices[handle].peri.type == 2){
        update_conn_params(handle, conn_params_on_gather_type2);
    }else if(devices[handle].peri.type == 3){
        //update_conn_params(handle, conn_params_on_gather_type3);
        update_conn_params(handle, devices[handle].periodic_params);
    }
}
bool start_fast_relay_ready = false;

void clear_updates_for_handle(int handle){
    memcpy(devices[handle].current_updating_conn_params.tlv, empty_tlv, 0);
    devices[handle].current_updating_conn_params.len = 0;
    devices[handle].current_updating_conn_params.do_tlv = false;
    devices[handle].waiting_conn_update = false;

    memcpy(devices[handle].queued_conn_params.tlv, empty_tlv, 0);
    devices[handle].queued_conn_params.len = 0;
    devices[handle].queued_conn_params.do_tlv = false;
    devices[handle].params_queued = false;

    copy_conn_params(&devices[handle].queued_conn_params.params, &conn_params_BAD);
    copy_conn_params(&devices[handle].current_updating_conn_params.params, &conn_params_BAD);
}

bool check_update(int h){
    int handle =h;
    //if(devices[h].peri.type > 0){
    //    if(devices[h].current_updating_conn_params.len == 1 && devices[h].current_updating_conn_params.tlv[0] == 11
    //}
    if(devices[h].waiting_conn_update){
        if(devices[h].current_updating_conn_params.do_tlv){
            if(tlvs_same(fast_tlv, 1, devices[handle].current_updating_conn_params.tlv, devices[handle].current_updating_conn_params.len) && !devices[h].peri.sent_start_send  && (!devices[h].peri.sending_periodic || !devices[h].peri.allDataSentFromPeri)){
                if(handle == RELAY_HANDLE){
                    tlv_start_data[0] = 0;
                    send_tlv_from_c(h, tlv_start_data, 2, "Rel start send data");
                    send_tlv_usb(false, "Sending START1 on update: ", tlv_start_data, 2, 2, h);
                }else{
                    tlv_start_data[0] = 1;
                    send_tlv_from_c(h, tlv_start_data, 2, "Periph start send data");
                    send_tlv_usb(false, "Sending START1 on update: ", tlv_start_data, 2, 2, h);
                }
                tlv_start_data[0] = 0;
                devices[h].peri.sent_start_send = true;
            }
            
            if(tlvs_same(fast_tlv, 1, devices[handle].current_updating_conn_params.tlv, devices[handle].current_updating_conn_params.len) ){
                if(!devices[h].peri.sent_send_fast && (!devices[h].peri.sending_periodic || !devices[h].peri.allDataSentFromPeri)){
                    devices[h].peri.sent_send_fast = true;
                    send_tlv_from_c(h, devices[handle].current_updating_conn_params.tlv, devices[handle].current_updating_conn_params.len, "Sending FAST on update");
                }
            }else if(devices[handle].current_updating_conn_params.len == 2 && (devices[handle].current_updating_conn_params.tlv[0] == 0 || devices[handle].current_updating_conn_params.tlv[0] == 1) && devices[handle].current_updating_conn_params.tlv[1] == DEVICE_ID){
                if(!devices[h].peri.sent_start_send){
                    devices[h].peri.sent_start_send = true;
                    if(handle == RELAY_HANDLE){
                        tlv_start_data[0] = 0;
                        send_tlv_from_c(h, tlv_start_data, 2, "Rel start send data");
                        send_tlv_usb(false, "Sending START2 on update: ", tlv_start_data, 2, 2, h);
                    }else{
                        tlv_start_data[0] = 1;
                        send_tlv_from_c(h, tlv_start_data, 2, "Periph start send data");
                        send_tlv_usb(false, "Sending START2 on update: ", tlv_start_data, 2, 2, h);
                    }
                    tlv_start_data[0] = 0;
                }
            }else{
                send_tlv_from_c(h, devices[handle].current_updating_conn_params.tlv, devices[handle].current_updating_conn_params.len, "Sending on update");
            }
            memset(devices[handle].current_updating_conn_params.tlv, 0, 250);
            devices[handle].current_updating_conn_params.len = 0;
            devices[handle].current_updating_conn_params.do_tlv = false;
        }
        devices[h].waiting_conn_update = false;
        if(devices[h].params_queued){
            copy_conn_params(&devices[handle].current_updating_conn_params.params, &devices[handle].queued_conn_params.params);
            devices[handle].waiting_conn_update = true;
            devices[handle].params_queued = false;
            copy_conn_params(&devices[handle].queued_conn_params.params, &conn_params_BAD);
            
            if(devices[h].queued_conn_params.do_tlv){
                memcpy(devices[handle].current_updating_conn_params.tlv, devices[handle].queued_conn_params.tlv, devices[handle].queued_conn_params.len);
                devices[handle].current_updating_conn_params.len = devices[handle].queued_conn_params.len;
                devices[handle].current_updating_conn_params.do_tlv = true;

                memset(devices[handle].current_updating_conn_params.tlv, 0, 250);
                devices[handle].queued_conn_params.len = 0;
                devices[handle].queued_conn_params.do_tlv = false;
            }else{
                memset(devices[handle].current_updating_conn_params.tlv, 0, 250);
                devices[handle].current_updating_conn_params.len = 0;
                devices[handle].current_updating_conn_params.do_tlv = false;
            }
            send_usb_data(true, "Sending QUEUED connParam Request to %s; h=%d: min=%d, max=%d, latency=%d, timeout=%d, told=%d", 
                                devices[handle].name, handle,
                                devices[handle].current_updating_conn_params.params.min_conn_interval * UNIT_1_25_MS / 1000,
                                devices[handle].current_updating_conn_params.params.max_conn_interval * UNIT_1_25_MS / 1000,
                                devices[handle].current_updating_conn_params.params.slave_latency,
                                devices[handle].current_updating_conn_params.params.conn_sup_timeout * UNIT_10_MS / 1000,
                                devices[handle].current_updating_conn_params.params.conn_sup_timeout);
            sd_ble_gap_conn_param_update(handle, &devices[handle].current_updating_conn_params.params);
            return true;
        }
    }
    //copy_conn_params(&devices[handle].current_updating_conn_params.params, &conn_params_BAD);
    clear_updates_for_handle(handle);
    return false;
}

bool once2 = true;
void copy_conn_params(ble_gap_conn_params_t* temp, ble_gap_conn_params_t* temp2){
    //ble_gap_conn_params_t temp= {
        temp->min_conn_interval =temp2->min_conn_interval;// MSEC_TO_UNITS(min, UNIT_1_25_MS);    // 100 * 1.25 ms = 125 ms
        temp->max_conn_interval = temp2->max_conn_interval;//MSEC_TO_UNITS(max, UNIT_1_25_MS);    // 200 * 1.25 ms = 250 ms
        temp->slave_latency = temp2->slave_latency;//sl;     // No slave latency
        temp->conn_sup_timeout = temp2->conn_sup_timeout;//MSEC_TO_UNITS(tout, UNIT_10_MS);          // Supervision timeout = 4000 * 10 ms = 40 seconds
        if(once2 && devices[RELAY_HANDLE].peri.mode == 1){
            send_usb_data(true, "Copy PARAMS min: t1min=%d, t2min=%d, t1==t2=%d, CompareF()=%d",temp->min_conn_interval*UNIT_1_25_MS/1000, temp2->min_conn_interval * UNIT_1_25_MS / 1000, temp->min_conn_interval==temp2->min_conn_interval, compare_conn_params(temp, temp2));
            once2 = false;
        }
    //}; 
    //return temp;
}

void copy_conn_params2(ble_gap_conn_params_t* temp, ble_gap_conn_params_t* temp2){
    //ble_gap_conn_params_t temp= {
        temp->min_conn_interval =MSEC_TO_UNITS(temp2->min_conn_interval*UNIT_1_25_MS/1000, UNIT_1_25_MS);// MSEC_TO_UNITS(min, UNIT_1_25_MS);    // 100 * 1.25 ms = 125 ms
        temp->max_conn_interval = MSEC_TO_UNITS(temp2->max_conn_interval*UNIT_1_25_MS/1000, UNIT_1_25_MS);//MSEC_TO_UNITS(max, UNIT_1_25_MS);    // 200 * 1.25 ms = 250 ms
        temp->slave_latency = temp2->slave_latency;//sl;     // No slave latency
        temp->conn_sup_timeout = MSEC_TO_UNITS(temp2->conn_sup_timeout*UNIT_10_MS/1000, UNIT_10_MS);;//MSEC_TO_UNITS(tout, UNIT_10_MS);          // Supervision timeout = 4000 * 10 ms = 40 seconds
        if(once2 && devices[RELAY_HANDLE].peri.mode == 1){
            send_usb_data(true, "Copy PARAMS min: t1min=%d, t2min=%d, t1==t2=%d, CompareF()=%d",temp->min_conn_interval*UNIT_1_25_MS/1000, temp2->min_conn_interval * UNIT_1_25_MS / 1000, temp->min_conn_interval==temp2->min_conn_interval, compare_conn_params(temp, temp2));
            once2 = false;
        }
    //}; 
    //return temp;
}

bool compare_conn_params(ble_gap_conn_params_t* temp, ble_gap_conn_params_t* temp2){
    return temp->min_conn_interval == temp2->min_conn_interval && temp->max_conn_interval == temp2->max_conn_interval && temp->slave_latency == temp2->slave_latency;
}

uint8_t empty_tlv[1] = {};
bool updating_relay_conns = false;
bool update_all_relay_conn_tlv(uint8_t * tlv, int len){
    send_usb_data(true, "at update_all_relay_conn_tlv");
    if(!connectedToPhone && false){//!updated_phone_conn){
        //updated_phone_conn = true;
        //update_conn_params(PHONE_HANDLE, devices[PHONE_HANDLE].conn_params);
        send_usb_data(true, "go to update_conn_params, return true");
        //return true;
    }

    for (uint8_t i=0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
       // send_usb_data(true, "Update all TRY %s, minint=%d, tlv=%d, type=%d, mode=%d",devices[i].name, devices[i].conn_params.min_conn_interval*1250/1000, len>0, devices[i].peri.type, devices[i].peri.mode);
       //send_usb_data(true, "type=%d, !updated_conn=%d, !asked=%d, !chosen=%d", devices[i].peri.type, !devices[i].updated_conn,  !devices[i].peri.askedAdvPhone, devices[i].peri.chosenRelay);
            
        if(isPhone && devices[i].peri.type == 0 && !devices[i].updated_conn && devices[i].connected && !devices[i].peri.finished && !devices[i].peri.askedAdvPhone && devices[i].peri.chosenRelay){
            //send_usb_data(true, "Update all FINAL %s, minint=%d, tlv=%d",devices[i].name, devices[i].conn_params.min_conn_interval*1250/1000, len>0);
            // send_usb_data(true, "type=%d, !updated_conn=%d, !asked=%d, !chosen=%d", devices[i].peri.type, !devices[i].updated_conn,  !devices[i].peri.askedAdvPhone, devices[i].peri.chosenRelay);
            if(!devices[i].peri.sending_periodic){
                if(devices[i].peri.mode != 1){
                    set_other_device_op_mode(i, 1);
                }
                send_usb_data(true, "go to update conn param tlv");
                //devices[i].updated_conn = 
                update_conn_params_send_tlv(i, devices[i].conn_params, tlv, len, true);
                memcpy(devices[i].relay_all_upd_params.tlv, tlv,len);
                devices[i].relay_all_upd_params.len = len;
                devices[i].relay_all_upd_params.do_tlv = true;
                devices[i].relay_all_upd_params.params = devices[i].conn_params;
            }else{
                set_other_device_op_mode(i, 2);
                //devices[i].updated_conn = 
                update_conn_params(i, devices[i].conn_params);
            }
            devices[i].updated_conn = true;
            if(devices[i].tp.use_pdr){
                devices[i].tp.stopped_pdr = true;
                devices[i].tp.use_pdr = false;
            }
            
            return true;
        }
    }
    for (uint8_t i=0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
        if(devices[i].peri.type == 1 && !devices[i].updated_conn && devices[i].connected && !devices[i].peri.finished && !devices[i].peri.askedAdvPhone && devices[i].peri.chosenRelay ){
            if(!devices[i].peri.sending_periodic && isPhone){
                if(devices[i].peri.mode != 1){
                    set_other_device_op_mode(i, 1);
                }
                update_conn_params_send_tlv(i, devices[i].conn_params, tlv, len, true);
                memcpy(devices[i].relay_all_upd_params.tlv, tlv,len);
                devices[i].relay_all_upd_params.len = len;
                devices[i].relay_all_upd_params.do_tlv = true;
                devices[i].relay_all_upd_params.params = devices[i].conn_params;
            }else{
                set_other_device_op_mode(i, 2);
                update_conn_params(i, devices[i].conn_params);
            }
            devices[i].updated_conn = true;
            if(devices[i].tp.use_pdr){
                devices[i].tp.stopped_pdr = true;
                devices[i].tp.use_pdr = false;
            }
            return true;
        }
    }
    for (uint8_t i=0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
        if(devices[i].peri.type == 2 && !devices[i].updated_conn && devices[i].connected && !devices[i].peri.finished && !devices[i].peri.askedAdvPhone && devices[i].peri.chosenRelay){
            if(!devices[i].peri.sending_periodic && isPhone){
                if(devices[i].peri.mode != 1){
                    set_other_device_op_mode(i, 1);
                }
                update_conn_params_send_tlv(i, devices[i].conn_params, tlv, len, true);
                memcpy(devices[i].relay_all_upd_params.tlv, tlv,len);
                devices[i].relay_all_upd_params.len = len;
                devices[i].relay_all_upd_params.do_tlv = true;
                devices[i].relay_all_upd_params.params = devices[i].conn_params;
            }else{
                set_other_device_op_mode(i, 2);
                update_conn_params(i, devices[i].conn_params);
            }
            devices[i].updated_conn = true;
            if(devices[i].tp.use_pdr){
                devices[i].tp.stopped_pdr = true;
                devices[i].tp.use_pdr = false;
            }
            return true;
        }
    }

    for (uint8_t i=0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
        if(!devices[i].updated_conn && devices[i].connected  &&  !devices[i].peri.chosenRelay && !devices[i].id == 'M'){
            set_other_device_op_mode(i, 3);
            update_conn_params(i, devices[i].conn_params);
            devices[i].updated_conn = true;
            if(devices[i].tp.use_pdr){
                devices[i].tp.stopped_pdr = true;
                devices[i].tp.use_pdr = false;
            }
            return true;
        }
    }

    for (uint8_t i=0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
        if(devices[i].peri.type == 3 && !devices[i].updated_conn && devices[i].connected  && !devices[i].peri.askedAdvPhone && devices[i].peri.chosenRelay){
            if(!devices[i].peri.sending_periodic){
               if(devices[i].peri.mode != 1){
                    set_other_device_op_mode(i, 1);
                }
            }else{
                set_other_device_op_mode(i, 2);
            }
            update_conn_params(i, devices[i].conn_params);
            devices[i].updated_conn = true;
            if(devices[i].tp.use_pdr){
                devices[i].tp.stopped_pdr = true;
                devices[i].tp.use_pdr = false;
            }
            return true;
        }
    }
    
    //if(lastToUpdate < NRF_SDH_BLE_CENTRAL_LINK_COUNT){
    //    update_conn_params(lastToUpdate, conn_params_on_gather_type1);
    //}
    for (uint8_t i=0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
        devices[i].updated_conn = false;
        memcpy(devices[i].relay_all_upd_params.tlv, empty_tlv,len);
        devices[i].relay_all_upd_params.len = 0;
        devices[i].relay_all_upd_params.do_tlv = false;
        devices[i].relay_all_upd_params.params = conn_params_BAD;
    }
    updated_phone_conn = false;
    send_usb_data(false, "finished update all");
    return false;
}



bool updating_ongather_conn = false;
bool update_all_conn_for_gather(void){
    for (uint8_t i=0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
        if(devices[i].peri.type == 1 && !devices[i].updated_conn && devices[i].connected && !devices[i].peri.finished && !devices[i].peri.askedAdvPhone){
            //!devices[i].peri.connToPhone && 
            //devices[i].conn_params.min_conn_interval != conn_params_on_gather_type1.min_conn_interval && 
            //devices[i].conn_params.max_conn_interval != conn_params_on_gather_type1.max_conn_interval &&
            //i != lastToUpdate){
            devices[i].updated_conn = true;
            update_conn_params(i, devices[i].conn_params);
            return true;
        }
    }
    for (uint8_t i=0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
        if(devices[i].peri.type == 2 && !devices[i].updated_conn && devices[i].connected && !devices[i].peri.finished && !devices[i].peri.askedAdvPhone){
            update_conn_params(i, devices[i].conn_params);
            devices[i].updated_conn = true;
            return true;
        }
    }
    for (uint8_t i=0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
        if(devices[i].peri.type == 3 && !devices[i].updated_conn && devices[i].connected && !devices[i].peri.finished && !devices[i].peri.askedAdvPhone){
            
            update_conn_params(i, devices[i].conn_params);
            devices[i].updated_conn = true;
            return true;
        }
    }
    for (uint8_t i=0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
        if(!devices[i].updated_conn && devices[i].connected && !devices[i].peri.finished && devices[i].peri.askedAdvPhone){
            
            update_conn_params(i, devices[i].conn_params);
            devices[i].updated_conn = true;
            return true;
        }
    }
    //if(lastToUpdate < NRF_SDH_BLE_CENTRAL_LINK_COUNT){
    //    update_conn_params(lastToUpdate, conn_params_on_gather_type1);
    //}
    for (uint8_t i=0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
        devices[i].updated_conn = false;
    }
    return false;
}

bool updating_ongather_conn_t23 = false;
bool update_all_conn_for_gather_t23(void){
    for (uint8_t i=0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
        if(devices[i].peri.type == 2 && !devices[i].updated_conn && devices[i].connected && !devices[i].peri.finished){
            update_conn_params(i, conn_params_on_gather_type2);
            devices[i].updated_conn = true;
            return true;
        }
    }
    for (uint8_t i=0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
        if(devices[i].peri.type == 3 && !devices[i].updated_conn && devices[i].connected && !devices[i].peri.finished){
            
            update_conn_params(i, devices[i].periodic_params);
            devices[i].updated_conn = true;
            return true;
        }
    }
    //if(lastToUpdate < NRF_SDH_BLE_CENTRAL_LINK_COUNT){
    //    update_conn_params(lastToUpdate, conn_params_on_gather_type1);
    //}
    for (uint8_t i=0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
        devices[i].updated_conn = false;
    }
    return false;
}

bool updating_onreq_conn = false;
void update_onreq_conn(int lastToUpdate){
    for (uint8_t i=0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
        if(devices[i].peri.type == 1 && !devices[i].peri.connToPhone && 
            devices[i].conn_params.min_conn_interval != conn_params_on_gather_type1.min_conn_interval && 
            devices[i].conn_params.max_conn_interval != conn_params_on_gather_type1.max_conn_interval &&
            i != lastToUpdate){

            update_conn_params(i, conn_params_on_gather_type1);
            return;
        }
    }
    if(lastToUpdate < NRF_SDH_BLE_CENTRAL_LINK_COUNT){
        update_conn_params(lastToUpdate, conn_params_on_gather_type1);
    }
    updating_onreq_conn = false;
}

//void calibrate_conn_params(uint16_t handle_not_to_cal, uint16_t extra_not_to_call){
//    for(int i=0; i<NRF_SDH_BLE_TOTAL_LINK_COUNT; i++){
//        if(i!=handle_not_to_cal && i!=extra_not_to_call && devices[i].connected){
//            update_conn_params(i, devices[i].conn_params);
//        }
//    }
//}

//void not_used_devices_conn_update(){
//    for(int i=0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
//        if(!devices[i].peri.chosenRelay && devices[i].connected){
//            update_conn_params(i, not_relayed_during_conn_params);
//        }
//    }
//}

void reset_not_used_devices_conn_update(){
    for(int i=0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
        if(devices[i].connected){
            update_conn_params(i, power_saving_conn_params);
        }
    }
}