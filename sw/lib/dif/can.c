/*
 * can.c
 *
 *  Created on: Feb 11, 2026
 *      Author: Fabio Gorini
 */

#include "dif/can.h"
#include "dif/clint.h"
#include "dif/uart.h"
#include "printf.h"

union Mode_Settings_CTU_CAN_FD mode_settings;
union Status_CTU_CAN_FD status;
union Interrupt_Status_CTU_CAN_FD int_stat;
union Bitrate_CTU_CAN_FD btr;
union Bitrate_FD_CTU_CAN_FD btr_fd;
union Fault_State_CTU_CAN_FD fault_state;
union TX_Status_CTU_CAN_FD tx_status;
union TX_Command_CTU_CAN_FD tx_command;
union ffw_CTU_CAN_FD ffw;
union id_w_CTU_CAN_FD id;
CTU_CAN_FD_rx_frame can_fd_rx_frame;



const char* tx_state_to_str(tx_state_t s) {
    switch (s) {
        case TXT_NOT_EXIST: return "NOT_EXIST";
        case TXT_READY:     return "READY";
        case TXT_TRAN:      return "TRAN";
        case TXT_ABTP:      return "ABTP";
        case TXT_TOK:       return "TOK";
        case TXT_UNUSED:    return "UNUSED";
        case TXT_ERR:       return "ERR";
        case TXT_ABT:       return "ABT";
        case TXT_ETY:       return "ETY";
        case TXT_PER:       return "PER";
        default:            return "UNKNOWN";
    }
}

const char* fault_state_to_str(fault_state_t s) {
    switch (s) {
        case FAULT_UNUSED: return "UNUSED";
        case ERA:          return "ERA";
        case ERP:          return "ERP";
        case FAULT_UNUSED_2:return "UNUSED_2";
        case BOF:          return "BOF";
        default:           return "UNKNOWN";
    }
}

/* wait_ms
 @ description : performs a simple busy‑wait delay loop.
 This provides a crude time delay whose duration depends on
 the CPU clock frequency. Not precise for real-time use.*/
void wait_ms(volatile unsigned int count) {
    while (count--) {
        __asm__ volatile ("nop");
    }
}

/*can_test
 @ description : performs a basic CAN bus test by checking device ID and
 YOLO register values*/
void can_test(void) {
    printf("CAN Version  = 0x%x (expected 0x0204CAFD)\n",MEM(CAN_REG_DEVICE_ID));
    printf("CAN_REG_YOLO = 0x%x (expected 0xDEADBEEF)\n",MEM(CAN_REG_YOLO));   
}

/*can_reset
 @ description : resets the CAN controller by setting the 'rst' bit in the
 Mode Settings register.*/
void can_reset(void) {
    printf("CAN Reset \n");
    mode_settings.all = MEM(CAN_REG_SETTING_MODE);
    mode_settings.bits.rst = 1;
    MEM(CAN_REG_SETTING_MODE) = mode_settings.all;
    //printf("Can CAN REG SETTING MODE during RESET : 0x%08X\r\n", MEM(CAN_REG_SETTING_MODE));
    wait_ms(100);
    //printf("Can CAN REG SETTING MODE after RESET : 0x%08X\r\n", MEM(CAN_REG_SETTING_MODE));
}

/*can_btr
 @ description : configures the Bit Timing Register (BTR) for CAN communication.
 This function sets up the timing parameters for the CAN controller based on
 a 50 MHz clock and desired bit rate of 500 Kbit/s. The timing values are
 calculated to achieve the specified bit rate and are written to the BTR register.*/
void can_btr(void){

    // clock 50 MHz
    // BTS 250K
    // printf("CAN Init @250K with clock 50MHz \n");
    /*
    btr.all = 0;
    btr_fd.all = 0;

    btr.bits.prop = 29;
    btr.bits.ph1  = 10;
    btr.bits.ph2  = 10;
    btr.bits.brp  = 4;
    btr.bits.sjw  = 3;
    MEM(CAN_BTR_ADDR) = btr.all;
    */
    // (29+10+10+1)*4 = 200 * 20ns = 4us = 250 Kbit

    // clock 50 MHz
    // BTS 500K
    btr.all = 0;
    btr_fd.all = 0;
    printf("CAN Init @500K with clock 50MHz \n");
    btr.bits.prop = 29;
    btr.bits.ph1  = 10;
    btr.bits.ph2  = 10;
    btr.bits.brp  = 2;
    btr.bits.sjw  = 3;
    MEM(CAN_BTR_ADDR) = btr.all;

    btr_fd.bits.prop_fd = 29;
    btr_fd.bits.ph1_fd  = 10;
    btr_fd.bits.ph2_fd  = 10;
    btr_fd.bits.brp_fd  = 2;
    btr_fd.bits.sjw_fd  = 3;
    MEM(CAN_BTR_FD_ADDR) = btr_fd.all;
    // (29+10+10+1)*2 = 100 * 20ns = 2us = 500 Kbit

    // clock 50 MHz
    // BTS 1M
    /*
    btr.all = 0;
    btr_fd.all = 0;
    // printf("CAN Init @1M with clock 50MHz \n");
    
    btr.bits.prop = 29;
    btr.bits.ph1  = 10;
    btr.bits.ph2  = 10;
    btr.bits.brp  = 1;
    btr.bits.sjw  = 3;
    MEM(CAN_BTR_ADDR) = btr.all;
    */
    // (29+10+10+1)*1 = 50 * 20ns = 1us = 1 Mbit

    printf("Can CAN BTR (expected 0x1821451D): 0x%08X\r\n", MEM(CAN_BTR_ADDR));
}

