#include "device_conn.h"
#include "ble_conn_state.h"
#include "common.h"

int RSSI_starting_limit = RSSI_WINDOW_SIZE;
float ema_coefficient = (float) 2/((float) RSSI_WINDOW_SIZE*1.0 + 1);
float ema10_coef = (float) 2/((float) 10*1.0 + 1);
float ema10 = 0.0;
// Maximum tx power index in connections
int max_tx_to_peri = 8;
int max_tx_to_cent = 9;

int min_tx_idx_own = 1;
int max_tx_idx_own = 6;
int default_tx_idx = 6;

int8_t tx_range[9] = {-40, -20, -16, -12, -8, -4, 0, 4, 8};

float change_ema_coefficient_window(int new_windowSize){
    RSSI_starting_limit = new_windowSize;
    ema_coefficient = 2/((float) new_windowSize + 1);
    for (uint8_t i=0; i<NRF_SDH_BLE_TOTAL_LINK_COUNT; i++){
        //if(i < NRF_SDH_BLE_CENTRAL_LINK_COUNT){//central can do min -20
            //devices[id].tp.min_tx_idx = max_tx_to_peri;
            devices[i].tp.window = new_windowSize;
        //}else{
        //    //devices[id].tp.min_tx_idx = max_tx_to_cent;
        //    devices[i].tp.window = new_windowSize - new_windowSize/3;///2;
        //    if(devices[i].tp.window < 0){
        //        devices[i].tp.window = 0;
        //    }
        //}    
        devices[i].tp.coefficient = (float) 2/((float) devices[i].tp.window*1.0 + 1);
    }
}

void change_ema_coefficient_window_specific(int h, int new_windowSize){
    //RSSI_starting_limit = new_windowSize;
    //ema_coefficient = 2/((float) new_windowSize + 1);
    //for (uint8_t i=0; i<NRF_SDH_BLE_TOTAL_LINK_COUNT; i++){
    //    if(i < NRF_SDH_BLE_CENTRAL_LINK_COUNT){//central can do min -20
    //        //devices[id].tp.min_tx_idx = max_tx_to_peri;
    //        devices[i].tp.window = new_windowSize;
    //    }else{
    //        //devices[id].tp.min_tx_idx = max_tx_to_cent;
    //        devices[i].tp.window = new_windowSize - new_windowSize/3;///2;
    //        if(devices[i].tp.window < 2){
    //            devices[i].tp.window = 2;
    //        }
    //    }    
    //    devices[i].tp.coefficient = (float) 2/((float) devices[i].tp.window*1.0 + 1);
    //}

    RSSI_starting_limit = new_windowSize;
    devices[h].tp.window = new_windowSize;
    devices[h].tp.coefficient = (float) 2/((float) devices[h].tp.window*1.0 + 1);
}

void change_rssi_margins_peri(int min, int max, int ideal){
    for (uint8_t i=0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
        devices[i].tp.min_rssi_limit = min;
        devices[i].tp.max_rssi_limit = max;
        devices[i].tp.ideal = ideal;
    }
}

void change_rssi_margins_peri_specific(int h, int min, int max, int ideal){
    //for (uint8_t i=0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
        devices[h].tp.min_rssi_limit = min;
        devices[h].tp.max_rssi_limit = max;
        devices[h].tp.ideal = ideal;
        if(devices[h].tp.received_counter != -1){
            devices[h].tp.received_counter = 0;
        }
        send_usb_data(true, "New SPEC margins for %s: min=%d, max=%d ideal=%d", devices[h].name, devices[h].tp.min_rssi_limit, devices[h].tp.max_rssi_limit, devices[h].tp.ideal);
                
    //}
}

void change_rssi_margins_cent(int min, int max, int ideal){
    for (uint8_t i=NRF_SDH_BLE_CENTRAL_LINK_COUNT; i<NRF_SDH_BLE_TOTAL_LINK_COUNT; i++){
        devices[i].tp.min_rssi_limit = min;
        devices[i].tp.max_rssi_limit = max;
        devices[i].tp.ideal = ideal;
    }
    relay_info.pref_min_rssi_limit = min;
    relay_info.pref_max_rssi_limit = max;
    relay_info.pref_ideal_rssi = ideal;
}

uint32_t change_conn_tx(int8_t power, int8_t handle, int8_t ema, int8_t rssi){//, int8_t original, bool isCentral){
    uint32_t ret;
    ret = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_CONN, handle, power);
    //if(send_test_rssi && ret == NRF_SUCCESS){
    //    //send_usb_data(false, "Increased manually TX power to %s to %d dBm", devices[handle].name, power);
    //}

    int j=0;
    for(int i = 0; i <= max_tx_idx_own; i++){
        if(tx_range[i] == power){
            j = i;
            break;
        }
    }
    uint8_t tlv_ownRssiChange[4] = {7, j, ema, rssi};

    if(handle < NRF_SDH_BLE_CENTRAL_LINK_COUNT){//IS_RELAY){
        tlv_ownRssiChange[0] = 7;
    }else{
        tlv_ownRssiChange[0] = 17;
    }

    if(ret == NRF_SUCCESS && j != 0){
        if(WAIT_FOR_TP_CONFIRMATION){                   
            //uint8_t tlv_ownRssiChange[2] = {7, i};
            //send_usb_data(true, "Changed Own TP to %s:%d to %d dBm from %d dBm __ avg=%d, rssiLast=%d", devices[handle].name, handle, devices[handle].tp.tx_range[j], devices[handle].tp.tx_range[devices[handle].tp.own_idx], ema, rssi);
            //send_usb_data(true, "Changed Own TP to %s:%d to %d dBm from %d dBm", devices[handle].name, handle, devices[handle].tp.tx_range[j], devices[handle].tp.tx_range[devices[handle].tp.own_idx]);
            if(handle < NRF_SDH_BLE_CENTRAL_LINK_COUNT){
                send_tlv_from_c(handle, tlv_ownRssiChange, 4, "Changed Own TX pow");
                if(send_test_rssi){
                    package_storage[0].data[3] = j;
                }
            }else{
                send_tlv_from_p(handle, tlv_ownRssiChange, 4);
            }

        
            //devices[h].tp.ema_rssi = expected_rssi;
            //devices[h].tp.incr_counter = 0;
            
        }
        devices[handle].tp.own_idx = j;
        if(PERIS_DO_TP){
            devices[handle].tp.tx_idx = j;
        }
    }else{
        send_usb_data(false, "ERROR:%d, change Own TP to %s:%d to %d dBm from %d dBm, ema=%d, rssiLast=%d", ret, devices[handle].name, handle, devices[handle].tp.tx_range[j], devices[handle].tp.tx_range[devices[handle].tp.own_idx], ema, rssi);
        //devices[h].tp.incr_counter++;
    }
    //usb_data_sender("Conn pow for hndl:%d set to %d, e:%d",handle, power, ret);
    return ret;
}

