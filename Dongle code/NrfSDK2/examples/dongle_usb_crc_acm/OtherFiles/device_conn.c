#include "device_conn.h"
#include "ble_conn_state.h"
#include "common.h"
//#include "boards.h"

// set to true for default communication, false for proposed method
bool doStar= false;//true;

void reset_device_base(int id){
    strcpy(devices[id].name, "     ");
    devices[id].id = 255;
    //devices[id].nameNum = 255;
    devices[id].connected = false;
    devices[id].updated_conn = false;
    devices[id].current_updating_conn_params.do_tlv = false;
    devices[id].queued_conn_params.do_tlv = false;
    devices[id].relay_all_upd_params.do_tlv = false;
    devices[id].conn_params = conn_params_periodic;
    devices[id].waiting_conn_update = false;
    devices[id].last_test_pref = -1;
    devices[id].cnt_no_rssi = 0;
    devices[id].curr_interval = 0;
    devices[id].time_passed = 0;
    devices[id].curr_sl = 0;
    devices[id].handle = id;
    devices[id].count = 0;
    devices[id].head = 0;
    devices[id].tail = 0;
    devices[id].evt_len = 0;
    devices[id].first_packet = true;
    devices[id].sending_alot = false;
    devices[id].params_queued = false;
    devices[id].is_relaying = false;
    devices[id].relayed = false;
    devices[id].relay_h = 255;
    devices[id].last_len = 0;
}

void reset_device_cent(int id){
    devices[id].cent.chooseCentDone = false;
    devices[id].cent.gotFinalCent = false;
    devices[id].cent.connToPhone = false;
    devices[id].cent.connecting = false;
    devices[id].cent.isPhone = false;
    //devices[id].cent.ensure_counter = 0;
    devices[id].cent.chosenRelay = false;
    devices[id].cent.isNotPaused = true;
    //devices[id].cent.ensure_send = false;
    devices[id].cent.notifiedRpi = false;
    devices[id].cent.last_pref_sent = -1;
    
    devices[id].cent.stoppedBatch = -1;
    devices[id].cent.stoppedPrefix = -1;
    devices[id].cent.start_send_after_conn_update = false;
}
bool allAdvAsked = false;
bool relay_sent_peris = false;
void reset_device_peri(int id, bool all){
    devices[id].peri.last = -1;
    devices[id].peri.last = 3;
    devices[id].peri.sending_missed = false;
    devices[id].peri.recPhoneMissData = false;
    //devices[id].peri.ensure_send = false;
    //devices[id].peri.ensure_counter = 0;
    devices[id].peri.first_miss_rec = -1;
    devices[id].peri.stopped_pref = -1;
    devices[id].peri.miss_ctr = 0;
    devices[id].peri.miss_sent = false;
    devices[id].peri.miss_ready = false;
    devices[id].peri.miss_rec = false;
    devices[id].peri.mode = 5;
    devices[id].peri.use_miss = true;
    devices[id].peri.batch_number = -1;
    devices[id].peri.miss_ctr_extra = 1;
    devices[id].peri.received_all_count = 0;
    devices[id].peri.last_missing_rec = -1;
    devices[id].peri.sent_missed_msg_cnt = 0;
    devices[id].peri.received_last_msg_cnt = 0;
    devices[id].peri.all_received_counter = -1;//0;
    devices[id].peri.stopped = false;
    devices[id].peri.sent_send_fast = false;
    devices[id].peri.sent_start_send = false;
    devices[id].peri.connToPhone = false;
    devices[id].peri.askedAdvPhone = false;
    devices[id].peri.firstAskAdv = false;
    //devices[id].peri.missed[0] = 2; // idx of last missed
    ////devices[id].peri.missed[1] = 0; // large counter before send, missed left to be rec. after
    //devices[id].peri.missed[1] = 0; // chunkID of last received, missed left to be rec. after
    memset(devices[id].peri.missed, 0, 256);
    devices[id].peri.missed[0] = 10;
    devices[id].peri.miss_b_idx = 2;
    devices[id].peri.new_miss_idx = 2;
    devices[id].peri.miss_idx = 4; 
    devices[id].peri.final_pref = -1; 
    devices[id].peri.final_batch = 4; 

    devices[id].peri.waiting_retransmit = false;
    devices[id].peri.type = 100;
    if(all){
        devices[id].peri.nus = NULL;
        devices[id].peri.chosenRelay = false;
        devices[id].peri.finished = false;
        devices[id].peri.is_sending_on_req = false;
        devices[id].peri.allDataSentFromPeri = false;
    }
}

void reset_device_tp(int id){
    //devices[id].tp.rssi = 0;
    //devices[id].tp.incr_counter = 0;
    //devices[id].tp.decr_counter = 0;
    devices[id].tp.signal_to_lower_counter = 0;
    devices[id].tp.tx_idx = 6;
    devices[id].tp.tx_range[8] = 8;
    devices[id].tp.tx_range[7] = 4;
    devices[id].tp.tx_range[6] = 0;
    devices[id].tp.tx_range[5] = -4;
    devices[id].tp.tx_range[4] = -8;
    devices[id].tp.tx_range[3] = -12;
    devices[id].tp.tx_range[2] = -16;
    devices[id].tp.tx_range[1] = -20;
    devices[id].tp.tx_range[0] = -40;
    if(id < NRF_SDH_BLE_CENTRAL_LINK_COUNT){//central can do min -20
        //devices[id].tp.min_tx_idx = max_tx_to_peri;
        devices[id].tp.window = RSSI_WINDOW_SIZE;
    }else{
        //devices[id].tp.min_tx_idx = max_tx_to_cent;
        devices[id].tp.window = RSSI_WINDOW_SIZE;// - RSSI_WINDOW_SIZE/3;///2;
        //if(devices[id].tp.window < 2){
            //devices[id].tp.window = 2;
        //}
    }    
    
    devices[id].tp.coefficient = (float) 2/((float) devices[id].tp.window*1.0 + 1);
    devices[id].tp.min_tx_idx = 1;
    devices[id].tp.max_tx_idx = 6;
    devices[id].tp.own_max_tx_idx = 6;
    devices[id].tp.skip = 20;
    devices[id].tp.requested_tx_idx = -1;
    devices[id].tp.other_tx_idx = 6;
    devices[id].tp.own_idx = 6;
    devices[id].tp.get_new_rssi = true;
    devices[id].tp.use_pdr = false;
    devices[id].tp.pdr_wait_count = -1;
    devices[id].tp.stopped_pdr = false;
    devices[id].tp.pdr = 0.0;
    devices[id].tp.window = RSSI_WINDOW_SIZE;
    devices[id].tp.sum_rssi = 0;
    devices[id].tp.head_rssi = 0;
    devices[id].tp.do_first_ema = true;
    devices[id].tp.avg_rssi = 0;
    memset(devices[id].tp.rssi_window, 0, RSSI_WINDOW_SIZE);
    devices[id].tp.received_counter = -1;
    devices[id].tp.ema_rssi = 70.0;

    devices[id].tp.min_rssi_limit = 1;
    devices[id].tp.ideal = 1;
    devices[id].tp.max_rssi_limit = 1;
    devices[id].tp.last_rssi = -72;
    devices[id].tp.wait_avg_cnt = 0;

    devices[id].tp.doing_const_test = false;

    devices[id].tp.first_evt_skip = false;
    //devices[id].tp.cnt_pack = {0};
    memset(devices[id].tp.cnt_pack, 0, sizeof(devices[id].tp.cnt_pack));
    devices[id].tp.docountpack = false;
    devices[id].tp.do_without_pdr = true;
    devices[id].tp.cnt_evt = 0;
    devices[id].tp.evt_head = 0;
    devices[id].tp.evt_max_p = 0;
    //devices[id].tp.received_counter = -1;
    //devices[id].tp.default_tx_idx = 0;
}

void reset_device_full(int id){
    reset_device_base(id);
    reset_device_cent(id);
    reset_device_peri(id, true);
    reset_device_tp(id);
}

void reset_on_req_queue(){
    if(on_req_queue == NULL){
        send_usb_data(false, "On_req_queue is empty.");
        return;
    }
    
    on_req_queue_t *current = on_req_queue;
    on_req_queue_t *next_node = NULL;

    // Traverse the list and free each node
    while (current != NULL) {
        next_node = current->next;  // Store the pointer to the next node
        send_usb_data(false, "Freeing node with handle: %d", current->handle);
        free(current);  // Free the current node
        current = next_node;  // Move to the next node
    }

    on_req_queue = NULL;
    send_usb_data(false, "All nodes in the queue have been freed.");
}

int total_pckg_count = 0;

void add_to_device_tlv_strg(int h, uint8_t* tlv, uint16_t length){
    if(devices[h].count == 15){
        send_usb_data(false, "Send to %s:%d max size storage reached", devices[h].name, h);
    }else{
        int head = devices[h].head;
        memcpy(devices[h].tlvs[head].data, tlv, length);
        devices[h].tlvs[head].length = length;
        devices[h].head = (head+1) % 15;
        devices[h].count++;
        total_pckg_count++;

        process_stored_tlv();
    }
}