/*can_enable
 @ description : enables the CAN controller by setting the 'ena' bit in the
 Mode Settings register. It wait for the bus integration to complete before 
 returning.*/
void can_enable(void){
    printf("Check CAN FD Fault State before enabling: %s\r\n",fault_state_to_str((fault_state_t)(MEM(FAULT_STATE) & 0x07)));

    printf("CAN Enable \n");
    mode_settings.all = MEM(CAN_REG_SETTING_MODE);
    mode_settings.bits.ena = 1;
    MEM(CAN_REG_SETTING_MODE) = mode_settings.all;
    do {
        printf("Check CAN FD Fault State: %s\r\n", fault_state_to_str((fault_state_t)(MEM(FAULT_STATE) & 0x07)));
    } while ((MEM(FAULT_STATE) & 0x0007) != 0x0001);
    printf("Integration DONE (FAULT_STATE expected = ERA): %s\r\n", fault_state_to_str((fault_state_t)(MEM(FAULT_STATE) & 0x07)));
    //printf("Can CAN REG SETTING MODE after ENABLE (expected 0x02400210): 0x%08X\r\n", MEM(CAN_REG_SETTING_MODE));
}

/*can_tx
 @ description : Transmits a CAN frame with the given payload and DLC (Data Length Code).*/
void can_tx(const uint32_t *payload_w32, const union ffw_CTU_CAN_FD *ffw, const union id_w_CTU_CAN_FD *id_w){
    ////printf("CAN fill buffer 1\n");
    MEM(CAN_TXT_BUFFER_1_BASE + 0x00) = ffw->all;
    MEM(CAN_TXT_BUFFER_1_BASE + 0x04) = id_w->all;
    MEM(CAN_TXT_BUFFER_1_BASE + 0x08) = 0;   // timestamp L
    MEM(CAN_TXT_BUFFER_1_BASE + 0x0C) = 0;   // timestamp H

    uint8_t n_bytes = ffw->bits.fdf ? 
                  dlc_to_bytes_fd[ffw->bits.dlc] : 
                  dlc_to_bytes_classic[ffw->bits.dlc];
    uint32_t n_words = (n_bytes + 3) / 4;

    for(uint32_t i=0; i < n_words; i++){
        MEM(CAN_TXT_BUFFER_1_BASE + 0x10 + i*4) = payload_w32[i];
    }

    /* Comando TX */
    tx_command.all = 0;
    tx_command.bits.txcr = HIGH;  // Ready
    tx_command.bits.txb1 = HIGH;      // Buffer 1

    //printf("CAN TX STATUS before command: %s\r\n", tx_state_to_str((tx_state_t)(MEM(TX_STATUS) & 0x0F)));

    //printf("CAN TX COMMAND\n");
    MEM(CAN_TX_COMMAND_ADDR) = tx_command.all;

    //printf("CAN TX STATUS after command: %s\r\n", tx_state_to_str((tx_state_t)(MEM(TX_STATUS) & 0x0F)));

    //printf("CAN FAULT STATE: %s\r\n", fault_state_to_str((fault_state_t)(MEM(FAULT_STATE) & 0x07)));

    do{
        printf("Polling on TX STATUS REGISTER: %s\r\n", tx_state_to_str((tx_state_t)(MEM(TX_STATUS) & 0x0F)));
    }while(((MEM(TX_STATUS) & 0x0F) == TXT_READY));
    printf("CAN TX STATUS register: %s\r\n", tx_state_to_str((tx_state_t)(MEM(TX_STATUS) & 0x0F)));
    }

/*can_rx
 @ description : Receives a CAN frame and prints its contents.*/
void can_rx(CTU_CAN_FD_rx_frame *can_fd_rx_frame) {
    /* Frame reception in automatic mode */

    //printf("CAN Polling RX Buffer \n");
    status.all = MEM(CAN_STATUS_ADDR);
    //printf("Can STATUS register before polling: 0x%01X\r\n",status.bits.rxne);
    do {
    status.bits.rxne = MEM(CAN_STATUS_ADDR) & 0x01;
    } while ((status.bits.rxne) == 0);
    //printf("Can STATUS register after polling: 0x%01X\r\n",status.bits.rxne);

    /* Read frame from RX buffer */
    can_fd_rx_frame->ffw_rx.all = MEM(RX_DATA_ADDR);
    can_fd_rx_frame->id_rx.all = MEM(RX_DATA_ADDR);
    can_fd_rx_frame->timestamp_l_w = MEM(RX_DATA_ADDR);
    can_fd_rx_frame->timestamp_h_w = MEM(RX_DATA_ADDR);

    for(uint32_t i = 0; i < can_fd_rx_frame->ffw_rx.bits.rwcnt - 3; i++){
        can_fd_rx_frame->data_w[i] = MEM(RX_DATA_ADDR);
    }
}