void do_rssi_ema_self(uint16_t h, int8_t new_rssi){
    if(devices[h].curr_interval != 0 && new_rssi != devices[h].tp.last_rssi && !doing_const_tp){
        tp_timer_start(&devices[h]);
    }

    if(!doTPC || devices[h].tp.last_rssi == new_rssi){
        return;
    }
    float ema_rssi = devices[h].tp.ema_rssi;
    if(new_rssi == 100){
        new_rssi = last_val_rssi;
    }
    ret_code_t ret;
    uint8_t tx_idx = devices[h].tp.tx_idx;
    if(new_rssi != 100){
        
        if(devices[h].tp.skip > 0){
            devices[h].tp.skip--;
            return;
        }
        last_val_rssi = new_rssi;
        //send_usb_data(true, "TEST TPC: ema=%.1f",(double) 5202 * 0.001);
        //send_usb_data(true, "TEST TPC: ema=%.1f",(double) 5.202);
        //send_usb_data(true, "TEST TPC: ema=%f",(double) 5202 * 0.001);
        //send_usb_data(true, "TEST TPC: ema=%f",5.202);
        //send_usb_data(true, "TEST TPC: ema=%d",(int8_t) 5.202);
        
        
        //int8_t eq_rssi = new_rssi - devices[h].tp.rssi + devices[h].tp.tx_range[tx_id];
    
        //devices[h].tp.ema_rssi = ema_coefficient*new_rssi + (1-ema_coefficient)*devices[h].tp.ema_rssi;
        //float ema_rssi = devices[h].tp.ema_rssi;
        ema_rssi = devices[h].tp.coefficient*new_rssi + (1.0-devices[h].tp.coefficient)*devices[h].tp.ema_rssi;
        devices[h].tp.ema_rssi = ema_rssi;
        //if(devices[h].tp.received_counter < RSSI_starting_limit){
        //    devices[h].tp.received_counter++;
        //}else{

        //}
        //int tx_idx = devices[h].tp.tx_idx;
    }

    float expected_rssi;

    devices[h].tp.last_rssi = new_rssi;
    char coef[10] ="";
    sprintf(coef, "%d.%d%d", (int8_t)  devices[h].tp.coefficient, ((int8_t)(devices[h].tp.coefficient*10))%10, ((int8_t)(devices[h].tp.coefficient*100))%10);
    char ema[20] ="";
    sprintf(ema, "%d.%d", (int8_t)  ema_rssi, abs((int8_t)(ema_rssi*10))%10);

    if(devices[h].tp.received_counter < devices[h].tp.window && devices[h].connected && devices[h].tp.received_counter != -1 && !DO_LONG_EMA && !devices[h].tp.do_first_ema){
        //if(devices[h].tp.received_counter == 0){
        //    ema_rssi = (float) new_rssi*1.0;
        //}
        devices[h].tp.received_counter++;
    }else if(devices[h].connected && devices[h].tp.received_counter != -1){
        if(DO_LONG_EMA && devices[h].tp.do_first_ema){
            devices[h].tp.ema_rssi = new_rssi;
            ema_rssi = new_rssi;
            devices[h].tp.do_first_ema = false;
            devices[h].tp.received_counter = devices[h].tp.window;
        }
        if(h == PHONE_HANDLE){
            return;
        }
        if(IS_RELAY){
            tlv_rssiChange[0] = 7;
        }else{
            tlv_rssiChange[0] = 17;
        }
        if(ema_rssi < devices[h].tp.min_rssi_limit * 1.0){//incr tp
            for(int i = devices[h].tp.tx_idx + 1; i <= devices[h].tp.max_tx_idx; i++){
                expected_rssi = ema_rssi + abs(tx_range[i] - tx_range[tx_idx]);
                if(devices[h].tp.ideal <= expected_rssi || i == devices[h].tp.max_tx_idx){
                        tlv_rssiChange[1] = i;
                        tlv_rssiChange[2] = (int8_t) ema_rssi;
                        tlv_rssiChange[3] = (int8_t) new_rssi;

                           
                            ret = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_CONN, h, tx_range[i]);

                            if(ret == NRF_SUCCESS){
                                devices[h].tp.own_idx = i;
                                if(PERIS_DO_TP){
                                    devices[h].tp.tx_idx = i;
                                }
                                 send_usb_data(true, "Increased TP for %s:%d to %d dBm from %d dBm __ ema=%d, min_limit=%d, last_rssi=%d, exp_ema=%d", devices[h].name, h, devices[h].tp.tx_range[i],(int8_t) devices[h].tp.tx_range[tx_idx],(int8_t) devices[h].tp.ema_rssi, devices[h].tp.min_rssi_limit, new_rssi, (int8_t)expected_rssi);
                                devices[h].tp.ema_rssi = expected_rssi;
                                //if(h < NRF_SDH_BLE_CENTRAL_LINK_COUNT){
                                //    send_tlv_from_c(h, tlv_rssiChange, 4, "Incr TX pow");
                                //}else{
                                //    send_tlv_from_p(h, tlv_rssiChange, 4);
                                //}
                                devices[h].tp.skip = 3;
                                add_to_device_tlv_strg(h, tlv_rssiChange, 4);
                            }else{
                                send_usb_data(false, "ERROR:%d, increase Own TP to %s:%d to %d dBm from %d dBm, ema=%d, rssiLast=%d", ret, devices[h].name, h, devices[h].tp.tx_range[i], devices[h].tp.tx_range[devices[h].tp.own_idx], ema, last_val_rssi);
                                //devices[h].tp.incr_counter++;
                            }
                        
                        break;
                }
            }
        }else if(ema_rssi > devices[h].tp.max_rssi_limit){//decr tp
            for(int i = devices[h].tp.tx_idx - 1; i >= devices[h].tp.min_tx_idx; i--){

                expected_rssi = ema_rssi - abs(devices[h].tp.tx_range[i] - devices[h].tp.tx_range[tx_idx]);
                int exp_last_rssi = new_rssi - abs(devices[h].tp.tx_range[i] - devices[h].tp.tx_range[tx_idx]);
                //if(abs(devices[h].tp.ideal - expected_rssi) < 4 || i == devices[h].tp.min_tx_idx){
                //if(expected_rssi >= devices[h].tp.ideal - 3 || i == devices[h].tp.min_tx_idx){
                //ret = change_conn_tx(devices[h].tp.tx_range[i], h);
                //if(ret == NRF_SUCCESS){
                if((expected_rssi <= devices[h].tp.ideal + 3 && expected_rssi >= devices[h].tp.ideal && exp_last_rssi <= devices[h].tp.ideal + 3 && exp_last_rssi >= devices[h].tp.ideal)|| (i == devices[h].tp.min_tx_idx && expected_rssi >= devices[h].tp.ideal)){
                //if((expected_rssi <= devices[h].tp.ideal + 3 && expected_rssi >= devices[h].tp.ideal) || i == devices[h].tp.min_tx_idx){
                    tlv_rssiChange[1] = i;
                    tlv_rssiChange[2] = (int8_t) ema_rssi;
                    tlv_rssiChange[3] = (int8_t) new_rssi;

                        ret = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_CONN, h, tx_range[i]);

                        if(ret == NRF_SUCCESS){
                            devices[h].tp.own_idx = i;
                            if(PERIS_DO_TP){
                                devices[h].tp.tx_idx = i;
                            }
                            send_usb_data(true, "Decreased TP for %s:%d to %d dBm from %d dBm __ ema=%d, max_limit=%d, last_rssi=%d, exp_ema=%d", devices[h].name, h, devices[h].tp.tx_range[i],(int8_t) devices[h].tp.tx_range[tx_idx],(int8_t)  devices[h].tp.ema_rssi, devices[h].tp.max_rssi_limit, new_rssi, (int8_t)expected_rssi);
                            devices[h].tp.ema_rssi = expected_rssi;
                            //if(h < NRF_SDH_BLE_CENTRAL_LINK_COUNT){
                            //    send_tlv_from_c(h, tlv_rssiChange, 4, "Incr TX pow");
                            //}else{
                            //    send_tlv_from_p(h, tlv_rssiChange, 4);
                            //}
                            devices[h].tp.skip = 3;
                            add_to_device_tlv_strg(h, tlv_rssiChange, 4);
                        }else{
                            send_usb_data(false, "ERROR:%d, increase Own TP to %s:%d to %d dBm from %d dBm, ema=%d, rssiLast=%d", ret, devices[h].name, h, devices[h].tp.tx_range[i], devices[h].tp.tx_range[devices[h].tp.own_idx], ema, last_val_rssi);
                                //devices[h].tp.incr_counter++;
                        }
                }
            }
        }
    }
    
    
    //send_usb_data(true, "New TPC: ema=%s, newRssi=%d, coeff=%s, otherTP=%d, ownTP=%d, requestedTP=%d, minLimit = %d, maxLimit=%d", ema, new_rssi, coef, devices[h].tp.tx_idx, devices[h].tp.own_idx, devices[h].tp.requested_tx_idx, devices[h].tp.max_rssi_limit, devices[h].tp.max_rssi_limit);
    //sprintf(ema, "%d.%d%d%d", (int8_t)  ema_rssi, abs((int8_t)(ema_rssi*10))%10, abs((int8_t) (ema_rssi*100))%10, abs((int8_t) (ema_rssi*1000))%10);
    //send_usb_data(true, "New TPC: ema=%s, newRssi=%d, coeff=%s, otherTP=%d, ownTP=%d, requestedTP=%d, minLimit = %d, maxLimit=%d", ema, new_rssi, coef, devices[h].tp.tx_idx, devices[h].tp.own_idx, devices[h].tp.requested_tx_idx, devices[h].tp.max_rssi_limit, devices[h].tp.max_rssi_limit);
    
    //send_usb_data(true, "New TPC2: ema=%d.%d, newRssi=%d, minRSSI=%d, maxRSSI=%d, coeff=%d.%d, otherTP=%d, ownTP=%d", (int8_t) devices[h].tp.ema_rssi, (int8_t) -1* ((int8_t)(devices[h].tp.ema_rssi*10))%10, new_rssi, (int8_t) devices[h].tp.min_rssi_limit, (int8_t) devices[h].tp.max_rssi_limit, (int8_t)  ema_coefficient, (int8_t) ((int8_t)(ema_coefficient*10))%10, devices[h].tp.tx_idx, devices[h].tp.own_idx);
    
}