void process_stored_tlv(void){
    ret_code_t err_code;
    int tail = 0;
    for (uint8_t h=0; h<NRF_SDH_BLE_TOTAL_LINK_COUNT; h++){
        while (devices[h].count  > 0) {

            tail = devices[h].tail;
            if(devices[h].tlvs[tail].length != 0){
                if(h < NRF_SDH_BLE_CENTRAL_LINK_COUNT){
                    err_code = send_specific_tlv_to_peripheral(h, tail);
                    //err_code = ble_nus_c_string_send(devices[h].peri.nus, devices[h].tlvs[tail].data, devices[h].tlvs[tail].length);
                }else{
                    err_code = send_specific_tlv_to_central(h, tail);
                    //if (err_code == NRF_SUCCESS && !doStar){
                    //    tp_timer_start(&devices[h]);
                    //}
                    //err_code = ble_nus_data_send(mm_nus, devices[h].tlvs[tail].data, &devices[h].tlvs[tail].length, h);
                }

                if (err_code == NRF_ERROR_RESOURCES) {
                    // Wait for BLE_NUS_EVT_TX_RDY before sending more
                    break;
                }else if (err_code == NRF_ERROR_INVALID_STATE){
                    send_tlv_usb(false, "ERR: Invalid state on send_tlv_to", devices[h].tlvs[tail].data, devices[h].tlvs[tail].length, devices[h].tlvs[tail].length, h);
                    devices[h].count --;
                    total_pckg_count--;
                    devices[h].tail = (tail+1)%15;
                    break;
                }
                else if (err_code != NRF_SUCCESS)
                {
                    char msg[300] = "Failed to send_tlv_to_peri. ";
                    int pos = 31;
                    pos += sprintf(msg+pos, "Error 0x%x:", err_code);
                    send_tlv_usb(false, msg, devices[h].tlvs[tail].data, devices[h].tlvs[tail].length, devices[h].tlvs[tail].length, h);

                    devices[h].count --;
                    total_pckg_count--;
                    devices[h].tail = (tail+1)%15;
                    break;
                }else{
                    //char msg2[300] = "";
                    //sprintf(msg2, "%s data sent, Type:%s", devices[peri_tlv_storage[peri_tlv_tail].handle].name, peri_tlv_storage[peri_tlv_tail].type);
                    if(!send_test_rssi && IS_RELAY){
                        send_tlv_usb(true, "Sent: ", devices[h].tlvs[tail].data, devices[h].tlvs[tail].length, devices[h].tlvs[tail].length, h);
                    }
                    devices[h].count --;
                    total_pckg_count--;
                    devices[h].tail = (tail+1)%15;
                }
            }else{
                devices[h].count --;
                total_pckg_count--;
                devices[h].tail = (tail+1)%15;
            }
        }
    }

    //for (uint8_t h=0; h<NRF_SDH_BLE_CENTRAL_LINK_COUNT; h++){
   
    //    while (devices[h].count  > 0) {
    //    tail = devices[h].tail;         
    //        if(devices[h].tlvs[tail].length != 0){
    //            err_code = ble_nus_c_string_send(devices[h].peri.nus, devices[h].tlvs[tail].data, devices[h].tlvs[tail].length);

    //            if (err_code == NRF_ERROR_RESOURCES) {
    //                // Wait for BLE_NUS_EVT_TX_RDY before sending more
    //                return;
    //            }else if (err_code == NRF_ERROR_INVALID_STATE){
    //                send_tlv_usb(false, "ERR: Invalid state on send_tlv_to", devices[h].tlvs[tail].data, devices[h].tlvs[tail].length, devices[h].tlvs[tail].length, h);
    //                devices[h].count --;
    //                devices[h].tail = (tail+1)%15;
    //                return;
    //            }
    //            else if (err_code != NRF_SUCCESS)
    //            {
    //                char msg[300] = "Failed to send_tlv_to_peri. ";
    //                int pos = 31;
    //                pos += sprintf(msg+pos, "Error 0x%x:", err_code);
    //                send_tlv_usb(false, msg, devices[h].tlvs[tail].data, devices[h].tlvs[tail].length, devices[h].tlvs[tail].length, h);
    //                return;
    //            }else{
    //                //char msg2[300] = "";
    //                //sprintf(msg2, "%s data sent, Type:%s", devices[peri_tlv_storage[peri_tlv_tail].handle].name, peri_tlv_storage[peri_tlv_tail].type);
    //                if(!send_test_rssi && IS_RELAY){
    //                    send_tlv_usb(true, devices[h].tlvs[tail].type, devices[h].tlvs[tail].data, devices[h].tlvs[tail].length, devices[h].tlvs[tail].length, h);
    //                }
    //                devices[h].count --;
    //                devices[h].tail = (tail+1)%15;
    //            }
    //        }else{
    //            devices[h].count --;
    //            devices[h].tail = (tail+1)%15;
    //        }
    //    }
    //}
    //for (uint16_t h=NRF_SDH_BLE_CENTRAL_LINK_COUNT; h<NRF_SDH_BLE_TOTAL_LINK_COUNT; h++){
    //    while (devices[h].count > 0) {
    //    tail = devices[h].tail;         
    //        if(devices[h].tlvs[tail].length != 0){
    //            err_code = ble_nus_data_send(mm_nus, devices[h].tlvs[tail].data, &devices[h].tlvs[tail].length, h);

    //            if (err_code == NRF_ERROR_RESOURCES) {
    //                // Wait for BLE_NUS_EVT_TX_RDY before sending more
    //                return;
    //            }else if (err_code == NRF_ERROR_INVALID_STATE){
    //                send_tlv_usb(false, "ERR: Invalid state on send_tlv_to", devices[h].tlvs[tail].data, devices[h].tlvs[tail].length, devices[h].tlvs[tail].length, h);
    //                devices[h].count--;
    //                devices[h].tail = (tail+1)%15;
    //                return;
    //            }
    //            else if (err_code != NRF_SUCCESS)
    //            {
    //                char msg[300] = "Failed to send_tlv_to_cent. ";
    //                int pos = 31;
    //                pos += sprintf(msg+pos, "Error 0x%x:", err_code);
    //                send_tlv_usb(false, msg, devices[h].tlvs[tail].data, devices[h].tlvs[tail].length, devices[h].tlvs[tail].length, h);
    //                return;
    //            }else{
    //                //char msg2[300] = "";
    //                //sprintf(msg2, "%s data sent", devices[cent_tlv_storage[cent_tlv_tail].handle].name);
    //                //send_tlv_usb(true, "Sent:", cent_tlv_storage[cent_tlv_tail].data, cent_tlv_storage[cent_tlv_tail].length, cent_tlv_storage[cent_tlv_tail].length, cent_tlv_storage[cent_tlv_tail].handle);
    //                devices[h].count--;
    //                devices[h].tail = (tail+1)%15;
    //            }
    //        }else{
    //            devices[h].count--;
    //            devices[h].tail = (tail+1)%15;
    //        }
    //    }
    //}
}


// Presets of command tlv messages
uint8_t tlv_nameParams[5] = {3};//, DEVICE_ID, SEND_TYPE, min_tx_idx_own, max_tx_idx_own};
uint8_t fast_tlv[1] = {11};
uint8_t tlv_start_data[2] = {0, DEVICE_ID};
uint8_t tlv_recAll[2] = {2,0};
uint8_t starting_periodic_tlv[2] = {12,12};
uint8_t tlv_rssiChange[4] = {6}; // was [3]=1,6,rssi
uint8_t tlv_reqData[2] = {1,DEVICE_ID}; // was 01
uint8_t tlv_chooseCent[5] = {0,0,DEVICE_ID};
uint8_t tlv_chooseCentResponse[13] = {0,0,255,255,0,0,0,0,0,0,0,0,0};
uint8_t tlv_chooseCentResponseSecond[7] = {0,0,DEVICE_ID};
uint8_t tlv_chooseCentFinal[3] = {0,2,DEVICE_ID};
uint8_t tlv_stopData[3] = {4,0,0}; // was [4]={1,0,0,0}
uint8_t tlv_resumeData[1] = {5}; // was [4] = {1,1,0,0}
uint8_t tlv_allDataSent[5] = {255, 255, DEVICE_ID};// was {255,255,255,255, DEVICE_ID};
uint8_t tlv_advertiseSink[4] = {9, 0, 0, 0};

int ALREADY_REC = 0;
int NO_MISSED = 1;
int MISSED = 2;
int MISSED_FULL = 3;

