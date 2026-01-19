// Thales CAN FD test application

#include "regs/cheshire.h"
#include "dif/clint.h"
#include "dif/uart.h"
#include "params.h"
#include "util.h"
#include <stdio.h>
#include "printf.h"

#define MEM(addr) (*(volatile uint32_t *)(addr))

#define CAN_BASE_ADDRESS 0x0300A000
#define CAN_REG_DEVICE_ID (CAN_BASE_ADDRESS + 0x00)
#define CAN_REG_SETTING_MODE (CAN_BASE_ADDRESS + 0x04)
#define CAN_BTR_ADDR      (CAN_BASE_ADDRESS + 0x24)
#define CAN_FD_ADDR       (CAN_BASE_ADDRESS + 0x28) 
#define CAN_TX_COMMAND_ADDR      (CAN_BASE_ADDRESS + 0x74) 
#define CAN_TXT_BUFFER_1_BASE      (CAN_BASE_ADDRESS + 0x100) 
#define CAN_REG_YOLO          (CAN_BASE_ADDRESS +0x90)

void can_init(void);
void can_tx(void);
void can_test(void);

void wait_ms(volatile unsigned int count) {
    while (count--) {
        __asm__ volatile ("nop");
    }
}

void uart_setup(void){
    uint32_t rtc_freq = *reg32(&__base_regs, CHESHIRE_RTC_FREQ_REG_OFFSET);
	uint64_t reset_freq = clint_get_core_freq(rtc_freq, 2500);
	uart_init(&__base_uart, reset_freq, __BOOT_BAUDRATE);
	char init[] = "Can Bus Test\r\n";
	uart_write_str(&__base_uart, init, sizeof(init) - 1);
    uart_write_flush(&__base_uart);
}

void can_test(void) {
    printf("CAN Version  = 0x%x (expected 0x0204CAFD)\n",MEM(CAN_REG_DEVICE_ID));
    printf("CAN_REG_YOLO = 0x%x (expected 0xDEADBEEF)\n",MEM(CAN_REG_YOLO));
    can_init();
    can_tx();
}

void can_init(void) {
    unsigned int btr = 0;
    printf("CAN Reset \n");
    MEM(CAN_REG_SETTING_MODE) = 1;
    wait_ms(100);

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

    MEM(CAN_BTR_ADDR) = btr;
    printf("CAN Enable \n");
    MEM(CAN_REG_SETTING_MODE) = 1 << 22;

}

void can_tx(void) {
    unsigned int frame_format_word = 0;
    unsigned int pattern = 0xAABBCCDD;
    unsigned int pattern1 = 0x12345678;
    frame_format_word |= 8; //DLC

    //normal frame
    frame_format_word |= (0 << 7);  // NO CAN FD Frame
    frame_format_word |= (0 << 9);  // NO CAN switch bitrate 
    printf("CAN fill buffer 1 \n");
    MEM(CAN_TXT_BUFFER_1_BASE) = frame_format_word;
  
    unsigned int id_word = (55 << 18); //id : 55 ()0x37 

    MEM(CAN_TXT_BUFFER_1_BASE + 0x04) = id_word;  // identifier base (no extended)
    MEM(CAN_TXT_BUFFER_1_BASE + 0x08) = 0;
    MEM(CAN_TXT_BUFFER_1_BASE + 0x0C) = 0;
    MEM(CAN_TXT_BUFFER_1_BASE + 0x10) = pattern;
    MEM(CAN_TXT_BUFFER_1_BASE + 0x14) = pattern1;
 
    unsigned int command = 0;
    command |= 0x2;   //ready command
    command |= (1<<8);  //buffer 1  

    printf("CAN command Tx \n");
    MEM(CAN_TX_COMMAND_ADDR) = command;  
}

int main(void) {
    uart_setup();
    can_test();
    return 0;
}