bool show_rssi = true;
int8_t rl[20] = {};
int8_t el[20] = {};
int rl_max = 20;
int rl_head = 0;
bool requested = false;

void reset_pdr_timer(int h){
    memset(devices[h].tp.cnt_pack, 0, sizeof(devices[h].tp.cnt_pack));
    devices[h].tp.cnt_evt = 0;
    devices[h].tp.first_evt_skip = true;
    devices[h].tp.docountpack = false;
    devices[h].tp.cnt_evt = 0;
    devices[h].tp.evt_head = 0;
    if(!devices[h].tp.do_without_pdr){
        tp_timer_start(&devices[h]);
    }
}

void simple_pdr_reset(int h){
    memset(devices[h].tp.cnt_pack, 0, sizeof(devices[h].tp.cnt_pack));
    devices[h].tp.cnt_evt = 0;
    devices[h].tp.first_evt_skip = true;
    devices[h].tp.docountpack = false;
    devices[h].tp.cnt_evt = 0;
    devices[h].tp.evt_head = 0;
}

void reset_pdr(uint16_t h){ //new
    //devices[h].tp.evt_head = 0;
    devices[h].tp.cnt_pack[devices[h].tp.evt_head] = 0;
    devices[h].tp.pdr_wait_count = -1;
    devices[h].tp.pdr = 0;
    strcpy(devices[h].tp.pdr_str, "0.0%");
}

void reset_tpc(uint16_t h){ //new
    reset_pdr(h);
    devices[h].tp.skip = 3;
    devices[h].tp.ema_rssi = devices[h].tp.ideal;
}

void do_tpc_pdr(uint16_t h){
    if(doStar){
        devices[h].tp.use_pdr = false;
        return;
    }

    int sum = 0;
    int max_idx = PDR_WINDOW;

    if(devices[h].tp.pdr_wait_count < PDR_WINDOW){
        max_idx = devices[h].tp.pdr_wait_count;
    }

    int j = 0;
    for(int i = 0; i <  max_idx - 1; i++){
        j = (devices[h].tp.evt_head + PDR_WINDOW - i) % (PDR_WINDOW + 1);
        sum += devices[h].tp.cnt_pack[j];
    }
    int pretx = 0;
    int posttx = 0;
    if(devices[h].tp.pdr_wait_count < PDR_WINDOW){
        devices[h].tp.pdr = (float) ((sum + (PDR_WINDOW - devices[h].tp.pdr_wait_count) * devices[h].tp.evt_max_p) * 100.0) / (devices[h].tp.evt_max_p*PDR_WINDOW);
    //    if(devices[h].tp.pdr < 76 && devices[h].tp.tx_idx < devices[h].tp.max_tx_idx){
    //        tlv_rssiChange[0] = 6;
    //        tlv_rssiChange[1] = devices[h].tp.tx_idx+1;
    //        tlv_rssiChange[2] = (int8_t) devices[h].tp.ema_rssi;
    //        tlv_rssiChange[3] = (int8_t) devices[h].tp.last_rssi;
            
    //        pretx = devices[h].tp.tx_idx;
    //        posttx = devices[h].tp.tx_idx + 1;
    //        send_usb_data(true, "Request %s:%d to PDR (pre) Increase TP to %d dBm from %d dBm __ ema=%d, min_limit=%d, last_rssi=%d, pdr=%s, pdr_cnt=%d", devices[h].name, h, devices[h].tp.tx_range[posttx],(int8_t) devices[h].tp.tx_range[pretx],(int8_t) devices[h].tp.ema_rssi, devices[h].tp.min_rssi_limit, devices[h].tp.last_rssi, devices[h].tp.pdr_str, devices[h].tp.pdr_wait_count);
    //        //send_usb_data(true, "DO_WITHOUT = %d, PDR = %s, CNTEVT= %d",devices[h].tp.do_without_pdr, devices[h].tp.pdr_str, devices[h].tp.cnt_evt);
    //        add_to_device_tlv_strg(h, tlv_rssiChange, 4);
    //        devices[h].tp.tx_idx = devices[h].tp.tx_idx+1;
            
    //        reset_tpc(h);
    //    }
    }else{
        devices[h].tp.pdr = (float) sum * 100.0 / (devices[h].tp.evt_max_p*PDR_WINDOW);
        if(devices[h].tp.pdr < 76.0 && devices[h].tp.tx_idx < devices[h].tp.max_tx_idx){
            //tlv_rssiChange[0] = 6;
            //tlv_rssiChange[1] = devices[h].tp.tx_idx+1;
            //tlv_rssiChange[2] = (int8_t) devices[h].tp.ema_rssi;
            //tlv_rssiChange[3] = (int8_t) devices[h].tp.last_rssi;

            //pretx = devices[h].tp.tx_idx;
            //posttx = devices[h].tp.tx_idx + 1;            
            //send_usb_data(true, "Request %s:%d to PDR Increase TP to %d dBm from %d dBm __ ema=%d, min_limit=%d, last_rssi=%d, pdr=%s", devices[h].name, h, devices[h].tp.tx_range[posttx],(int8_t) devices[h].tp.tx_range[pretx],(int8_t) devices[h].tp.ema_rssi, devices[h].tp.min_rssi_limit, devices[h].tp.last_rssi, devices[h].tp.pdr_str);

            //add_to_device_tlv_strg(h, tlv_rssiChange, 4);
            //devices[h].tp.tx_idx = devices[h].tp.tx_idx+1;
            
            //reset_tpc(h);
        }
        else if(devices[h].tp.pdr > 90.0 && devices[h].tp.ema_rssi > devices[h].tp.ideal && devices[h].tp.tx_idx - 1 > devices[h].tp.min_tx_idx_curr){
            tlv_rssiChange[0] = 6;
            tlv_rssiChange[1] = devices[h].tp.tx_idx-1;
            tlv_rssiChange[2] = (int8_t) devices[h].tp.ema_rssi;
            tlv_rssiChange[3] = (int8_t) devices[h].tp.last_rssi;

            pretx = devices[h].tp.tx_idx;
            posttx = devices[h].tp.tx_idx - 1;
            send_usb_data(true, "Request %s:%d to PDR Decrease TP to %d dBm from %d dBm __ ema=%d, max_limit=%d, last_rssi=%d, pdr=%s", devices[h].name, h, devices[h].tp.tx_range[posttx],(int8_t) devices[h].tp.tx_range[pretx],(int8_t)  devices[h].tp.ema_rssi, devices[h].tp.max_rssi_limit, devices[h].tp.last_rssi, devices[h].tp.pdr_str);
                     
            add_to_device_tlv_strg(h, tlv_rssiChange, 4);
            devices[h].tp.tx_idx = devices[h].tp.tx_idx-1;
            
            reset_tpc(h);
        }
    }
    sprintf(devices[h].tp.pdr_str, "%d.%d", (int8_t)  devices[h].tp.pdr, abs((int8_t)(devices[h].tp.pdr*10))%10);
}