int checkMissedInBatch(int h, int received_prefix, int received_batch, int last){// return 0 = already rec, 1 = no missed, 2 = missed, 3 = missed full
    if(received_prefix <= devices[h].peri.last){
        return ALREADY_REC;
    }
    if(received_prefix > devices[h].peri.last+1){
        //if(devices[h].peri.missed[devices[h].peri.miss_b_idx + 1] == 0){
        //    devices[h].peri.missed[devices[h].peri.miss_b_idx] = received_batch;
        //}
        //if(!devices[h].peri.sending_periodic){
        //    for(uint8_t a = devices[h].peri.last+1; a<received_prefix; a++){
        //        devices[h].peri.missed[devices[h].peri.miss_idx] = a;
        //        devices[h].peri.missed[devices[h].peri.miss_b_idx+1]++;
            
        //        devices[h].peri.miss_idx++;

        //        if(devices[h].peri.miss_idx == 242){
        //            //if(a == received_prefix - 1){
        //            //    devices[h].peri.missed[243] = devices[h].peri.last+1;
        //            //}else{
        //                devices[h].peri.missed[243] = a+1;
        //                devices[h].peri.last = a;
        //            //}
        //            devices[h].peri.missed[242] = devices[h].peri.batch_number;
        //            //send_tlv_from_c(relay_h, devices[h].peri.missed, 244, "sending missed prefs");
        //            //devices[h].peri.sending_missed = true;
        //            //devices[h].peri.miss_b_idx = 2;
        //            //devices[h].peri.new_miss_idx = 2;
        //            //if(devices[h].tp.use_pdr){
        //            //    devices[h].tp.use_pdr = false;
        //            //    devices[h].tp.stopped_pdr = true;
        //            //}
        //            //devices[h].peri.missed[0] = 255;
        //            return MISSED_FULL;
        //        }
        //    }
        //}
        //send_tlv_usb(false, "Added missed ", devices[h].peri.missed, devices[h].peri.missed[0],devices[h].peri.missed[0], h);
    }else{
        devices[h].peri.last = received_prefix;
        return NO_MISSED;
    }
    devices[h].peri.last = received_prefix;
    return MISSED;
}

int checkMissed(int h, int received_prefix, int received_batch, bool finished){// new batch
    int res;
    int res2;
    if(finished){
        received_prefix++;
    }
    if(received_batch == devices[h].peri.batch_number+1){
        res = checkMissedInBatch(h, 255, devices[h].peri.batch_number, devices[h].peri.last);
        if(res == MISSED_FULL){
            return  res;
        }
        // start on new batch
        devices[h].peri.batch_number++;
        devices[h].peri.last = -1;
        //if(devices[h].peri.missed[devices[h].peri.miss_b_idx + 1] > 0){
        //    devices[h].peri.miss_b_idx = devices[h].peri.miss_idx;
        //    devices[h].peri.missed[devices[h].peri.miss_b_idx + 1] = 0;
        //    devices[h].peri.miss_idx += 2;
        //}

        res2 = checkMissedInBatch(h, received_prefix, received_batch, -1);
        if(received_prefix > 0){
            res = res2;
        }

        //if(res2 == MISSED && finished){
        //    if(devices[h].peri.missed[devices[h].peri.miss_idx-1] > devices[h].peri.last){
        //        devices[h].peri.last = devices[h].peri.missed[devices[h].peri.miss_idx-1];
        //    }
        //}
    }else{
        res = checkMissedInBatch(h, received_prefix, received_batch, devices[h].peri.last);
        //devices[h].peri.last = received_prefix;
    }
    if(res == MISSED && finished){
        devices[h].peri.last = received_prefix-1;
    }
    return res;
}


int bytes_per_period = 0;
int stored_bytes = 0; //1 hour 
int stored_bytes_sent = 0;
int periodic_waiting_bytes = 0;
bool finished_stored = false;
bool finished_missing = false;
uint8_t batch_counter = 0;
uint8_t pref_counter = 0;
uint8_t dummy_data[244];

//devices[h].tp.min_rssi_limit = -72;
//devices[h].tp.max_rssi_limit = -65;
//devices[h].tp.ideal = -69;

