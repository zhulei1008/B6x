//SYS_DEBUG
assign sys_debug[15:0] = (SYSCFG.sys_debug_sel[1:0]  ==2'b00 ) ? rf_debug[15:0] :
                         (SYSCFG.sys_debug_sel[1:0]  ==2'b01 ) ? {ble_dbg1[7:0],ble_dbg0[7:0]} :
                         (SYSCFG.sys_debug_sel[1:0]  ==2'b10 ) ? mdm_debug[15:0] :
                         (SYSCFG.sys_debug_sel[1:0]  ==2'b11 ) ? {usb_debug[15:0]} : 16'h0 ;


//RF DEBUG
assign rf_debug[15:0] =  RF.rf_dbg_sel[1:0] == 2'b11 ? calib_cnt_debug[15:0] :
                         RF.rf_dbg_sel[1:0] == 2'b10 ? {2'b0,rf_debug1[13:0]} :
                         RF.rf_dbg_sel[1:0] == 2'b01 ? {2'b0,rf_debug0[13:0]} : 16'h0;

//RF_DBG_TX
assign rf_debug0[13:0] = {
                            fsm_in_tx_en,                                                             //[13]
                            fsm_en_pll,    fsm_tx_ldo_en, fsm_afc_en, fsm_gain_cal_en, fsm_cal_clken, //[12:8]
							fsm_out_tx_en, PLL_LOCK,   FAST_LOCK_DONE,                                //[7:5]
                            state[4:0]                                                                //[4:0]
                         }; 
//RF_DBG_RX
assign rf_debug1[13:0] = {
                            2'b0,
                            fsm_in_rx_en,  fsm_en_pll,     fsm_rx_ldo_en, fsm_afc_en,     //[11:8]
                            fsm_out_rx_en, fsm_bpf_cal_en, PLL_LOCK,      FAST_LOCK_DONE, //[7:4]
                            fsm_en_lna,    fsm_en_pa,      fsm_cal_clken, fsm_en_agc      //[3:0]  
                         };


//MDM DEBUG
 mdm_debug[15:0] = (MDM.debug_mode[3:0] == 4'b0001 ) ? mdm_debug1 :
                   (MDM.debug_mode[3:0] == 4'b0010 ) ? mdm_debug2 :
                   (MDM.debug_mode[3:0] == 4'b0011 ) ? mdm_debug3 :
                   (MDM.debug_mode[3:0] == 4'b0100 ) ? mdm_ext_debug0 :
                   (MDM.debug_mode[3:0] == 4'b0101 ) ? mdm_ext_debug1 : 16'h0 ;

assign mdm_debug1[15:0] = {
                            1'b0,
                            iq_invert,        acc_invert,    rx_invert,            //[14:12]
                            tx_invert,        rx_data,       rx_valid,  sync_pulse,//[11:8]
                            vb_decoder_flush, receive_start, tx_valid,  tx_data,   //[7:4]
                            mdm_tx_en,        mdm_rate[0],   mdm_frstn, mdm_fclk   //[3:0]
                          }; 

assign mdm_debug2[15:0] = {
                            3'b000, 
                            tx_out[7:0],                                       //[12:5]
                            tx_data,    mdm_tx_en, tx_rate, mdm_frstn, mdm_fclk//[4:0]
                          };  
             
assign mdm_debug3[15:0] = {
                                4'b0,
                                demod_data,       demod_clk,     demod_ready, rx_data,//[11:8] 
                                rx_valid,         sync_pulse,    q_in,        i_in,   //[7:4]
                                vb_decoder_flush, receive_start, mdm_rate[0], mdm_fclk//[3:0]
                          }; 

assign mdm_ext_debug0[15:0] = {
                                2'b0,
                                tx_dat_sent[7:0],                                 //[13:6]
                                mdm_slot_int, tx_send_bit,                        //[5:4]
                                tx_frd,       ext_tx_dat_wr_pl, mdm_tx_done, tx_en//[3:0]
                              };
                              
assign mdm_ext_debug1[15:0] = {
                                ext_rx_dat[7:0],                                         //[15:8]
                                mdm_slot_int, mdm_sync_err,     rx_data,        rx_valid,//[7:4]
                                rx_fwr,       ext_rx_dat_rd_pl, mdm_sync_found, rx_en    //[3:0]
                              }; 

//USB DEBUG
assign usb_debug[15:0] = SYSCFG.usb_dbg_sel[1:0] == 2'b01 ? {1'b0,usb_gen_clk,2'h0,Usb_debug0[11:0]} :
                         SYSCFG.usb_dbg_sel[1:0] == 2'b10 ? {usb_gen_clk,mem_ncs,mem_nwr,mem_addr[5:0], SIEState[5:0]} : 16'h0 ;
                         
assign Usb_debug0[11:0] = {
                            TxData[7:0],                //[11:4]
                            TxByte, CRCOp, CRCRst, TxBit//[3:0]
                          };
                         