void do_tpc_rssi(uint16_t h, int8_t new_rssi){
    //if(!doTPC || devices[h].tp.ideal > -50){
    //    return;
    //}
    if(doStar) {//|| devices[h].curr_interval == 0){
      return;
    }
    ret_code_t ret;
    
    last_val_rssi = new_rssi;
    if(devices[h].tp.skip > 0){// && !devices[h].tp.doing_const_test){
        devices[h].tp.skip--;
        return;
    }
    
    uint8_t tx_idx = devices[h].tp.tx_idx;
    float ema_rssi = devices[h].tp.coefficient*new_rssi + (1.0-devices[h].tp.coefficient)*devices[h].tp.ema_rssi;
    devices[h].tp.ema_rssi = ema_rssi;
    float expected_rssi;
    devices[h].tp.last_rssi = new_rssi;

    //char ema[20] ="";
    //sprintf(ema, "%d.%d", (int8_t)  ema_rssi, abs((int8_t)(ema_rssi*10))%10);
    //send_usb_data(true, "NEW rssi=%d, ema = %s, maxLimit= %d, minLimit=%d", new_rssi, ema,  devices[h].tp.max_rssi_limit, devices[h].tp.min_rssi_limit);

    // Increase
    if(ema_rssi < devices[h].tp.min_rssi_limit * 1.0 
        && (!devices[h].tp.use_pdr || (devices[h].tp.pdr_wait_count > 0 && devices[h].tp.pdr < 80.0))
        ){//incr tp

        for(int i = devices[h].tp.tx_idx + 1; i <= devices[h].tp.max_tx_idx; i++){
            expected_rssi = ema_rssi + abs(devices[h].tp.tx_range[i] - devices[h].tp.tx_range[tx_idx]);

            if(devices[h].tp.ideal <= expected_rssi || i == devices[h].tp.max_tx_idx || i == devices[h].tp.tx_idx+2){
                tlv_rssiChange[0] = 6;
                tlv_rssiChange[1] = i;
                tlv_rssiChange[2] = (int8_t) ema_rssi;
                tlv_rssiChange[3] = (int8_t) new_rssi;
                
                send_usb_data(true, "Request %s:%d to RSSI Increase TP to %d dBm from %d dBm __ ema=%d, min_limit=%d, last_rssi=%d, exp_ema=%d, pdr=%s", devices[h].name, h, devices[h].tp.tx_range[i],(int8_t) devices[h].tp.tx_range[tx_idx],(int8_t) devices[h].tp.ema_rssi, devices[h].tp.min_rssi_limit, new_rssi, (int8_t)expected_rssi, devices[h].tp.pdr_str);
                //send_usb_data(true, "DO_WITHOUT = %d, PDR = %s, CNTEVT= %d",devices[h].tp.do_without_pdr, devices[h].tp.pdr_str, devices[h].tp.cnt_evt);
                add_to_device_tlv_strg(h, tlv_rssiChange, 4);
                devices[h].tp.tx_idx = i;
                
                reset_tpc(h);
                break;
            }
        }
    }
    // Decrease
    else if(ema_rssi > devices[h].tp.max_rssi_limit*1.0 
        && (!devices[h].tp.use_pdr || (devices[h].tp.pdr_wait_count == PDR_WINDOW && devices[h].tp.pdr > 84))//82.0))
        && (devices[h].tp.tx_idx > 3 || ema_rssi > devices[h].tp.max_rssi_limit * 1.0 + 2.0)//new
        && (devices[h].tp.tx_idx > 2 || ema_rssi > devices[h].tp.max_rssi_limit * 1.0 + 4.0)){//new
        //decr tp
        if(devices[h].curr_interval == 0){
          return;
        }
        for(int i = devices[h].tp.tx_idx - 1; i >= devices[h].tp.min_tx_idx_curr; i--){

            expected_rssi = ema_rssi - abs(devices[h].tp.tx_range[i] - devices[h].tp.tx_range[tx_idx]) - 2;
            int exp_last_rssi = new_rssi - abs(devices[h].tp.tx_range[i] - devices[h].tp.tx_range[tx_idx]);

            if((expected_rssi <= devices[h].tp.ideal + 3 && expected_rssi >= devices[h].tp.ideal+1) ||
                i == devices[h].tp.min_tx_idx_curr || i == devices[h].tp.tx_idx-2){
                tlv_rssiChange[0] = 6;
                tlv_rssiChange[1] = i;
                tlv_rssiChange[2] = (int8_t) ema_rssi;
                tlv_rssiChange[3] = (int8_t) new_rssi;
  
                send_usb_data(true, "Request %s:%d to RSSI Decrease TP to %d dBm from %d dBm __ ema=%d, max_limit=%d, last_rssi=%d, exp_ema=%d, pdr=%s, doPDR=%d", devices[h].name, h, devices[h].tp.tx_range[i],(int8_t) devices[h].tp.tx_range[tx_idx],(int8_t)  devices[h].tp.ema_rssi, devices[h].tp.max_rssi_limit, new_rssi, (int8_t)expected_rssi, devices[h].tp.pdr_str,devices[h].tp.use_pdr);
                         
                add_to_device_tlv_strg(h, tlv_rssiChange, 4);
                devices[h].tp.tx_idx = i;
                
                reset_tpc(h);
                break;
            }
        }
    }
}