//devices[h].tp.min_rssi_limit = -75;
//devices[h].tp.max_rssi_limit = -68;
//devices[h].tp.ideal = -72;
bool isPhone = false;
void set_this_device_sim_params(void){
    remove_peri_from_scan_list(DEVICE_NAME);
    

    IS_SENSOR_NODE = true;
    //relay_info.pref_min_rssi_limit = -75;
    //relay_info.pref_max_rssi_limit = -68;
    //relay_info.pref_ideal_rssi = -72;
    relay_info.pref_ema_window_size = RSSI_WINDOW_SIZE;
    
    
    //get_desired_conn_params(&conn_params_maintain_relay, 1000, 1000, 2, 30000);
    get_desired_conn_params(&conn_params_conn_phone_wait, 250, 250, 0, 30000);
    if(DEVICE_ID == 'V'){
        
        //relay_info.pref_min_rssi_limit = -72;
        //relay_info.pref_max_rssi_limit = -65;
        //relay_info.pref_ideal_rssi = -69;
        SEND_TYPE = 3;
        if(!TEST_RSSI){
            get_desired_conn_params(&conn_params_periodic, 1250, 1250, 4, 30000);
            get_desired_conn_params(&conn_params_idle, 1250, 1250, 4, 30000);
            get_desired_conn_params(&conn_params_maintain_relay, 1250, 1250, 4, 30000);
            get_desired_conn_params(&conn_params_stored, 1250, 1250, 4, 30000);
            get_desired_conn_params(&conn_params_phone, 1250, 1250, 4, 30000);
            //get_desired_conn_params(&conn_params_idle, 2000, 2000, 4, 20000);
            //get_desired_conn_params(&conn_params_maintain_relay, 2000, 2000, 4, 20000);
            //get_desired_conn_params(&conn_params_stored, 2000, 2000, 4, 20000);
            //get_desired_conn_params(&conn_params_phone, 2000, 2000, 4, 30000);
        }
        // 36 B/min
        max_tx_idx_own = 4;
        min_tx_idx_own = 1;
        bytes_per_period = 36;//*5;
        stored_bytes = 36*60;//per 1 h //2160;
        PERIODIC_TIMER_INTERVAL = 60000;//5000;//60000;//*5;

    }else if(DEVICE_ID == 'M'){
        // 800 B/s
        max_tx_idx_own = 6;
        min_tx_idx_own = 1;
        bytes_per_period = 400;//800;
        stored_bytes = 800*60*60;// per 1 h //100000;//2880000/2;
        PERIODIC_TIMER_INTERVAL = 500;//1000;
        SEND_TYPE = 1;
        //if(!TEST_RSSI){
            get_desired_conn_params(&conn_params_periodic, 250, 250, 4, 30000);
            get_desired_conn_params(&conn_params_idle, 1000, 1000, 2, 20000);
            get_desired_conn_params(&conn_params_maintain_relay, 1000, 1000, 2, 20000);
            get_desired_conn_params(&conn_params_stored, 100, 100, 0, 20000);
            get_desired_conn_params(&conn_params_phone, 50, 50, 0, 20000);
        //}
    }else if(DEVICE_ID == 'C'){
        //if(EXISTING_RELAY){
            max_tx_idx_own = 6;
            //remove_peri_from_scan_list("Calmr");
        //}else{
        //    max_tx_idx_own = 6;
        //}
        min_tx_idx_own = 1;
        // 1300 B/s
        bytes_per_period = 650;//1300;
        stored_bytes = 1300*60*60;//200000;//4680000/2;
        PERIODIC_TIMER_INTERVAL = 500;//1000;
        SEND_TYPE = 0;
        IS_RELAY = true;//EXISTING_RELAY;
        if(IS_RELAY){
            SEND_TYPE = 0;
        }
        //if(!TEST_RSSI){
            get_desired_conn_params(&conn_params_periodic, 250, 250, 4, 30000);
            get_desired_conn_params(&conn_params_idle, 1000, 1000, 2, 20000);
            get_desired_conn_params(&conn_params_maintain_relay, 1000, 1000, 2, 20000);
            get_desired_conn_params(&conn_params_stored, 50, 50, 0, 20000);
            get_desired_conn_params(&conn_params_phone, 50, 50, 0, 20000);
        //}
    }else if(DEVICE_ID == 'L'){
        max_tx_idx_own = 5;
        min_tx_idx_own = 1;
        // 16 B/s
        bytes_per_period = 16;//160; // 16/s or 960/min
        stored_bytes = 16*60*60; // per 1 h
        PERIODIC_TIMER_INTERVAL = 1000;//10000;
        SEND_TYPE = 2;
        //if(!TEST_RSSI){
            get_desired_conn_params(&conn_params_periodic, 1000, 1000, 2, 10000);
            get_desired_conn_params(&conn_params_idle, 1000, 1000, 2, 20000);
            get_desired_conn_params(&conn_params_maintain_relay, 1000, 1000, 2, 20000);
            get_desired_conn_params(&conn_params_stored, 250, 250, 0, 20000);
            get_desired_conn_params(&conn_params_phone, 250, 250, 0, 20000);
        //}
    }else if(DEVICE_ID == 'R'){
        max_tx_idx_own = 5;
        min_tx_idx_own = 1;
        // 16 B/s
        bytes_per_period = 16;//160; // 16/s or 960/min
        stored_bytes = 16*60*60; // per 1 h
        PERIODIC_TIMER_INTERVAL = 1000;//10000;
        SEND_TYPE = 2;
        //if(!TEST_RSSI){
            get_desired_conn_params(&conn_params_periodic, 1000, 1000, 2, 30000);
            get_desired_conn_params(&conn_params_idle, 1000, 1000, 2, 20000);
            get_desired_conn_params(&conn_params_maintain_relay, 1000, 1000, 2, 20000);
            get_desired_conn_params(&conn_params_stored, 250, 250, 0, 20000);
            get_desired_conn_params(&conn_params_phone, 250, 250, 0, 20000);
        //}
    }else if(DEVICE_ID == 'E'){
        max_tx_idx_own = 6;
        min_tx_idx_own = 1;
        // 24 B/s
        bytes_per_period = 24;//240;
        stored_bytes = 24*60*60;
        PERIODIC_TIMER_INTERVAL = 1000;//10000;
        SEND_TYPE = 2;
        //if(!TEST_RSSI){
            get_desired_conn_params(&conn_params_periodic, 1000, 1000, 2, 30000);
            get_desired_conn_params(&conn_params_idle, 1000, 1000, 2, 20000);
            get_desired_conn_params(&conn_params_maintain_relay, 1000, 1000, 2, 20000);
            get_desired_conn_params(&conn_params_stored, 250, 250, 0, 20000);
            get_desired_conn_params(&conn_params_phone, 250, 250, 0, 20000);
        //}

    }else if(DEVICE_ID == 'G'){
        max_tx_idx_own = 4;
        min_tx_idx_own = 1;
        // 16 B/5m
        bytes_per_period = 16;
        stored_bytes = 16*12;
        PERIODIC_TIMER_INTERVAL = 1000*60*5;//5000;//300000;
        SEND_TYPE = 3;
        //relay_info.pref_min_rssi_limit = -72;
        //relay_info.pref_max_rssi_limit = -65;
        //relay_info.pref_ideal_rssi = -69;
        //if(!TEST_RSSI){
            get_desired_conn_params(&conn_params_periodic, 1250, 1250, 4, 30000);
            get_desired_conn_params(&conn_params_idle, 1250, 1250, 4, 30000);
            get_desired_conn_params(&conn_params_maintain_relay, 1250, 1250, 4, 30000);
            get_desired_conn_params(&conn_params_stored, 1250, 1250, 4, 30000);
            get_desired_conn_params(&conn_params_phone, 1250, 1250, 4, 30000);
            //get_desired_conn_params(&conn_params_idle, 2000, 2000, 4, 20000);
            //get_desired_conn_params(&conn_params_maintain_relay, 2000, 2000, 4, 20000);
            //get_desired_conn_params(&conn_params_stored, 2000, 2000, 4, 20000);
            //get_desired_conn_params(&conn_params_phone, 2000, 2000, 4, 30000);
        //}
    }else if(DEVICE_ID == 'P'){
        isPhone = true;
        SEND_TYPE = 1;
        max_tx_idx_own = 6;
        min_tx_idx_own = 6;
        stored_bytes = 0;
        SEND_TYPE = 10;
        IS_RELAY = false;
        IS_SENSOR_NODE = false;
        get_desired_conn_params(&conn_params_periodic, 1000, 1000, 2, 30000);
        get_desired_conn_params(&conn_params_idle, 1000, 1000, 2, 20000);
        get_desired_conn_params(&conn_params_maintain_relay, 1000, 1000, 2, 20000);
        get_desired_conn_params(&conn_params_stored, 250, 250, 0, 20000);
        get_desired_conn_params(&conn_params_phone, 250, 250, 0, 20000);

        
    }
    if(DEVICE_ID != 'P'){
        periodic_timer_start();
        stored_bytes = stored_bytes/2;
    }
    og_stored_bytes = stored_bytes;
    //if(TEST_RSSI){
    //    max_tx_idx_own = 6;
    //    min_tx_idx_own = 1;
    //    default_tx_idx = 6;

    //    relay_info.pref_ema_window_size = RSSI_WINDOW_SIZE;
    //    conn_params_periodic = conn_params_test_rssi;
    //}else{
    //    set_own_periodic_conn_params();
    //}
    //if(DEVICE_ID == 'C' || DEVICE_ID == 'M'){
    //    stored_bytes = 241*1500; // TODO: REMOVEEEE
    //}else{
    //    stored_bytes = 241*800;
    //}
    if(PERIS_DO_TP){
        for(int i = 0; i < NRF_SDH_BLE_TOTAL_LINK_COUNT; i++){
            devices[i].tp.max_tx_idx = max_tx_idx_own;
            devices[i].tp.min_rssi_limit = min_tx_idx_own;
            //devices[i].tp.window = RSSI_WINDOW_SIZE; // done in function change_ema_coefficient_window

        }
        change_ema_coefficient_window(RSSI_WINDOW_SIZE);
    }else{    
        change_ema_coefficient_window(relay_info.pref_ema_window_size);
    }

    //IS_RELAY = (SEND_TYPE == 3);
    tlv_nameParams[1] = DEVICE_ID;
    tlv_nameParams[2] = SEND_TYPE;
    tlv_nameParams[3] = min_tx_idx_own;
    tlv_nameParams[4] =  max_tx_idx_own;
    
    memset(dummy_data, 'a', MAX_PACKAGE_SIZE);
    dummy_data[0] = 0;
    dummy_data[1] = 0;
    dummy_data[2] = DEVICE_ID;

    relay_info.count_type1 = 0;
    relay_info.count_type1_sink = 0;
    relay_info.count_type2 = 0;
    relay_info.count_type3 = 0;

    relay_info.disc_type1 = 0;
    relay_info.disc_type1_sink = 0;
    relay_info.disc_type2 = 0;
    relay_info.disc_type3 = 0;


    
    default_tx_idx = 6;
    if(default_tx_idx > max_tx_idx_own){
        default_tx_idx = max_tx_idx_own;
    }
    
    
}
bool og_signal_sent = false;
void set_other_device_op_mode(int h, int mode){
    devices[h].peri.sending_idle = false;
    devices[h].peri.sending_stored = false;
    devices[h].peri.sending_stored_waiting = false;
    devices[h].peri.sending_periodic = false;
    devices[h].peri.sending_to_phone = false;
    devices[h].peri.mode = mode;
    devices[h].tp.stopped_pdr = false;
    devices[h].curr_interval = 0; //added
        if(mode == 0){ //idle
            devices[h].tp.min_tx_idx_curr = 3;
            devices[h].tp.use_pdr = false;
            //devices[h].conn_params = devices[h].idle_params;
            devices[h].tp.ideal = -70;
            devices[h].tp.min_rssi_limit = -73;
            devices[h].tp.max_rssi_limit = -67;
            devices[h].peri.sending_idle = true;

            if(devices[h].id != 'V' || devices[h].id != 'G'){
                change_ema_coefficient_window_specific(h, 50);
            }else{
                change_ema_coefficient_window_specific(h, 50);
            }
            //copy_conn_params(&devices[h].conn_params, &devices[h].idle_params);
            devices[h].conn_params = devices[h].idle_params;
        }else if(mode == 1){ //stored
            if(devices[h].id != 'V' || devices[h].id != 'G'){
                devices[h].tp.ideal = -72;
                devices[h].tp.min_rssi_limit = -75;
                devices[h].tp.max_rssi_limit = -68;
                devices[h].tp.use_pdr = true;
                devices[h].tp.min_tx_idx_curr = 1;
                if(devices[h].id == 'M' && !isPhone && IS_RELAY){
                    change_ema_coefficient_window_specific(h, 10);
                }else if(devices[h].id == 'C' || (devices[h].id == 'M' && isPhone)){
                    change_ema_coefficient_window_specific(h, 5);
                }else{
                    change_ema_coefficient_window_specific(h, 20);
                }
            }else{
                devices[h].tp.use_pdr = false;
                change_ema_coefficient_window_specific(h, 50);
                devices[h].tp.min_tx_idx_curr = 3;
                devices[h].tp.ideal = -70;
                devices[h].tp.min_rssi_limit = -73;
                devices[h].tp.max_rssi_limit = -67;
            }
            if(isPhone){
                //copy_conn_params(&devices[h].conn_params, &devices[h].phone_params);
                devices[h].conn_params = devices[h].phone_params;
                //devices[h].conn_params.min_conn_interval * UNIT_1_25_MS / 1000;
                send_usb_data(true, "Param set for phone spec, compare=%d", compare_conn_params(&devices[h].conn_params, &devices[h].phone_params));
                //send_usb_data(true, "Current conn param for %s:%d: MIN %d  ms; MAX %d ms; latency=%d, timeout=%d", 
                //        devices[h].name, h,
                //        devices[h].conn_params.min_conn_interval * UNIT_1_25_MS / 1000, 
                //        devices[h].conn_params.max_conn_interval * UNIT_1_25_MS / 1000, 
                //        devices[h].conn_params.slave_latency,
                //        devices[h].conn_params.conn_sup_timeout * UNIT_10_MS / 1000);
                //send_usb_data(true, "Current conn param for %s:%d: MIN %d  ms; MAX %d ms; latency=%d, timeout=%d", 
                //        devices[h].name, h,
                //        devices[h].phone_params.min_conn_interval * UNIT_1_25_MS / 1000, 
                //        devices[h].phone_params.max_conn_interval * UNIT_1_25_MS / 1000, 
                //        devices[h].phone_params.slave_latency,
                //        devices[h].phone_params.conn_sup_timeout * UNIT_10_MS / 1000);
                //send_usb_data(true, "!!!!!!!!!!!!!!!!!!!!!!"); 

                //copy_conn_params2(&devices[h].conn_params, &devices[h].phone_params);
                //m2 = devices[h].conn_params.min_conn_interval * UNIT_1_25_MS / 1000;

                //send_usb_data(true, "Param set for phone spec, compare=%d", compare_conn_params(&devices[h].conn_params, &devices[h].phone_params));
                //send_usb_data(true, "Current conn param for %s:%d: MIN %d  ms; MAX %d ms; latency=%d, timeout=%d", 
                //        devices[h].name, h,
                //        devices[h].conn_params.min_conn_interval * UNIT_1_25_MS / 1000, 
                //        devices[h].conn_params.max_conn_interval * UNIT_1_25_MS / 1000, 
                //        devices[h].conn_params.slave_latency,
                //        devices[h].conn_params.conn_sup_timeout * UNIT_10_MS / 1000);
                //send_usb_data(true, "Current conn param for %s:%d: MIN %d  ms; MAX %d ms; latency=%d, timeout=%d", 
                //        devices[h].name, h,
                //        devices[h].phone_params.min_conn_interval * UNIT_1_25_MS / 1000, 
                //        devices[h].phone_params.max_conn_interval * UNIT_1_25_MS / 1000, 
                //        devices[h].phone_params.slave_latency,
                //        devices[h].phone_params.conn_sup_timeout * UNIT_10_MS / 1000);
            }else{
                //copy_conn_params(&devices[h].conn_params, &devices[h].stored_params);
                devices[h].conn_params = devices[h].stored_params;
                send_usb_data(true, "Param set for Stored spec");
            }

            devices[h].peri.sending_stored = true;
        }else if(mode == 2){ //periodic
            devices[h].tp.use_pdr = false;
            devices[h].tp.min_tx_idx_curr = 3;
            devices[h].tp.ideal = -70;
            devices[h].tp.min_rssi_limit = -73;
            devices[h].tp.max_rssi_limit = -66;

            if(devices[h].id == 'M' || devices[h].id == 'C'){
                devices[h].tp.min_tx_idx_curr = 2;
                change_ema_coefficient_window_specific(h, 20);
                devices[h].tp.ideal = -72;
                devices[h].tp.min_rssi_limit = -75;
                devices[h].tp.max_rssi_limit = -68;

            }else if(devices[h].id == 'E' || devices[h].id == 'L' || devices[h].id == 'R'){
                change_ema_coefficient_window_specific(h, 50);
            }
            //copy_conn_params(&devices[h].conn_params, &devices[h].periodic_params);
            devices[h].conn_params = devices[h].periodic_params;
            devices[h].peri.sending_periodic = true;
        }else if(mode == 3){ //to phone, maintain relay
            devices[h].peri.sending_to_phone = true;
            change_ema_coefficient_window_specific(h, 50);
            devices[h].tp.use_pdr = false;
            devices[h].tp.min_tx_idx_curr = 3;
            devices[h].tp.ideal = -70;
            devices[h].tp.min_rssi_limit = -73;
            devices[h].tp.max_rssi_limit = -66;
            devices[h].conn_params = devices[h].idle_params;
            //copy_conn_params(&devices[h].conn_params, &devices[h].idle_params);
        }else if(mode == 4){ //to phone, conn phone wait
            
            devices[h].peri.sending_stored_waiting = true;
            if(devices[h].id != 'V' || devices[h].id != 'G'){
                change_ema_coefficient_window_specific(h, 10);
                devices[h].tp.min_tx_idx_curr = 1;
                devices[h].tp.ideal = -72;
                devices[h].tp.min_rssi_limit = -75;
                devices[h].tp.max_rssi_limit = -68;
            }else{
                change_ema_coefficient_window_specific(h, 50);
                devices[h].tp.min_tx_idx_curr = 3;
                devices[h].tp.ideal = -70;
                devices[h].tp.min_rssi_limit = -73;
                devices[h].tp.max_rssi_limit = -67;
            }
            devices[h].conn_params = devices[h].wait_params;
            //copy_conn_params(&devices[h].conn_params, &devices[h].wait_params);
            devices[h].tp.use_pdr = false;
        }
        devices[h].tp.ideal += 2;
        devices[h].tp.min_rssi_limit += 2;
        devices[h].tp.max_rssi_limit += 4;
    //send_usb_data(true, "Check params for %s:%d: IDLE %d ms, STORED %d ms; PERIODIC %d ms, PHONE %d ms; WAIT %d ms, CONN %d ms", 
    //        devices[h].name, h,
    //        devices[h].idle_params.min_conn_interval * UNIT_1_25_MS / 1000, 
    //        devices[h].stored_params.min_conn_interval * UNIT_1_25_MS / 1000, 
    //        devices[h].periodic_params.min_conn_interval * UNIT_1_25_MS / 1000, 
    //        devices[h].phone_params.min_conn_interval * UNIT_1_25_MS / 1000, 
    //        devices[h].wait_params.min_conn_interval * UNIT_1_25_MS / 1000,
    //        devices[h].conn_params.min_conn_interval * UNIT_1_25_MS / 1000);
    //send_usb_data(true, "%s set to mode %d with min conn = %d ms",devices[h].name, mode, devices[h].conn_params.min_conn_interval * UNIT_1_25_MS / 1000);
    //send_usb_data(true, "Check mode for %s:%d: MIN %d units, %d in ms; MAX %d units, %d in ms; latency=%d, timeout=%d", 
    //                devices[h].name,h,
    //                devices[h].conn_params.min_conn_interval * UNIT_1_25_MS / 1000, 
    //                devices[h].conn_params.min_conn_interval + devices[h].conn_params.min_conn_interval/4, 
    //                devices[h].conn_params.max_conn_interval * UNIT_1_25_MS / 1000, 
    //                devices[h].conn_params.max_conn_interval + devices[h].conn_params.max_conn_interval/4,
    //                devices[h].conn_params.slave_latency,
    //                devices[h].conn_params.conn_sup_timeout * UNIT_10_MS / 1000);
    //if(mode == 1){
        //get_desired_conn_params(&devices[h].conn_params, devices[h].phone_params.min_conn_interval, devices[h].phone_params.max_conn_interval, devices[h].phone_params.slave_latency, devices[h].phone_params.conn_sup_timeout);
        //m3 = devices[h].conn_params.min_conn_interval * UNIT_1_25_MS / 1000;
        //devices[h].conn_params = devices[h].phone_params;
        //m4 = devices[h].conn_params.min_conn_interval * UNIT_1_25_MS / 1000;
        //send_usb_data(true, "2222 Check params for %s:%d: IDLE %d ms, STORED %d ms; PERIODIC %d ms, PHONE %d ms; WAIT %d ms, CONN %d ms", 
        //    devices[h].name, h,
        //    devices[h].idle_params.min_conn_interval * UNIT_1_25_MS / 1000, 
        //    devices[h].stored_params.min_conn_interval * UNIT_1_25_MS / 1000, 
        //    devices[h].periodic_params.min_conn_interval * UNIT_1_25_MS / 1000, 
        //    devices[h].phone_params.min_conn_interval * UNIT_1_25_MS / 1000, 
        //    devices[h].wait_params.min_conn_interval * UNIT_1_25_MS / 1000,
        //    devices[h].conn_params.min_conn_interval * UNIT_1_25_MS / 1000);
        //send_usb_data(true, "2222 %s set to mode %d with min conn = %d ms",devices[h].name, mode, devices[h].conn_params.min_conn_interval * UNIT_1_25_MS / 1000);
        //send_usb_data(true, "2222 Check mode for %s:%d: MIN %d units, %d in ms; MAX %d units, %d in ms; latency=%d, timeout=%d", 
        //                devices[h].name,h,
        //                devices[h].conn_params.min_conn_interval * UNIT_1_25_MS / 1000, 
        //                devices[h].conn_params.min_conn_interval + devices[h].conn_params.min_conn_interval/4, 
        //                devices[h].conn_params.max_conn_interval * UNIT_1_25_MS / 1000, 
        //                devices[h].conn_params.max_conn_interval + devices[h].conn_params.max_conn_interval/4,
        //                devices[h].conn_params.slave_latency,
        //                devices[h].conn_params.conn_sup_timeout * UNIT_10_MS / 1000);
    //}
    send_usb_data(true, "Current conn param for %s:%d, mode=%d: PHcon= %d ms, cmp=%d, MIN %d ms",devices[h].name, h,devices[h].peri.mode, devices[h].phone_params.min_conn_interval * UNIT_1_25_MS / 1000, compare_conn_params(&devices[h].conn_params, &devices[h].conn_params), devices[h].conn_params.min_conn_interval * UNIT_1_25_MS / 1000);
}

