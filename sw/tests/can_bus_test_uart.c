// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Nicole Narr <narrn@student.ethz.ch>
// Christopher Reinwardt <creinwar@student.ethz.ch>
// Paul Scheffler <paulsc@iis.ee.ethz.ch>

#include "regs/cheshire.h"
#include "dif/clint.h"
#include "dif/uart.h"
#include "params.h"
#include "util.h"
#include <stdio.h>
#include "printf.h"

#define CTU_CAN_FD_DEVICE_ID 0x0
#define CTU_CAN_FD_VERSION 0x2
#define FAULT_STATE 0X2E
#define CTU_CAN_FD_MODE 0x4
#define CTU_CAN_FD_COMMAND 0xC
#define CTU_CAN_FD_BTR_ADDR 0x24
#define CTU_CAN_FD_BTR_FD_ADDR 0x28
#define CTU_CAN_FD_RX_STATUS 0x68
#define CTU_CAN_FD_RX_DATA 0X6C
#define CTU_CAN_FD_TX_STATUS 0x70
#define CTU_CAN_FD_TX_COMMAND 0x74
#define CTU_CAN_FD_TXTB_INFO 0x76
#define CTU_CAN_FD_TXTB1_DATA1 0X100
#define CTU_CAN_FD_YOLO 0X90
void can_init(void);
void can_tx(void);

void delay(volatile unsigned int count) {
    while (count--) {
        __asm__ volatile ("nop");
    }
}