void do_rssi_ema2(uint16_t h, int8_t new_rssi){
    if(devices[h].curr_interval != 0 && !PERIS_DO_TP && !PERI_TP_TIMER && devices[h].handle < NRF_SDH_BLE_CENTRAL_LINK_COUNT){//&& IS_RELAY){ 
        tp_timer_start(&devices[h]);
    }
    if(!doTPC){
        return;
    }

    if(send_test_rssi && show_rssi && devices[h].handle < NRF_SDH_BLE_CENTRAL_LINK_COUNT){//IS_RELAY){
        send_usb_data(false, "New RSSI = %d, currTP = %d", new_rssi, tx_range[devices[h].tp.tx_idx]);
    }
    
    int wait_cnt = 1;
    if(devices[h].tp.window == 2){
        wait_cnt = 2;
    }else if(devices[h].tp.window > 2){
        wait_cnt = 3;
    }

    last_val_rssi = new_rssi;
    if(devices[h].tp.skip > 0 && !devices[h].tp.doing_const_test){
        devices[h].tp.skip--;
        if(devices[h].tp.skip == 0 && PDR_TCP && requested){
            requested = false;
            reset_pdr_timer(h);
        }
        return;
    }
    ret_code_t ret;
    uint8_t tx_idx = devices[h].tp.tx_idx;
    float ema_rssi = devices[h].tp.coefficient*new_rssi + (1.0-devices[h].tp.coefficient)*devices[h].tp.ema_rssi;
    ema10 = ema10_coef*new_rssi + (1.0-ema10_coef)*devices[h].tp.ema_rssi;

    if(devices[h].tp.received_counter == 0){//(!(devices[h].tp.received_counter > 0 && devices[h].tp.wait_avg_cnt > 1)){ //(rec counter == 0 && wait avg cnt == 1)
        devices[h].tp.ema_rssi = ema_rssi;
    }
    float expected_rssi;

    devices[h].tp.last_rssi = new_rssi;
    char coef[10] ="";
    sprintf(coef, "%d.%d%d", (int8_t)  devices[h].tp.coefficient, ((int8_t)(devices[h].tp.coefficient*10))%10, ((int8_t)(devices[h].tp.coefficient*100))%10);
    //char ema[20] ="";
    //sprintf(ema, "%d.%d", (int8_t)  ema_rssi, abs((int8_t)(ema_rssi*10))%10);

    if(((DO_LONG_EMA && devices[h].tp.do_first_ema) || devices[h].tp.ema_rssi == 0) && devices[h].tp.wait_avg_cnt <= 1){
        devices[h].tp.ema_rssi = new_rssi;
        ema_rssi = new_rssi;
        devices[h].tp.do_first_ema = false;
    }

    if(devices[h].tp.received_counter > 0){
        devices[h].tp.received_counter--;
        if(devices[h].tp.wait_avg_cnt >= 1){
            devices[h].tp.ema_rssi += ((float) new_rssi)/devices[h].tp.wait_avg_cnt;
            ema_rssi = devices[h].tp.ema_rssi;
            //char ema2[20] ="";
            //sprintf(ema2, "%d.%d", (int8_t) devices[h].tp.ema_rssi, abs((int8_t)(devices[h].tp.ema_rssi*10))%10);
            //send_usb_data(true, "Wait counter = %d, received cnt = %d, ema = %s", devices[h].tp.wait_avg_cnt, devices[h].tp.received_counter,ema2);
        }

        //return;
    } 
    char ema[20] ="";
    sprintf(ema, "%d.%d", (int8_t)  ema_rssi, abs((int8_t)(ema_rssi*10))%10);

    float retr = 0.0;
    int max = MAX_EVENT_WINDOW;
    if(devices[h].tp.window + 1 < MAX_EVENT_WINDOW){
        max = devices[h].tp.window + 1;
    }

    if(PDR_TCP && devices[h].tp.cnt_evt >= max){
        int j = (devices[h].tp.evt_head + MAX_EVENT_WINDOW - 1)% MAX_EVENT_WINDOW;
        for(int i = 0; i <  max - 1; i++){
            retr += devices[h].tp.cnt_pack[j];
            j = (j + MAX_EVENT_WINDOW - 1)% MAX_EVENT_WINDOW;
        }
        int max_p = ((max - 1) * devices[h].tp.evt_max_p);
        retr = 100*retr/max_p;
    }
    //if(devices[h].tp.do_without_pdr){
    //    retr = 78.0;
    //}
    char retr_str[20] ="";
    sprintf(retr_str, "%d.%d", (int8_t)  retr, abs((int8_t)(retr*10))%10);
    //if(PDR_TCP && devices[h].tp.cnt_evt < MAX_EVENT_WINDOW){
    //    retr = 0.0;
    //}

    //ema[20] ="";
    //sprintf(ema, "%d.%d", (int8_t)  ema_rssi, abs((int8_t)(ema_rssi*10))%10);

    //char emad[20] ="";
    //sprintf(emad, "%d.%d", (int8_t)  devices[h].tp.ema_rssi, abs((int8_t)(devices[h].tp.ema_rssi*10))%10);

    //send_usb_data(true, "EMA = %s, EMA D cnt = %s, new rssi = %d, rec_cnt = %d", ema, emad, new_rssi,devices[h].tp.received_counter);
    
    if(devices[h].tp.received_counter == 0){
        
        int est_tx = tx_idx;
        //if(devices[h].tp.requested_tx_idx != -1){
        //    est_tx = devices[h].tp.requested_tx_idx;
        //}
        //int estimated_rssi_in_other = new_rssi - devices[h].tp.tx_range[est_tx] + devices[h].tp.tx_range[devices[h].tp.own_idx];

        if(h == PHONE_HANDLE){
            return;
        }
        if(PDR_TCP  && !devices[h].tp.do_without_pdr && devices[h].tp.cnt_evt < max){
            return;
        }
        if(PDR_TCP && !devices[h].tp.do_without_pdr ){
            uint8_t new_txidx = 0;
            if(retr > 85.0 && ema_rssi > devices[h].tp.ideal && tx_idx > devices[h].tp.min_tx_idx){//old 87
                new_txidx = tx_idx - 1;
                expected_rssi = ema_rssi + abs(devices[h].tp.tx_range[new_txidx] - devices[h].tp.tx_range[tx_idx]);
                if(devices[h].handle < NRF_SDH_BLE_CENTRAL_LINK_COUNT){//IS_RELAY){
                    tlv_rssiChange[0] = 6;
                }else{
                    tlv_rssiChange[0] = 16;
                }
            
                tlv_rssiChange[1] = new_txidx;
                tlv_rssiChange[2] = (int8_t) ema_rssi;
                tlv_rssiChange[3] = (int8_t) new_rssi;
                send_usb_data(true, "Request %s:%d to PDRRR Decrease TP to %d dBm from %d dBm __ PDR = %s, ema=%d, max_limit=%d, last_rssi=%d, exp_ema=%d", devices[h].name, h, devices[h].tp.tx_range[new_txidx],(int8_t) devices[h].tp.tx_range[tx_idx], retr_str, (int8_t) devices[h].tp.ema_rssi, devices[h].tp.max_rssi_limit, new_rssi, (int8_t)expected_rssi);
                add_to_device_tlv_strg(h, tlv_rssiChange, 4);
                devices[h].tp.tx_idx = new_txidx;
                devices[h].tp.skip = 3;//TODO: Consider changing for sending an indication with response or write with response
                devices[h].tp.received_counter = wait_cnt;
                devices[h].tp.wait_avg_cnt = wait_cnt;
                devices[h].tp.ema_rssi = 0;//expected_rssi;
                tp_timer_stop(&devices[h]);
                simple_pdr_reset(h);
                requested = true;
                return;
            }else if(retr > 1.0 && retr < 75.0 && ema_rssi < devices[h].tp.ideal && tx_idx < devices[h].tp.max_tx_idx){ //was -70 pdr
                new_txidx = tx_idx + 1;
                expected_rssi = ema_rssi + abs(devices[h].tp.tx_range[new_txidx] - devices[h].tp.tx_range[tx_idx]);
                if(devices[h].handle < NRF_SDH_BLE_CENTRAL_LINK_COUNT){//IS_RELAY){
                    tlv_rssiChange[0] = 6;
                }else{
                    tlv_rssiChange[0] = 16;
                }
            
                tlv_rssiChange[1] = new_txidx;
                tlv_rssiChange[2] = (int8_t) ema_rssi;
                tlv_rssiChange[3] = (int8_t) new_rssi; 
                send_usb_data(true, "Request %s:%d to PDRRR Increase TP to %d dBm from %d dBm __ PDR = %s, ema=%d, min_limit=%d, last_rssi=%d, exp_ema=%d", devices[h].name, h, devices[h].tp.tx_range[new_txidx],(int8_t) devices[h].tp.tx_range[tx_idx], retr_str,(int8_t) devices[h].tp.ema_rssi, devices[h].tp.min_rssi_limit, new_rssi, (int8_t)expected_rssi);
                add_to_device_tlv_strg(h, tlv_rssiChange, 4);
                devices[h].tp.tx_idx = new_txidx;
                devices[h].tp.skip = 3;//TODO: Consider changing for sending an indication with response or write with response
                devices[h].tp.received_counter = wait_cnt;
                devices[h].tp.wait_avg_cnt = wait_cnt;
                devices[h].tp.ema_rssi = 0;//expected_rssi;
                tp_timer_stop(&devices[h]);
                simple_pdr_reset(h);
                requested = true;
                return;
            }
        }
        if(ema_rssi < devices[h].tp.min_rssi_limit * 1.0){//incr tp
            
            for(int i = devices[h].tp.tx_idx + 1; i <= devices[h].tp.max_tx_idx; i++){
                
                expected_rssi = ema_rssi + abs(devices[h].tp.tx_range[i] - devices[h].tp.tx_range[tx_idx]);
                if(devices[h].tp.ideal <= expected_rssi || i == devices[h].tp.max_tx_idx || i == devices[h].tp.tx_idx+2){
                    //ret = change_conn_tx(devices[h].tp.tx_range[i], h);
                    //if(ret == NRF_SUCCESS){
                        if(PDR_TCP && retr > 85.0 && !devices[h].tp.do_without_pdr){
                            if(devices[h].tp.cnt_evt == MAX_EVENT_WINDOW){
                                send_usb_data(true, "Tried to INCREASE to %d from %d dBm, EMA = %s, newRSSI = %d, min = %d, PDR = %s, evtCnt = %d", devices[h].tp.tx_range[i], (int8_t) devices[h].tp.tx_range[tx_idx], ema, new_rssi, devices[h].tp.min_rssi_limit, retr_str, devices[h].tp.cnt_evt);
                                send_usb_data(true, "DO_WITHOUT = %d, PDR = %s, CNTEVT= %d",devices[h].tp.do_without_pdr, retr_str, devices[h].tp.cnt_evt);
                            }
                            return;
                        }
                        if(devices[h].handle < NRF_SDH_BLE_CENTRAL_LINK_COUNT){//IS_RELAY){
                            tlv_rssiChange[0] = 6;
                        }else{
                            tlv_rssiChange[0] = 16;
                        }
                    
                        tlv_rssiChange[1] = i;
                        tlv_rssiChange[2] = (int8_t) ema_rssi;
                        tlv_rssiChange[3] = (int8_t) new_rssi;
                        if(WAIT_FOR_TP_CONFIRMATION){
                            if(devices[h].tp.requested_tx_idx == -1){
                                
                                add_to_device_tlv_strg(h, tlv_rssiChange, 4);

                                devices[h].tp.requested_tx_idx = i;
                                devices[h].tp.ema_rssi = ema_rssi;
                                //send_usb_data(true, "New TPC2: ema=%d.%d, newRssi=%d, minRSSI=%d, maxRSSI=%d, coeff=%d.%d, otherTP=%d, ownTP=%d", (int8_t) devices[h].tp.ema_rssi, (int8_t) -1* ((int8_t)(devices[h].tp.ema_rssi*10))%10, new_rssi, (int8_t) devices[h].tp.min_rssi_limit, (int8_t) devices[h].tp.max_rssi_limit, (int8_t)  ema_coefficient, (int8_t) ((int8_t)(ema_coefficient*10))%10, devices[h].tp.tx_idx, devices[h].tp.own_idx);
                                
                                devices[h].tp.skip = 3; //TODO: Consider changing for sending an indication with response or write with response
                                devices[h].tp.received_counter = wait_cnt;
                                devices[h].tp.wait_avg_cnt = wait_cnt;
                                
                                send_usb_data(true, "Request %s:%d to Increase TP to %d dBm from %d dBm __ ema=%d, min_limit=%d, last_rssi=%d, exp_ema=%d", devices[h].name, h, devices[h].tp.tx_range[i],(int8_t) devices[h].tp.tx_range[tx_idx],(int8_t)  devices[h].tp.ema_rssi, devices[h].tp.min_rssi_limit, new_rssi, (int8_t)expected_rssi);
                                send_usb_data(true, "DO_WITHOUT = %d, PDR = %s, CNTEVT= %d",devices[h].tp.do_without_pdr, retr_str, devices[h].tp.cnt_evt);
                                ////devices[h].tp.ema_rssi = expected_rssi;
                                ////devices[h].tp.incr_counter = 0;
                                ////devices[h].tp.tx_idx = i;
                                //devices[h].tp.skip = 3;
                            }
                        }else{
                            
                            //send_usb_data(true, "New TPC2: ema=%d.%d, newRssi=%d, minRSSI=%d, maxRSSI=%d, coeff=%d.%d, otherTP=%d, ownTP=%d", (int8_t) devices[h].tp.ema_rssi, (int8_t) -1* ((int8_t)(devices[h].tp.ema_rssi*10))%10, new_rssi, (int8_t) devices[h].tp.min_rssi_limit, (int8_t) devices[h].tp.max_rssi_limit, (int8_t)  ema_coefficient, (int8_t) ((int8_t)(ema_coefficient*10))%10, devices[h].tp.tx_idx, devices[h].tp.own_idx);


                            send_usb_data(true, "Request %s:%d to Increase TP to %d dBm from %d dBm __ ema=%d, min_limit=%d, last_rssi=%d, exp_ema=%d", devices[h].name, h, devices[h].tp.tx_range[i],(int8_t) devices[h].tp.tx_range[tx_idx],(int8_t) devices[h].tp.ema_rssi, devices[h].tp.min_rssi_limit, new_rssi, (int8_t)expected_rssi);
                            send_usb_data(true, "DO_WITHOUT = %d, PDR = %s, CNTEVT= %d",devices[h].tp.do_without_pdr, retr_str, devices[h].tp.cnt_evt);
                            add_to_device_tlv_strg(h, tlv_rssiChange, 4);
                            devices[h].tp.tx_idx = i;
                            devices[h].tp.skip = 3;//TODO: Consider changing for sending an indication with response or write with response
                            devices[h].tp.received_counter = wait_cnt;
                            devices[h].tp.wait_avg_cnt = wait_cnt;
                            devices[h].tp.ema_rssi = 0;//expected_rssi;
                        }
                        if(PDR_TCP){
                            tp_timer_stop(&devices[h]);
                            simple_pdr_reset(h);
                        }
                        requested = true;
                        break;
                }
            }
        }else if(ema_rssi > devices[h].tp.max_rssi_limit){//decr tp
            
            for(int i = devices[h].tp.tx_idx - 1; i >= devices[h].tp.min_tx_idx; i--){

                expected_rssi = ema_rssi - abs(devices[h].tp.tx_range[i] - devices[h].tp.tx_range[tx_idx]);
                int exp_last_rssi = new_rssi - abs(devices[h].tp.tx_range[i] - devices[h].tp.tx_range[tx_idx]);
                //if(abs(devices[h].tp.ideal - expected_rssi) < 4 || i == devices[h].tp.min_tx_idx){
                //if(expected_rssi >= devices[h].tp.ideal - 3 || i == devices[h].tp.min_tx_idx){
                //ret = change_conn_tx(devices[h].tp.tx_range[i], h);
                //if(ret == NRF_SUCCESS){
                //if((expected_rssi <= devices[h].tp.ideal + 3 && expected_rssi >= devices[h].tp.ideal && 
                //    exp_last_rssi <= devices[h].tp.ideal + 3 && exp_last_rssi >= devices[h].tp.ideal)|| 
                //    (i == devices[h].tp.min_tx_idx && expected_rssi >= devices[h].tp.ideal)){
                if((expected_rssi <= devices[h].tp.ideal + 3 && expected_rssi >= devices[h].tp.ideal) ||
                    i == devices[h].tp.min_tx_idx || i == devices[h].tp.tx_idx-2){
                    if(PDR_TCP && retr < 80 && !devices[h].tp.do_without_pdr){
                        if(devices[h].tp.cnt_evt == MAX_EVENT_WINDOW){
                            send_usb_data(true, "Tried to DECREASE to %d from %d dBm, EMA = %s, newRSSI = %d, max = %d, PDR = %s, evtCnt = %d", devices[h].tp.tx_range[i],(int8_t) devices[h].tp.tx_range[tx_idx], ema, new_rssi, devices[h].tp.max_rssi_limit, retr_str, devices[h].tp.cnt_evt);
                            send_usb_data(true, "DO_WITHOUT = %d, PDR = %s, CNTEVT= %d",devices[h].tp.do_without_pdr, retr_str, devices[h].tp.cnt_evt);
                        }
                        return;
                    }
                    if(devices[h].handle < NRF_SDH_BLE_CENTRAL_LINK_COUNT){//IS_RELAY){
                        tlv_rssiChange[0] = 6;
                    }else{
                        tlv_rssiChange[0] = 16;
                    }

                    tlv_rssiChange[1] = i;
                    tlv_rssiChange[2] = (int8_t) ema_rssi;
                    tlv_rssiChange[3] = (int8_t) new_rssi;
                    if(WAIT_FOR_TP_CONFIRMATION){
                        if(i != devices[h].tp.requested_tx_idx && devices[h].tp.requested_tx_idx == -1){
                            
                            add_to_device_tlv_strg(h, tlv_rssiChange, 4);
                            
                            //send_usb_data(true, "New TPC2: ema=%d.%d, newRssi=%d, minRSSI=%d, maxRSSI=%d, coeff=%d.%d, otherTP=%d, ownTP=%d", (int8_t) devices[h].tp.ema_rssi, (int8_t) -1* ((int8_t)(devices[h].tp.ema_rssi*10))%10, new_rssi, (int8_t) devices[h].tp.min_rssi_limit, (int8_t) devices[h].tp.max_rssi_limit, (int8_t)  ema_coefficient, (int8_t) ((int8_t)(ema_coefficient*10))%10, devices[h].tp.tx_idx, devices[h].tp.own_idx);
                            devices[h].tp.requested_tx_idx = i;
                            devices[h].tp.ema_rssi = ema_rssi;
                            devices[h].tp.skip = 3;//TODO: Consider changing for sending an indication with response or write with response
                            devices[h].tp.received_counter = wait_cnt;
                            devices[h].tp.wait_avg_cnt = wait_cnt;
                            
                            send_usb_data(true, "Request %s:%d to Decrease TP to %d dBm from %d dBm __ ema=%d, max_limit=%d, last_rssi=%d, exp_ema=%d", devices[h].name, h, devices[h].tp.tx_range[i],(int8_t) devices[h].tp.tx_range[tx_idx],(int8_t)  devices[h].tp.ema_rssi, devices[h].tp.max_rssi_limit, new_rssi, (int8_t)expected_rssi);
                            send_usb_data(true, "DO_WITHOUT = %d, PDR = %s, CNTEVT= %d",devices[h].tp.do_without_pdr, retr_str, devices[h].tp.cnt_evt);
                            ////devices[h].tp.decr_counter = 0;
                            ////devices[h].tp.tx_idx = i;
                            ////devices[h].tp.ema_rssi = expected_rssi;
                
                        }
                        
                    }else{
                        send_usb_data(true, "Request %s:%d to Decrease TP to %d dBm from %d dBm __ ema=%d, max_limit=%d, last_rssi=%d, exp_ema=%d", devices[h].name, h, devices[h].tp.tx_range[i],(int8_t) devices[h].tp.tx_range[tx_idx],(int8_t)  devices[h].tp.ema_rssi, devices[h].tp.max_rssi_limit, new_rssi, (int8_t)expected_rssi);
                        send_usb_data(true, "DO_WITHOUT = %d, PDR = %s, CNTEVT= %d",devices[h].tp.do_without_pdr, retr_str, devices[h].tp.cnt_evt);
                        add_to_device_tlv_strg(h, tlv_rssiChange, 4);
                        devices[h].tp.tx_idx = i;
                        devices[h].tp.ema_rssi = 0;//expected_rssi;
                        devices[h].tp.received_counter = wait_cnt;
                        devices[h].tp.wait_avg_cnt = wait_cnt;
                        devices[h].tp.skip = 3;//TODO: Consider changing for sending an indication with response or write with response

                        
                        
                        //send_usb_data(true, "New TPC2: ema=%d.%d, newRssi=%d, minRSSI=%d, maxRSSI=%d, coeff=%d.%d, otherTP=%d, ownTP=%d", (int8_t) devices[h].tp.ema_rssi, (int8_t) -1* ((int8_t)(devices[h].tp.ema_rssi*10))%10, new_rssi, (int8_t) devices[h].tp.min_rssi_limit, (int8_t) devices[h].tp.max_rssi_limit, (int8_t)  ema_coefficient, (int8_t) ((int8_t)(ema_coefficient*10))%10, devices[h].tp.tx_idx, devices[h].tp.own_idx);


                        
                    }
                    if(PDR_TCP){
                        tp_timer_stop(&devices[h]);
                        simple_pdr_reset(h);
                    }
                    requested = true;
                    break;
                }
            }
        }
    }
    
    
    //send_usb_data(true, "New TPC: ema=%s, newRssi=%d, coeff=%s, otherTP=%d, ownTP=%d, requestedTP=%d, minLimit = %d, maxLimit=%d", ema, new_rssi, coef, devices[h].tp.tx_idx, devices[h].tp.own_idx, devices[h].tp.requested_tx_idx, devices[h].tp.max_rssi_limit, devices[h].tp.max_rssi_limit);
    //sprintf(ema, "%d.%d%d%d", (int8_t)  ema_rssi, abs((int8_t)(ema_rssi*10))%10, abs((int8_t) (ema_rssi*100))%10, abs((int8_t) (ema_rssi*1000))%10);
    //send_usb_data(true, "New TPC: ema=%s, newRssi=%d, coeff=%s, otherTP=%d, ownTP=%d, requestedTP=%d, minLimit = %d, maxLimit=%d", ema, new_rssi, coef, devices[h].tp.tx_idx, devices[h].tp.own_idx, devices[h].tp.requested_tx_idx, devices[h].tp.max_rssi_limit, devices[h].tp.max_rssi_limit);
    
    //send_usb_data(true, "New TPC2: ema=%d.%d, newRssi=%d, minRSSI=%d, maxRSSI=%d, coeff=%d.%d, otherTP=%d, ownTP=%d", (int8_t) devices[h].tp.ema_rssi, (int8_t) -1* ((int8_t)(devices[h].tp.ema_rssi*10))%10, new_rssi, (int8_t) devices[h].tp.min_rssi_limit, (int8_t) devices[h].tp.max_rssi_limit, (int8_t)  ema_coefficient, (int8_t) ((int8_t)(ema_coefficient*10))%10, devices[h].tp.tx_idx, devices[h].tp.own_idx);
    
}