void set_other_device_sim_params(int h, uint8_t* info){
    //if(info[1] != 'P'){
    //    set_other_device_op_mode(h, 0);
    //}
    get_desired_conn_params(&devices[h].wait_params, 250,250,0,20000);//500, 600, 0, 20000);
    send_tlv_usb(true, "Got peri name Set DEVICE ", info, 5, 5, h);
    uint8_t start_periodic_tlv[1] = {2};
    if(info[1] == 'V'){
      strcpy(devices[h].name, "ViQtr");
      devices[h].tp.evt_max_p = 3;
      devices[h].peri.type = 3;
      //if(IS_RELAY && devices[h].id == 255){
          //stored_bytes += 2160;
          //add_other_device_periodic_bytes_at_start('V', 2160);
              //send_tlv_from_c(h, start_periodic_tlv, 1, "Start periodic data");
              //get_desired_conn_params(&devices[h].idle_params, 2000, 2000, 4, 20000);
              //get_desired_conn_params(&devices[h].stored_params, 2000, 2000, 4, 20000);
              //get_desired_conn_params(&devices[h].periodic_params, 2000, 2000, 4, 20000);
              //get_desired_conn_params(&devices[h].phone_params, 2000, 2000, 4, 20000);
              get_desired_conn_params(&devices[h].idle_params, 1250, 1250, 4, 30000);
              get_desired_conn_params(&devices[h].phone_params, 1250, 1250, 4, 30000);
              get_desired_conn_params(&devices[h].stored_params, 1250, 1250, 4, 30000);
              get_desired_conn_params(&devices[h].wait_params, 1250, 1250, 4, 30000);
              get_desired_conn_params(&devices[h].periodic_params, 1250, 1250, 4, 30000);
              //get_desired_conn_params(&conn_params_idle, 2000, 2000, 4, 20000);
              //get_desired_conn_params(&conn_params_maintain_relay, 2000, 2000, 4, 20000);
              //get_desired_conn_params(&conn_params_stored, 2000, 2000, 4, 20000);
              //get_desired_conn_params(&conn_params_phone, 2000, 2000, 4, 30000);
      //}
    }else if(info[1] == 'M'){
      strcpy(devices[h].name, "MicrV");
      devices[h].evt_len = 12;
      devices[h].tp.evt_max_p = 9;
      devices[h].peri.type = 1;
      //devices[h].evt_len = 20;
      //devices[id].tp.evt_max_p = 12;
      get_desired_conn_params(&devices[h].idle_params, 1000, 1000, 2, 30000);
      get_desired_conn_params(&devices[h].stored_params, 100, 100, 0, 3000);
      get_desired_conn_params(&devices[h].phone_params, 50, 50, 0, 20000);
      get_desired_conn_params(&devices[h].periodic_params, 250, 250, 4, 20000);
    }else if(info[1] == 'C'){
      strcpy(devices[h].name, "Calmr");
      connecting_handle = 0;
      RELAY_HANDLE = h;
      devices[h].peri.type = 0;
      get_desired_conn_params(&devices[h].idle_params, 1000, 1000, 2, 30000);
      get_desired_conn_params(&devices[h].stored_params, 50, 50, 0, 20000);
      get_desired_conn_params(&devices[h].phone_params, 50, 50, 0, 20000);
      get_desired_conn_params(&devices[h].periodic_params, 250, 250, 4, 20000);
      send_usb_data(true, "Check params for %s:%d: IDLE %d ms, STORED %d ms; PERIODIC %d ms, PHONE %d ms; WAIT %d ms", 
            devices[h].name, h,
            devices[h].idle_params.min_conn_interval * UNIT_1_25_MS / 1000, 
            devices[h].stored_params.min_conn_interval * UNIT_1_25_MS / 1000, 
            devices[h].periodic_params.min_conn_interval * UNIT_1_25_MS / 1000, 
            devices[h].phone_params.min_conn_interval * UNIT_1_25_MS / 1000, 
            devices[h].wait_params.min_conn_interval * UNIT_1_25_MS / 1000);

    }else if(info[1] == 'L'){
      devices[h].peri.type = 2;
      strcpy(devices[h].name, "PhysL");
      get_desired_conn_params(&devices[h].idle_params, 1000, 1000, 2, 20000);
      get_desired_conn_params(&devices[h].stored_params, 250, 250, 0, 30000);
      get_desired_conn_params(&devices[h].phone_params, 250, 250, 0, 20000);
      get_desired_conn_params(&devices[h].periodic_params, 1000, 1000, 2, 20000);
      devices[h].evt_len = 8;
      devices[h].tp.evt_max_p = 5;
    }else if(info[1] == 'R'){
      strcpy(devices[h].name, "PhysR");
      devices[h].peri.type = 2;
      get_desired_conn_params(&devices[h].idle_params, 1000, 1000, 2, 20000);
      get_desired_conn_params(&devices[h].stored_params, 250, 250, 0, 30000);
      get_desired_conn_params(&devices[h].phone_params, 250, 250, 0, 30000);
      get_desired_conn_params(&devices[h].periodic_params, 1000, 1000, 2, 20000);
      devices[h].evt_len = 8;
      devices[h].tp.evt_max_p = 5;
    }else if(info[1] == 'E'){
      strcpy(devices[h].name, "EpocX");
      devices[h].peri.type = 2;
      get_desired_conn_params(&devices[h].idle_params, 1000, 1000, 2, 20000);
      get_desired_conn_params(&devices[h].phone_params, 250, 250, 0, 30000);
      get_desired_conn_params(&devices[h].stored_params, 250, 250, 0, 30000);
      get_desired_conn_params(&devices[h].periodic_params, 1000, 1000, 2, 20000);
      devices[h].evt_len = 8;
      devices[h].tp.evt_max_p = 5;
    }else if(info[1] == 'G'){
      strcpy(devices[h].name, "Glucs");
      devices[h].peri.type = 3;
      //if(IS_RELAY && devices[h].id == 255){
          devices[h].tp.evt_max_p = 3;
          //stored_bytes += 2160;
          //add_other_device_periodic_bytes_at_start('G', 192);
          //if(!TEST_RSSI){
              //send_tlv_from_c(h, start_periodic_tlv, 1, "Start periodic data");
              //get_desired_conn_params(&devices[h].idle_params, 2000, 2000, 4, 20000);
              //get_desired_conn_params(&devices[h].phone_params, 2000, 2000, 4, 20000);
              //get_desired_conn_params(&devices[h].stored_params, 2000, 2000, 4, 20000);
              //get_desired_conn_params(&devices[h].periodic_params, 2000, 2000, 4, 20000);
              get_desired_conn_params(&devices[h].idle_params, 1250, 1250, 4, 30000);
              get_desired_conn_params(&devices[h].phone_params, 1250, 1250, 4, 30000);
              get_desired_conn_params(&devices[h].wait_params, 1250, 1250, 4, 30000);
              //get_desired_conn_params(&conn_params_maintain_relay, 1000, 1000, 4, 30000);
              get_desired_conn_params(&devices[h].stored_params, 1250, 1250, 4, 30000);
              get_desired_conn_params(&devices[h].periodic_params, 1250, 1250, 4, 30000);
              //get_desired_conn_params(&conn_params_idle, 2000, 2000, 4, 20000);
              //get_desired_conn_params(&conn_params_maintain_relay, 2000, 2000, 4, 20000);
              //get_desired_conn_params(&conn_params_stored, 2000, 2000, 4, 20000);
              //get_desired_conn_params(&conn_params_phone, 2000, 2000, 4, 30000);
          //}
      //}
    }else if(info[1] == 'X'){
      strcpy(devices[h].name, "Extra");//"Relay");
    }else if(info[1] == 'P'){
      strcpy(devices[h].name, "Phone");
      devices[h].peri.type = 10;
      connectedToPhone = true;
    }else{
      send_usb_data(false, "Got INVALID name ID = %c", info[1]);
      get_desired_conn_params(&devices[h].idle_params, 1000, 1000, 2, 20000);
      get_desired_conn_params(&devices[h].phone_params, 250, 250, 0, 30000);
      get_desired_conn_params(&devices[h].stored_params, 250, 250, 0, 30000);
      get_desired_conn_params(&devices[h].periodic_params, 1000, 1000, 2, 20000);
      return;
    }
    send_usb_data(true, "Check params for %s:%d: IDLE %d ms, STORED %d ms; PERIODIC %d ms, PHONE %d ms; WAIT %d ms, CONN %d ms", 
            devices[h].name, h,
            devices[h].idle_params.min_conn_interval * UNIT_1_25_MS / 1000, 
            devices[h].stored_params.min_conn_interval * UNIT_1_25_MS / 1000, 
            devices[h].periodic_params.min_conn_interval * UNIT_1_25_MS / 1000, 
            devices[h].phone_params.min_conn_interval * UNIT_1_25_MS / 1000, 
            devices[h].wait_params.min_conn_interval * UNIT_1_25_MS / 1000,
            devices[h].conn_params.min_conn_interval * UNIT_1_25_MS / 1000);
    devices[h].connected = true;
    devices[h].tp.own_idx = default_tx_idx;
    //send_usb_data(true, "Early check: IDnum=%d, IDchar=%c, conn=%d, minTP=%d, maxTP=%d", info[1], devices[h].id, devices[h].connected, tx_range[devices[h].tp.min_tx_idx], tx_range[devices[h].tp.max_tx_idx]);
    //send_usb_data(true, "Early check2: oldId=%d, if old==new =%d, newId=%c, if new!=P =%d, amRelay=%d", devices[h].id, devices[h].id != info[1], info[1],info[1] != 'P', IS_RELAY);
    if(devices[h].id == info[1] && devices[h].connected){
        return;
    }
    if(devices[h].id != info[1]){
        devices[h].id = info[1];
        //send_usb_data(true, "Early check: oldId=%d, if old==new =%d, newId=%c, if new!=P =%d, amRelay=%d", devices[h].id, devices[h].id != info[1], info[1],info[1] != 'P', IS_RELAY);
        if(devices[h].id != 'P'){
            //TODO: NO NEED TO BE DONE WHEN PERIS CHOSE
            //devices[h].peri.type = info[2];
            devices[h].tp.min_tx_idx = info[3];
            devices[h].tp.max_tx_idx = info[4];

            if(info[2] == 0){ // is relay
                devices[h].peri.type = 0;
                RELAY_HANDLE = h;
                //if(!TEST_RSSI && !PERIS_DO_TP){
                    devices[h].tp.min_rssi_limit = -72;//relay_info.pref_min_rssi_limit; //-72;
                    devices[h].tp.max_rssi_limit = -65;//relay_info.pref_max_rssi_limit;//-65;
                    devices[h].tp.ideal = -69;//relay_info.pref_ideal_rssi;//-69;
                //}
                change_ema_coefficient_window_specific(h, relay_info.pref_ema_window_size);

                char coef[10] ="";
                sprintf(coef, "%d.%d%d", (int8_t)  devices[h].tp.coefficient, ((int8_t)(devices[h].tp.coefficient*10))%10, ((int8_t)(devices[h].tp.coefficient*100))%10);
                send_usb_data(true, "RELAY %s has: WINDOW=%d, coeff=%s, min=%d, max=%d, ideal=%d", devices[h].name, devices[h].tp.window, coef, devices[h].tp.min_rssi_limit, devices[h].tp.max_rssi_limit, devices[h].tp.ideal);

                devices[h].tp.ema_rssi = devices[h].tp.ideal;
                devices[h].tp.received_counter = -1; // NO TPC before exchanging TP init info
               
                if(PERIS_DO_TP){
                    devices[h].tp.received_counter = 0;
                }

                send_usb_data(true, "Received Relay info: IDnum=%d, IDchar=%c, conn=%d, minTP=%d, maxTP=%d", info[1], devices[h].id, devices[h].connected, tx_range[devices[h].tp.min_tx_idx], tx_range[devices[h].tp.max_tx_idx]);
                

                if(AUTO_START){
                    //adv_start(false);
                    
                }
                bsp_board_leds_off();
                bsp_board_led_on(led_green); // 0=yellowGreen (right), 1=RED (left), 2=GREEN (left)
            }else{ // is peripheral
                //IS_SENSOR_NODE = true;
                //devices[h].peri.type = info[2];
                devices[h].peri.chosenRelay = true;
            
                if(!devices[h].connected){
                    if(devices[h].peri.type == 1){
                        relay_info.count_type1++;
                        //if(devices[handle].peri.askedAdvPhone){
                        //    relay_info.count_type1_sink--;
                        //}
                    }else if(devices[h].peri.type == 2){
                        relay_info.count_type2++;
                    }else if(devices[h].peri.type == 3){
                        relay_info.count_type3++;
                    }
                }
                if(relay_info.count_type3 + relay_info.count_type2 + relay_info.count_type1 == 6 && AUTO_START){
                    adv_start(false);
                    
                }
                bsp_board_leds_off();
                bsp_board_led_on(led_green); // 0=yellowGreen (right), 1=RED (left), 2=GREEN (left)
                //if(!TEST_RSSI && !PERIS_DO_TP){
                //    devices[h].tp.min_rssi_limit = -75;
                //    devices[h].tp.max_rssi_limit = -68;
                //    devices[h].tp.ideal = -72;
                //}
                devices[h].tp.ema_rssi = 0;//(float) devices[h].tp.ideal * 1.0;
                devices[h].tp.received_counter = 0;
                if(PERIS_DO_TP){
                    devices[h].tp.received_counter = -1;
                }
                send_usb_data(true, "Received Peripheral info: IDnum=%d, IDchar=%c, conn=%d, minTP=%d, maxTP=%d", info[1], devices[h].id, devices[h].connected, tx_range[devices[h].tp.min_tx_idx], tx_range[devices[h].tp.max_tx_idx]);
                //if(!TEST_RSSI){
                //    set_periodic_conn_params(h);
                //}else{
                //    devices[h].conn_params = conn_params_test_rssi;
                //    devices[h].periodic_params = conn_params_test_rssi;
                //}
            }
            if(devices[h].tp.tx_idx == 6){
                if(devices[h].tp.max_tx_idx < 6){
                    devices[h].tp.tx_idx = info[4];
                }else{
                    devices[h].tp.tx_idx = 6; // 0 dbm 
                }
            }
        }else{
            //if(!PERIS_DO_TP){
                devices[h].tp.received_counter = -1;
                devices[h].tp.min_rssi_limit = -110;
                devices[h].tp.max_rssi_limit = -1;
            //}
            devices[h].cent.isPhone = true;
            PHONE_HANDLE = h;
            connectedToPhone = true;
            send_usb_data(true, "Received Phone info: IDnum=%d, IDchar=%c, conn=%d, to gather=%d", info[1], devices[h].id, devices[h].connected, info[2]);
        }
        
    }else{
        send_usb_data(true, "Connected again to old device: IDnum=%d, IDchar=%c, conn=%d, to gather=%d", info[1], devices[h].id, devices[h].connected, info[2]);
    }
    //devices[h].peri.chosenRelay = true;
    
    //if(TEST_RSSI && IS_RELAY){
    //    set_peri_window_size(info[1], h);
    //}
    //if(TEST_RSSI && PERIS_DO_TP){
    //    devices[h].tp.max_tx_idx = 6;
    //}
    //if(send_test_rssi || TEST_RSSI){
    //    devices[h].conn_params = conn_params_test_rssi;
    //    devices[h].periodic_params = conn_params_test_rssi;
    //}

}