int main(void) {
    uint32_t rtc_freq = *reg32(&__base_regs, CHESHIRE_RTC_FREQ_REG_OFFSET);
	uint64_t reset_freq = clint_get_core_freq(rtc_freq, 2500);
	uart_init(&__base_uart, reset_freq, __BOOT_BAUDRATE);
	char init[] = "Can Bus Test\r\n";
	uart_write_str(&__base_uart, init, sizeof(init) - 1);
    uart_write_flush(&__base_uart);

    uint32_t device_id = *reg32(&__base_can_bus, CTU_CAN_FD_DEVICE_ID);
    printf("CAN Version  = 0x%x (expected 0x0204CAFD)\n",device_id);
    uint32_t yolo = *reg32(&__base_can_bus, CTU_CAN_FD_YOLO);
    printf("CAN_REG_YOLO = 0x%x (expected 0xDEADBEEF)\n",yolo);
 
    unsigned int btr = 0;
 
    printf("CAN Reset \n");
    uint32_t can_fd_mode_check = *reg32(&__base_can_bus, CTU_CAN_FD_MODE);
    printf("Can fd mode before RESET: 0x%x\r\n", can_fd_mode_check);
    //*reg32(&__base_can_bus, CTU_CAN_FD_MODE) = 1;
    *reg32(&__base_can_bus, CTU_CAN_FD_MODE) |= 0x00000001;
    delay(100);
    can_fd_mode_check = *reg32(&__base_can_bus, CTU_CAN_FD_MODE);
    printf("Can fd mode after RESET: 0x%08X\r\n", can_fd_mode_check);
 
    //PIN tx D20  on FMC
    //PIN rx D23 on FMC
 
    // clock 100 MHz
    //BTS 500K
    /*
    printf("CAN Init @500K with clock 100MHz \n");
    btr = (4<<19);    //time quanta
    btr |= 29;        // prop
    btr |= (10<<7);   // phase 1
    btr |= (10<<13);  // phase 2
    btr |= (3<<27);   // SJW
 
    // (29+10+10+1)*4 = 200 * 10ns = 2us = 500 Kbit
    */
 
 
    // clock 10 MHz
 
    // BTS 1M
    /*
    printf("CAN Init @1M with clock 10MHz \n");
    btr = (1<<19);    //time quanta
    btr |= 5;         // prop
    btr |= (2<<7);    // phase 1
    btr |= (2<<13);   // phase 2
    btr |= (3<<27);   // SJW
    // (5+2+2+1)*1 = 10 * 100ns = 1us = 1 Mbit
    */
 
    // clock 10 MHz
    // BTS 1M
 
    printf("CAN Init @1M with clock 20MHz \n");
    btr = (1<<19);    //time quanta
    btr |= 9;         // prop
    btr |= (5<<7);    // phase 1
    btr |= (5<<13);   // phase 2
    btr |= (3<<27);   // SJW
    // (9+5+5+1)*1 = 20 * 50ns = 1us = 1 Mbit
 
 
 
    *reg32(&__base_can_bus, CTU_CAN_FD_BTR_ADDR) = btr;

    printf("CAN SAM \n");
    *reg32(&__base_can_bus, CTU_CAN_FD_MODE) |= 1 << 11;
    can_fd_mode_check = *reg32(&__base_can_bus, CTU_CAN_FD_MODE);
    printf("Can fd mode after SAM: 0x%08X\r\n", can_fd_mode_check);

    printf("CAN ILBP \n");
    *reg32(&__base_can_bus, CTU_CAN_FD_MODE) |= 1 << 21;
    can_fd_mode_check = *reg32(&__base_can_bus, CTU_CAN_FD_MODE);
    printf("Can fd mode after ILBP: 0x%08X\r\n", can_fd_mode_check);
 
    printf("CAN Enable \n");
    *reg32(&__base_can_bus, CTU_CAN_FD_MODE) |= 1 << 22;

    can_fd_mode_check = *reg32(&__base_can_bus, CTU_CAN_FD_MODE);
    printf("Can fd mode after Enable: 0x%08X\r\n", can_fd_mode_check);
 
    


    unsigned int frame_format_word = 0;
    unsigned int pattern = 0xAABBCCDD;
    unsigned int pattern1 = 0x12345678;
    frame_format_word |= 8; //DLC
 
    //normal frame
    frame_format_word |= (0 << 7);  // NO CAN FD Frame
    frame_format_word |= (0 << 9);  // NO CAN switch bitrate
 
    printf("CAN fill buffer 1 \n");
    *reg32(&__base_can_bus, CTU_CAN_FD_TXTB1_DATA1) = frame_format_word;

    unsigned int id_word = (55 << 18); //id : 55 ()0x37
 
    *reg32(&__base_can_bus, CTU_CAN_FD_TXTB1_DATA1 + 0x04) = id_word;  // identifier base (no extended)
    *reg32(&__base_can_bus, CTU_CAN_FD_TXTB1_DATA1 + 0x08) = 0;
    *reg32(&__base_can_bus, CTU_CAN_FD_TXTB1_DATA1 + 0x0C) = 0;
    *reg32(&__base_can_bus, CTU_CAN_FD_TXTB1_DATA1 + 0x10) = pattern;
    *reg32(&__base_can_bus, CTU_CAN_FD_TXTB1_DATA1 + 0x14) = pattern1;

    unsigned int command = 0;
    command |= 0x2;   //ready command
    command |= (1<<8);  //buffer 1

    printf("CAN command Tx \n");
    *reg32(&__base_can_bus, CTU_CAN_FD_TX_COMMAND) = command;

    uint16_t fault_state_check = *reg32(&__base_can_bus, FAULT_STATE);
	printf("CAN FAULT STATE register: 0x%04X\r\n", fault_state_check);
	if((fault_state_check & 0x0007) == 0x0004){
		printf("CAN bus in Bus Off State\r\n");
		while(((fault_state_check = *reg32(&__base_can_bus, FAULT_STATE)) & 0x0007) == 0x0004){
			// wait until bus recovers
			printf("Waiting to recover from Bus Off...\r\n");
			command |= 0X00000010;
			*reg32(&__base_can_bus, CTU_CAN_FD_COMMAND) = command;//ERCRST 1
			delay(1000000);
			command &= 0xffffffef;
			*reg32(&__base_can_bus, CTU_CAN_FD_COMMAND) = command; //ERCRST 0
		}
	}
	if ((fault_state_check & 0x0007) == 0x0001) {
		printf("CAN bus in Error Active State\r\n");
	} else if ((fault_state_check & 0x0007) == 0x0002) {
		printf("CAN bus in Error Passive State\r\n");
	}

    uint32_t tx_status = *reg32(&__base_can_bus, CTU_CAN_FD_TX_STATUS); //check TX status after issuing READY
	printf("CAN TX STATUS register: 0x%08X\r\n", tx_status);


    //Can RX 
    uint16_t rx_status;
    do{
        rx_status = *reg32(&__base_can_bus, CTU_CAN_FD_RX_STATUS); //check RX status after issuing READY
        printf("CAN RX STATUS register: 0x%04X\r\n", rx_status);
    } while ((rx_status & 0x0001) == 0);

    /* Read frame from RX buffer */
    uint8_t data[64];
    uint32_t tmp;
    uint32_t ffw = *reg32(&__base_can_bus, CTU_CAN_FD_RX_DATA);
    uint32_t id = *reg32(&__base_can_bus, CTU_CAN_FD_RX_DATA);
    uint32_t ts_l = *reg32(&__base_can_bus, CTU_CAN_FD_RX_DATA);
    uint32_t ts_h = *reg32(&__base_can_bus, CTU_CAN_FD_RX_DATA);

    uint32_t rwcnt = (ffw >> 11) & 0x1F; // number of received words
    for(unsigned int i = 0; i< rwcnt; i++){
        tmp = *reg32(&__base_can_bus, CTU_CAN_FD_RX_DATA);
        data[i*4] = tmp & 0xFF; 
        data[i*4+1] = (tmp >> 8) & 0xFF;
        data[i*4+2] = (tmp >> 16) & 0xFF;
        data[i*4+3] = (tmp >> 24) & 0xFF;
    }

    printf("Received %d data words \r\n", rwcnt);
    printf("Identifier: 0x%08X\r\n", id);
    printf("Frame Format Word: 0x%08X\r\n", ffw);
    for(unsigned int j =  0; j< rwcnt*4; j++){
        printf("Recieved data : 0x%02X\r\n", data[j]);
    }

    return 0;
}