void instruct_tp_change(int h, int i){
     if(devices[h].handle < NRF_SDH_BLE_CENTRAL_LINK_COUNT){//IS_RELAY){
        tlv_rssiChange[0] = 6;
    }else{
        tlv_rssiChange[0] = 16;
    }
    int8_t expected_rssi = 0;
    tlv_rssiChange[1] = i;
    tlv_rssiChange[2] = (int8_t) devices[h].tp.ema_rssi;
    tlv_rssiChange[3] = (int8_t) devices[h].tp.last_rssi;
    int8_t ema_rssi = (int8_t) devices[h].tp.ema_rssi;
    int8_t new_rssi = (int8_t) devices[h].tp.last_rssi;
    int tx_idx = devices[h].tp.tx_idx;
    if(i<devices[h].tp.tx_idx){
        expected_rssi = (int8_t)ema_rssi - abs(devices[h].tp.tx_range[i] - devices[h].tp.tx_range[tx_idx]);
        send_usb_data(true, "Request %s:%d to Decrease TP to %d dBm from %d dBm __ ema=%d, max_limit=%d, last_rssi=%d, exp_ema=%d", devices[h].name, h, devices[h].tp.tx_range[i],(int8_t) tx_range[devices[h].tp.tx_idx],(int8_t)  devices[h].tp.ema_rssi, devices[h].tp.max_rssi_limit, new_rssi, (int8_t)expected_rssi); 
    }else{
        expected_rssi = (int8_t) ema_rssi + abs(devices[h].tp.tx_range[i] - devices[h].tp.tx_range[tx_idx]);
        send_usb_data(true, "Request %s:%d to Increase TP to %d dBm from %d dBm __ ema=%d, max_limit=%d, last_rssi=%d, exp_ema=%d", devices[h].name, h, devices[h].tp.tx_range[i],(int8_t) tx_range[devices[h].tp.tx_idx],(int8_t)  devices[h].tp.ema_rssi, devices[h].tp.max_rssi_limit, new_rssi, (int8_t)expected_rssi);
    }
    add_to_device_tlv_strg(h, tlv_rssiChange, 4);
    //devices[h].tp.tx_idx = i;
    //devices[h].tp.ema_rssi = expected_rssi*1.0;
    //devices[h].tp.skip = 4;//TODO: Consider changing for sending an indication with response or write with response
}