void set_peri_window_size(char id, int h){
    if(id == 'V'){
      change_ema_coefficient_window_specific(h, 10);
    }else if(id == 'M'){
      change_ema_coefficient_window_specific(h, 20);
    }else if(id == 'L'){
      change_ema_coefficient_window_specific(h, 100);
    }else if(id == 'R'){
      change_ema_coefficient_window_specific(h,75);
    }else if(id == 'E'){
      change_ema_coefficient_window_specific(h, 200);
    }else if(id == 'G'){
      change_ema_coefficient_window_specific(h, 5);
    }else if(id == 'X'){
      change_ema_coefficient_window_specific(h, 50);
    }
    char coef[10] ="";
    sprintf(coef, "%d.%d%d", (int8_t)  devices[h].tp.coefficient, ((int8_t)(devices[h].tp.coefficient*10))%10, ((int8_t)(devices[h].tp.coefficient*100))%10);
    send_usb_data(true, "PERI %s has: WINDOW=%d, coeff=%s, min=%d, max=%d, ideal=%d", devices[h].name, devices[h].tp.window, coef, devices[h].tp.min_rssi_limit, devices[h].tp.max_rssi_limit, devices[h].tp.ideal);
                
}

// Handles for connection to send on request or periodic data
uint16_t PHONE_HANDLE = NRF_SDH_BLE_TOTAL_LINK_COUNT;
uint16_t CHOSEN_HANDLE = NRF_SDH_BLE_TOTAL_LINK_COUNT;
uint16_t RELAY_HANDLE = NRF_SDH_BLE_TOTAL_LINK_COUNT;
// Helper variables for state of self device
bool connectedToPhone = false;
bool is_sending_to_phone = false;
uint8_t periodicRelayCounter = 0;
bool connected_peris[7] = {false, false, false, false, false,false,false};
int8_t peri_relay_count = 0;
bool there_are_stopped = false;
bool sentAllOwnStoredData = false;

