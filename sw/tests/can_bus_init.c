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
#define CTU_CAN_FD_TX_STATUS 0x70
#define CTU_CAN_FD_TX_COMMAND 0x74
#define CTU_CAN_FD_TXTB_INFO 0x76
#define CTU_CAN_FD_TXTB1_DATA1 0X100


uint8_t flag = 0;
void delay(volatile unsigned int count) {
    while (count--) {
        __asm__ volatile ("nop");
    }
}


int main(void) {

	uint32_t rtc_freq = *reg32(&__base_regs, CHESHIRE_RTC_FREQ_REG_OFFSET);
	uint64_t reset_freq = clint_get_core_freq(rtc_freq, 2500);
	uart_init(&__base_uart, reset_freq, __BOOT_BAUDRATE);
	char init[] = "Can Bus Init\r\n";
	uart_write_str(&__base_uart, init, sizeof(init) - 1);
    uart_write_flush(&__base_uart);

	uint32_t device_id = *reg32(&__base_can_bus, CTU_CAN_FD_DEVICE_ID);
	uint32_t version = *reg32(&__base_can_bus, CTU_CAN_FD_VERSION);
    uint32_t can_fd_mode = *reg32(&__base_can_bus, CTU_CAN_FD_MODE);
	uint32_t can_fd_mode_check = 0;
	uint32_t command = 0;
	printf("CAN FD Device ID: 0x%08X\r\n", device_id);
	printf("CAN FD Version: 0x%08X\r\n", version);
	printf("Initial CAN FD MODE: 0x%08X\r\n", can_fd_mode);


	//can_fd_mode |= 0x00000001;	// RST Soft Reset 0->1 -bit 0-
	//*reg32(&__base_can_bus, CTU_CAN_FD_MODE) = can_fd_mode;
	// do {
	// 	can_fd_mode_check = *reg32(&__base_can_bus, CTU_CAN_FD_MODE);
	// 	printf("Can fd mode after RST: 0x%08X\r\n", can_fd_mode_check);
	// } while (can_fd_mode_check & 0x00000001);
	//printf("Can fd mode after RST: 0x%08X\r\n", can_fd_mode_check);

	do {
		can_fd_mode |= 0x00000800;	// SAM_ENABLE -for loopback mode- -bit 11-
		*reg32(&__base_can_bus, CTU_CAN_FD_MODE) = can_fd_mode;
		can_fd_mode_check = *reg32(&__base_can_bus, CTU_CAN_FD_MODE);
		printf("Can fd mode after SAM: 0x%08X\r\n", can_fd_mode_check);

		flag = can_fd_mode_check & 0x02000A10;
	}while (can_fd_mode_check != 0x02000A10);


	// can_fd_mode |= 0x00000800;	// SAM_ENABLE -for loopback mode- -bit 11-
	// *reg32(&__base_can_bus, CTU_CAN_FD_MODE) = can_fd_mode;
	// can_fd_mode_check = *reg32(&__base_can_bus, CTU_CAN_FD_MODE);
	// printf("Can fd mode after SAM: 0x%08X\r\n", can_fd_mode_check);

	uint32_t btr=0;
	btr = (4 << 19);    // Time Quanta: 4
	btr |= 29;          // Prop: 29
	btr |= (10 << 7);   // Phase Seg1: 10
	btr |= (10 << 13);  // Phase Seg2: 10
	btr |= (3 << 27);   // SJW: 3
	*reg32(&__base_can_bus, CTU_CAN_FD_BTR_ADDR) = btr; // (29+10+10+1)*4=200*10ns=2us -> 500kbps
	printf("CAN BTR set to: 0x%08X\r\n", btr);
	uint32_t can_btr_check = *reg32(&__base_can_bus, CTU_CAN_FD_BTR_ADDR);
	printf("CAN BTR readback: 0x%08X\r\n", can_btr_check);	

	uint32_t btr_fd=0;
	btr_fd = (1 << 19);    // Time Quanta: 1
	btr_fd |= 29;          // Prop: 29
	btr_fd |= (10 << 7);   // Phase Seg1: 10
	btr_fd |= (10 << 13);  // Phase Seg2: 10
	btr_fd |= (3 << 27);   // SJW: 3
	*reg32(&__base_can_bus, CTU_CAN_FD_BTR_FD_ADDR) = btr_fd; // (29+10+10+1)*1=50*10ns=0.5us -> 2Mbps
	printf("CAN BTR FD set to: 0x%08X\r\n", btr_fd);
	uint32_t can_btr_fd_check = *reg32(&__base_can_bus, CTU_CAN_FD_BTR_FD_ADDR);
	printf("CAN BTR FD readback: 0x%08X\r\n", can_btr_fd_check);

	

	// can_fd_mode |= 0x00200000;   // ILBP -bit 21- ENA need to be 0 to set ILBP
	// *reg32(&__base_can_bus, CTU_CAN_FD_MODE) = can_fd_mode;
	// can_fd_mode_check = *reg32(&__base_can_bus, CTU_CAN_FD_MODE);
	// printf("Can fd mode after ILBP: 0x%08X\r\n", can_fd_mode_check);

	

		can_fd_mode |= 0x00400000;	// ENA set to 1 -bit 22-
	*reg32(&__base_can_bus, CTU_CAN_FD_MODE) = can_fd_mode;
	can_fd_mode_check = *reg32(&__base_can_bus, CTU_CAN_FD_MODE);
	printf("Can fd mode after ENA: 0x%08X\r\n", can_fd_mode_check);






	// uint16_t fault_state_check = *reg32(&__base_can_bus, FAULT_STATE);
	// printf("CAN FAULT STATE register: 0x%04X\r\n", fault_state_check);
	// if((fault_state_check & 0x0007) == 0x0004){
	// 	printf("CAN bus in Bus Off State\r\n");
	// 	while(((fault_state_check = *reg32(&__base_can_bus, FAULT_STATE)) & 0x0007) == 0x0004){
	// 		// wait until bus recovers
	// 		printf("Waiting to recover from Bus Off...\r\n");
	// 		command |= 0X00000010;
	// 		*reg32(&__base_can_bus, CTU_CAN_FD_COMMAND) = command;//ERCRST 1
	// 		delay(1000000);
	// 		command &= 0xffffffef;
	// 		*reg32(&__base_can_bus, CTU_CAN_FD_COMMAND) = command; //ERCRST 0
	// 	}
	// }
	// if ((fault_state_check & 0x0007) == 0x0001) {
	// 	printf("CAN bus in Error Active State\r\n");
	// } else if ((fault_state_check & 0x0007) == 0x0002) {
	// 	printf("CAN bus in Error Passive State\r\n");
	// }

	can_fd_mode_check = *reg32(&__base_can_bus, CTU_CAN_FD_MODE);
	printf("CAN FD MODE check bit 1 BMM and bit 6: 0x%08X\r\n", can_fd_mode_check);

	uint32_t tx_status = *reg32(&__base_can_bus, CTU_CAN_FD_TX_STATUS);
	printf("CAN TX STATUS register: 0x%08X\r\n", tx_status);

	uint32_t txtb_info = *reg32(&__base_can_bus, CTU_CAN_FD_TXTB_INFO);
	printf("CAN TXTB INFO register: 0x%08X\r\n", txtb_info);

	uint32_t frame_format_w = 0;
	frame_format_w |= 0x00000004; //DLC = 4 bytes 
	frame_format_w |= 0x00000100; //LBPF loopback frame
	*reg32(&__base_can_bus, CTU_CAN_FD_TXTB1_DATA1) = frame_format_w;

	uint32_t identifier_w = 0x00DC0000; // IDE = 55 standard IDE
	*reg32(&__base_can_bus, CTU_CAN_FD_TXTB1_DATA1 + 4) = identifier_w;
	*reg32(&__base_can_bus, CTU_CAN_FD_TXTB1_DATA1 + 10) = 0xAABBCCDD; //data bytes 0-3

	uint16_t tx_command = 0x0102; //command READY to TXTB1
	*reg32(&__base_can_bus, CTU_CAN_FD_TX_COMMAND) = tx_command;

	// issue command "Set ready" only when unit is not bus-off or it will go "Aborted"
	tx_status = *reg32(&__base_can_bus, CTU_CAN_FD_TX_STATUS); //check TX status after issuing READY
	printf("CAN TX STATUS register: 0x%08X\r\n", tx_status);


	uint32_t rx_status = *reg32(&__base_can_bus, CTU_CAN_FD_RX_STATUS); //check RX status after issuing READY
	printf("CAN RX STATUS register: 0x%08X\r\n", rx_status);

	return 0;
}