// BELOW NOT USED ANYMORE 
void set_max_tx(int to_peri, int to_cent){

    //min_tx_to_peri = to_peri;
    //min_tx_to_cent = to_cent;
    //uint32_t ret;
    //for (uint8_t i=0; i<NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++){
    //    if(devices[i].connected && devices[i].tp.tx_idx > min_tx_idx_own){
    //        //tx_id = max_tx_to_peri;
    //        ret = change_conn_tx(devices[i].tp.tx_range[min_tx_idx_own], i);
    //        if(ret == NRF_SUCCESS){
    //            tlv_rssiChange[2] = (uint8_t) devices[i].tp.tx_range[min_tx_idx_own];
    //            send_tlv_from_c(i, tlv_rssiChange, 3, "Increase TX");
    //            send_usb_data(false, "Increased TX power to Peri:%d to %d dBm", i, devices[i].tp.tx_range[min_tx_idx_own]);
    //            devices[i].tp.incr_counter = 0;
    //            devices[i].tp.tx_idx = min_tx_idx_own;
                
    //        }else{
    //            send_usb_data(false, "ERROR:%d, Incr TX power to Peri:%d to %d dBm", ret, i, devices[i].tp.tx_range[min_tx_idx_own]);
    //            devices[i].tp.incr_counter++;
    //        }
    //    }
    //    //devices[i].tp.max_tx_idx = max_tx_to_peri;
    //}
    //for (uint8_t i=NRF_SDH_BLE_CENTRAL_LINK_COUNT; i<NRF_SDH_BLE_TOTAL_LINK_COUNT; i++){
    //    if(devices[i].nameNum != 255 && devices[i].tp.tx_idx > max_tx_to_cent){
    //        //tx_id = max_tx_to_cent;
    //        ret = change_conn_tx(devices[i].tp.tx_range[max_tx_to_cent], i);
    //        if(ret == NRF_SUCCESS){
    //            tlv_rssiChange[2] = (uint8_t) devices[i].tp.tx_range[max_tx_to_cent];
    //            send_tlv_from_p(i, tlv_rssiChange, 3);
    //            send_usb_data(false, "Increased TX power to Peri:%d to %d dBm", i, devices[i].tp.tx_range[max_tx_to_cent]);
    //            devices[i].tp.incr_counter = 0;
    //            devices[i].tp.tx_idx = max_tx_to_cent;
                
    //        }else{
    //            send_usb_data(false, "ERROR:%d, Incr TX power to Cent:%d to %d dBm", ret, i, devices[i].tp.tx_range[max_tx_to_cent]);
    //            devices[i].tp.incr_counter++;
    //        }
    //    }
    //    devices[i].tp.max_tx_idx = max_tx_to_cent;
    //}
    //find_best_cent();
}    