// Array for holding device information for all devices
device_info_t devices[NRF_SDH_BLE_TOTAL_LINK_COUNT];
relay_info_t relay_info;

// For keeping track of scanning
char m_target_periph_name[] = "MicrV";
char spec_target_peri[6] = {0};
//#if doStar || DEVICE_ID == 'C'
char *m_target_periph_names[] = {
    "ViQtr",
    "MicrV",
    "Calmr",
    "PhysL",
    "PhysR",
    "EpocX"
    ,"Glucs"
    ,"     "
    ,"     "
    //,"     "
    //,"     "
    //,"     "
    //,"     "
};
//#else
//char *m_target_periph_names[] = {
//    //"ViQtr",
//    //"MicrV",
//    "Calmr",
//    //"PhysL",
//    //"PhysR",
//    //"EpocX",
//    //"Glucs",
//    "     "
//    ,"     "
//    ,"     "
//    ,"     "
//    ,"     "
//    //,"     "
//};
//#endif
int find_idx_of_relayed(char id){
    int result = 0;
    //for(int i=NRF_SDH_BLE_TOTAL_LINK_COUNT-1; i>=0; i--){
    for(int i=0; i<NRF_SDH_BLE_TOTAL_LINK_COUNT; i++){
        if(devices[i].id == id){
            return i;
        }else if(devices[i].id == 255 && !devices[i].connected && i >= result){
            result = i;
        }
    }
    return result;
}
int og_stored_bytes = 0;
char* getNameFromId(char id){
    switch(id){
        case 'V':
            return "ViQtr";
        case 'M':
            return "MicrV";
        case 'C':
            return "Calmr";
        case 'L':
            return "PhysL";
        case 'R':
            return "PhysR";
        case 'E':
            return "EpocX";
        case 'G':
            return "Glucs";
        case 'P':
            return "Phone";
        default:
            return "     ";
    }
}