void set_max_tx_specific(int handle, int new_min){
//    uint32_t ret;
    
//     // to central
//    if(devices[handle].nameNum != 255 && devices[handle].tp.tx_idx < new_min){
//        //tx_id = max_tx_to_peri;
//        ret = change_conn_tx(devices[handle].tp.tx_range[new_min], handle);
//        if(ret == NRF_SUCCESS){
//            tlv_rssiChange[2] = (uint8_t) devices[handle].tp.tx_range[new_min];
//            if(handle >= NRF_SDH_BLE_CENTRAL_LINK_COUNT){
//                send_tlv_from_p(handle, tlv_rssiChange, 3);
//            }else{
//                send_tlv_from_c(handle, tlv_rssiChange, 3, "Increase TX");
//            }
//            send_usb_data(false, "Increased manually TX power to %s to %d dBm", devices[handle].name, devices[handle].tp.tx_range[new_min]);
//            devices[handle].tp.incr_counter = 0;
//            devices[handle].tp.tx_idx = new_min;
        
//        }else{
//            send_usb_data(false, "ERROR:%d, Incr manually TX power to %s to %d dBm", ret, devices[handle].name, devices[handle].tp.tx_range[new_min]);
//            devices[handle].tp.incr_counter++;
//        }
//    }
//    devices[handle].tp.min_tx_idx = new_min;
}    

void do_rssi_ema(uint16_t h, int new_rssi){
    //if(send_test_rssi){
    //    return;
    //}
    //ret_code_t ret;
    //uint8_t tx_id = devices[h].tp.tx_idx;
    //int8_t eq_rssi = new_rssi - devices[h].tp.rssi + devices[h].tp.tx_range[tx_id];
    
    //devices[h].tp.ema_rssi = ema_coefficient*eq_rssi + (1-ema_coefficient)*devices[h].tp.ema_rssi;
    //float ema_rssi = devices[h].tp.ema_rssi;

    //if(devices[h].tp.received_counter < RSSI_starting_limit){
    //    devices[h].tp.received_counter++;
    //}else{
    //    if(ema_rssi < -80){
    //        for(int i = devices[h].tp.tx_idx - 1; i >= devices[h].tp.max_tx_idx; i--){
    //            if(abs(-75 - ema_rssi + devices[h].tp.tx_range[i]) <= 2 || i == devices[h].tp.max_tx_idx){
    //                ret = change_conn_tx(devices[h].tp.tx_range[i], h);
    //                if(ret == NRF_SUCCESS){
    //                    devices[h].tp.ema_rssi = abs(devices[h].tp.tx_range[i] - devices[h].tp.tx_range[tx_id]) + devices[h].tp.ema_rssi;
                        
    //                    tlv_rssiChange[1] = (uint8_t) devices[h].tp.tx_range[i];

    //                    if(h < NRF_SDH_BLE_CENTRAL_LINK_COUNT){
    //                        send_tlv_from_c(h, tlv_rssiChange, 2, "Incr TX pow");
    //                    }else{
    //                        send_tlv_from_p(h, tlv_rssiChange, 2);
    //                    }

    //                    send_usb_data(true, "Increased TX power to %s:%d to %d dBm __ avg=%d", devices[h].name, h, devices[h].tp.tx_range[i], devices[h].tp.avg_rssi);
    //                    devices[h].tp.incr_counter = 0;
    //                    devices[h].tp.tx_idx = i;
    //                }else{
    //                    send_usb_data(false, "ERROR:%d, Incr TX power to %s:%d to %d dBm", ret, devices[h].name, h, devices[h].tp.tx_range[i-1]);
    //                    devices[h].tp.incr_counter++;
    //                }
    //            }
    //        }
    //    }else if(ema_rssi > -70){
    //        for(int i = devices[h].tp.tx_idx + 1; i <= 9; i++){
    //            if(abs(-75 - ema_rssi + devices[h].tp.tx_range[i]) <= 2){
    //                ret = change_conn_tx(devices[h].tp.tx_range[i], h);
    //                if(ret == NRF_SUCCESS){
    //                    devices[h].tp.ema_rssi = devices[h].tp.ema_rssi - abs(devices[h].tp.tx_range[i] - devices[h].tp.tx_range[tx_id]);

    //                    tlv_rssiChange[1] = (uint8_t) devices[h].tp.tx_range[i];

    //                    if(h < NRF_SDH_BLE_CENTRAL_LINK_COUNT){
    //                        send_tlv_from_c(h, tlv_rssiChange, 2, "Decr TX pow");
    //                    }else{
    //                        send_tlv_from_p(h, tlv_rssiChange, 2);
    //                    }

    //                    send_usb_data(true, "Decreased TX power to %s:%d to %d dBm __ avg=%d", devices[h].name, h, devices[h].tp.tx_range[i], devices[h].tp.avg_rssi);
    //                    devices[h].tp.decr_counter = 0;
    //                    devices[h].tp.tx_idx = i;
    //                }else{
    //                    send_usb_data(false, "ERROR:%d, Decr TX power to %s:%d to %d dBm", ret, devices[h].name, h, devices[h].tp.tx_range[i+1]);
    //                    devices[h].tp.decr_counter++;
    //                }
    //            }
    //        }
    //    }

    //}
    ////return devices[device].tp.ema_rssi;
}