void remove_peri_from_scan_list(char* name){
    bool removed = false;
    char msg[200] = "Removed ";
    sprintf(msg + strlen(msg), "%s from scan list, Current:[", name);
    int size = sizeof(m_target_periph_names) / sizeof("Calmr");
    for(int i=0; i<size; i++){
        if(strncmp(m_target_periph_names[i], name, 5) == 0){
            m_target_periph_names[i] = "     ";
            //send_usb_data(true, "");
            removed = true;
        }else if(strncmp(m_target_periph_names[i], "     ",5) != 0){
            strcat(msg, m_target_periph_names[i]);
            strcat(msg, ", ");
        }
    }
    //if(removed){
        //msg[strlen(msg) - 3] = ']';
        msg[strlen(msg) - 2] = ']';
        msg[strlen(msg) - 1] = '\0';
        send_usb_data(true, msg);
    //}
}

void remove_all_from_scan_list(void){
    if(doStar){return;}
    int size = sizeof(m_target_periph_names) / sizeof("Calmr");
    send_usb_data(true, "remove all Size array = %d", size);
    for(int i=0; i<size; i++){
        m_target_periph_names[i] = "     ";
    }
    send_usb_data(true, "Post remove all Size array = %d", size);
}

int count_peri_from_scan_list(void){
    int result = 0;
    int size =sizeof(m_target_periph_names) / sizeof("Calmr");
    for(int i=0; i<size; i++){
        if(!strncmp(m_target_periph_names[i], "     ", 5) == 0){
            result++;
        }
    }
    send_usb_data(true, "There are %d in scan list, arr_size=%d", result, size);
    return result;
}

void show_scan_list(void){
    char msg[200] = "Scan List [";
    //sprintf(msg + strlen(msg), "%s to scan list, Current:[", name);
    int size =sizeof(m_target_periph_names) / sizeof("Calmr");
    for(int i=0; i<size; i++){
        //if(strncmp(m_target_periph_names[i],  "     ",5) == 0 && !added){
            //m_target_periph_names[i] = "     ";
            strcat(msg, m_target_periph_names[i]);
            strcat(msg, ", ");
            //added = true;
        //}else if(strncmp(m_target_periph_names[i], "     ",5) != 0){
        //    strcat(msg, m_target_periph_names[i]);
        //    strcat(msg, ", ");
        //}
    }
    //if(added){
        //msg[strlen(msg) - 3] = ']';
        msg[strlen(msg) - 2] = ']';
        msg[strlen(msg) - 1] = '\0';
        send_usb_data(true, msg);
    //}
}
void add_peri_to_scan_list(char* name){
    bool added = false;
    char msg[200] = "Added ";
    sprintf(msg + strlen(msg), "%s to scan list, Current:[", name);
    int size = sizeof(m_target_periph_names) / sizeof("Calmr");
    for(int i=0; i<size; i++){
        if(strncmp(m_target_periph_names[i],  "     ",5) == 0 && !added){
            m_target_periph_names[i] = name;//"     ";

            if(strncmp(m_target_periph_names[i], name, 5) != 0){
                strncpy(m_target_periph_names[i], name, 5);
            }
            strcat(msg, m_target_periph_names[i]);
            strcat(msg, ", ");
            added = true;
        }else if(strncmp(m_target_periph_names[i], "     ",5) != 0){
            strcat(msg, m_target_periph_names[i]);
            strcat(msg, ", ");
        }
    }
    if(added){
        //msg[strlen(msg) - 3] = ']';
        msg[strlen(msg) - 2] = ']';
        msg[strlen(msg) - 1] = '\0';
        send_usb_data(true, msg);
    }
}

bool scanning_for_on_req = false;

// System configuration
int SYSTEM_COMM_MODE = 1; // 0=Nonsensor relay, 1=One sensor relay, 2=Two sensor relays, 3=No relay ??0=RELAY_REQ_ON_QUEUE, 1=RELAY_PERIS_CHOOSE
int IS_SENSOR_NODE = 1;
int RELAYING_MODE = 1; // 0=All only relay, 1=Periodic relay On-req choose, 2=All choose ??3=Star to phone -> Same as 3-No relay, dont use 3 here
uint8_t SEND_TYPE = 0; // 0 = relay, 1 = request, 2 = periodic, 3 = rare
bool IS_RELAY = false; //(DEVICE_ID == 'X') || (EXISTING_RELAY);
bool IS_TESTING = true;
bool send_test_rssi = false;

bool done_onreq_adv_for_phone = false;
bool got_adv_phone = false;
int count_onreq = 0;
int count_onreq_not_phone = 0;
int count_periodic = 0;



//int get_equivalent_rssi(uint16_t h, int new_rssi){
//    uint8_t tx_id = devices[h].tp.tx_idx;
//    int8_t eq_rssi_from_cent = rssi - devices[h].tp.rssi + devices[h].tp.tx_range[tx_id];
//}
//#if RELAY_REQ_ON_QUEUE
// Queue of peripherals to be relayed on request
on_req_queue_t* on_req_queue = NULL;
on_req_queue_t* last_on_req_queue = NULL;
int8_t last_rssi_test = 0;
uint8_t last_pref_rssi_test = 255;
//#endif
//#endif

//void register_device_info(uint16_t handle, uint8_t id, bool isRequest){
//    if(id == 'P'){
//        memcpy(devices[handle].name, "Phone",5);
//        devices[handle].id = 'P';
//        devices[handle].nameNum = 9;// + '1' + 0;

//        PHONE_HANDLE = handle;
//        devices[handle].cent.isPhone = true;
//        is_sending_to_phone = true;
//        connectedToPhone = true;
        
//        //set_max_tx_specific(handle, 2);
//        uint8_t tlv_name2[4] ={3,DEVICE_ID,SEND_TYPE,IS_RELAY};
//        send_tlv_from_p(handle, tlv_name2, 4);
//        if(send_test_rssi){
//            update_conn_params(handle, conn_params_test_rssi);
//            return;
//        }
//        if(DEVICE_ID != 'C' && DEVICE_ID != 'D'){
//            send_usb_data(false,"REGISTER PHONE isRequest=%d",isRequest);
//            if(isRequest){
//                if(SEND_TYPE){//periodic
//                    update_conn_params(handle, periodic_conn_params);
//                }else{//onrequest
//                    //if(!got_adv_phone){
//                    update_conn_params(handle, peri_request_to_phone_conn_params);
//                    //}
//                }
//            }
//        }else{
//            if(isRequest){
//                if(SEND_TYPE){//periodic
//                    update_conn_params(handle, phone_no_request_conn_params);
//                }else{//onrequest
//                    update_conn_params(handle, phone_with_request_conn_params);
//                }
//            }
//        }

//        //uint8_t tlv_name[11] ={3,DEVICE_ID,SEND_TYPE};
//        //int onReqPeris = 0;
//        //set_max_tx_specific(handle, 2);

//        //if(DEVICE_ID != 'C' && DEVICE_ID != 'D'){
//        //    //send_tlv_from_p(p_evt->conn_handle, tlv_name, 4);
//        //    uint8_t tlv_name2[4] ={3,DEVICE_ID, SEND_TYPE};
//        //    //memcpy(tlv_name+2, DEVICE_NAME, 5);
//        //    send_tlv_from_p(handle, tlv_name2, 4);
//        //    update_conn_params(handle, peri_request_to_phone_conn_params);
//        //}else{
//        //    update_conn_params(handle, phone_with_request_conn_params);
//        //    for(int i=0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
//        //        if(devices[i].nameNum != 255 && !devices[i].peri.isPeriodic){
//        //            tlv_name[4+onReqPeris] = devices[i].nameID;
//        //            onReqPeris++;
//        //        }
//        //    }
//        //    //memcpy(tlv_name+2, DEVICE_NAME, 5);
//        //    send_tlv_from_p(handle, tlv_name, 4+onReqPeris);
//        //}
//    }
//    else if(id > 'D'){
//        memcpy(devices[handle].name, "Peri",4);
//        devices[handle].nameID = (char) id;
//        devices[handle].nameNum = (id - 'I')+1;// + '1' + 0;
//        devices[handle].name[4] =  (id - 'I') + '1';
//        //connected_peris[devices[hp_evt->conn_handle.nameNum-1] = true;
//    }else{
//        memcpy(devices[handle].name, "Cent",4);
//        devices[handle].nameID = (char) id;
//        devices[handle].nameNum = (id - 'C')+1+6;// + '1' + 0;
//        devices[handle].name[4] =  (id - 'C') + '1';
//        //connected_peris[devices[hp_evt->conn_handle.nameNum-1] = true;
//    }
//    devices[handle].name[5] =  '\0';
//}