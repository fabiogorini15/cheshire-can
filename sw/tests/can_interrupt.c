//Thales CAN FD test application

#include "regs/cheshire.h"
#include "dif/clint.h"
#include "dif/uart.h"
#include "params.h"
#include "util.h"
#include <stdio.h>
#include "printf.h"

#define MEM(addr) (*(volatile uint32_t *)(addr))
#define BSWAP32(x) (((x)<<24) | (((x)&0xFF00)<<8) | (((x)>>8)&0xFF00) | ((x)>>24))

#define CAN_BASE_ADDRESS        0x0300A000
#define CAN_REG_DEVICE_ID       (CAN_BASE_ADDRESS + 0x000)
#define CAN_REG_SETTING_MODE    (CAN_BASE_ADDRESS + 0x004)
#define CAN_INT_STAT            (CAN_BASE_ADDRESS + 0x010)
#define CAN_INT_ENA_SET         (CAN_BASE_ADDRESS + 0x014)
#define CAN_INT_ENA_CLR         (CAN_BASE_ADDRESS + 0x018)
#define CAN_INT_MASK_SET        (CAN_BASE_ADDRESS + 0x01C)
#define CAN_INT_MASK_CLR        (CAN_BASE_ADDRESS + 0x020)
#define CAN_BTR_ADDR            (CAN_BASE_ADDRESS + 0x024)
#define CAN_FD_ADDR             (CAN_BASE_ADDRESS + 0x028) 
#define CAN_TX_COMMAND_ADDR     (CAN_BASE_ADDRESS + 0x074) 
#define CAN_TXT_BUFFER_1_BASE   (CAN_BASE_ADDRESS + 0x100) 
#define CAN_REG_YOLO            (CAN_BASE_ADDRESS + 0x090)
#define FAULT_STATE             (CAN_BASE_ADDRESS + 0X02E)
#define TX_STATUS               (CAN_BASE_ADDRESS + 0x070)
#define RETR_CTR                (CAN_BASE_ADDRESS + 0x07D)
#define DEBUG_REGISTER          (CAN_BASE_ADDRESS + 0x08C)   
#define RX_DATA_ADDR            (CAN_BASE_ADDRESS + 0x06C)
#define RX_STATUS_ADDR          (CAN_BASE_ADDRESS + 0x068)
#define COMMAND_ADDR            (CAN_BASE_ADDRESS + 0x00C)
#define RX_MEM_INFO_ADDR        (CAN_BASE_ADDRESS + 0x060) 
#define CAN_STATUS_ADDR         (CAN_BASE_ADDRESS + 0x008)


const char* tx_states[] = {
    "TXT_NOT_EXIST",  // 0
    "TXT_READY",        // 1
    "TXT_TRAN",       // 2
    "TXT_ABTP",       // 3
    "TXT_TOK",        // 4
    "UNUSED",         // 5
    "TXT_ERR",        // 6
    "TXT_ABT",        // 7
    "TXT_ETY",        // 8
    "TXT_PER",        // 9
};
const char* rx_states[] = {
    "UNUSED",  // 0
    "RXE",        // 1
    "RXF",       // 2
    "UNUSED",       // 3
    "RXMOF",        // 4
};
const char* fault_states[] = {
    "UNUSED",  // 0
    "ERA",     // 1
    "ERP",     // 2
    "UNUSED",  // 3
    "BOF",     // 4
};

void wait_ms(volatile unsigned int count);
void uart_setup(void);
void can_test(void);
void can_reset(void);
void can_enable(void);
void can_btr(void);
void can_tx(const uint32_t *payload_w32, uint8_t dlc);
void can_rx(void);


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
    can_reset();
    can_btr();
    can_enable();
    while(1) {
        can_rx();
    }
    // for(int i=0; i<10; i++){
    //     wait_ms(100000);
    //     can_tx();
    // }
    
}

void can_reset(void) {
    printf("CAN Reset \n");
    MEM(CAN_REG_SETTING_MODE) = 1;
    printf("Can CAN REG SETTING MODE during RESET : 0x%08X\r\n", MEM(CAN_REG_SETTING_MODE));
    wait_ms(100);
    printf("Can CAN REG SETTING MODE after RESET : 0x%08X\r\n", MEM(CAN_REG_SETTING_MODE));
}

void can_btr(void){
    unsigned int btr = 0;

    // clock 50 MHz
    // BTS 250K
    // printf("CAN Init @250K with clock 50MHz \n");
    // btr = (4<<19);    //time quanta
    // btr |= 29;        // prop
    // btr |= (10<<7);   // phase 1
    // btr |= (10<<13);  // phase 2
    // btr |= (3<<27);   // SJW
    // (29+10+10+1)*4 = 200 * 20ns = 4us = 250 Kbit

    // clock 50 MHz
    // BTS 500K
    printf("CAN Init @500K with clock 50MHz \n");
    btr = (2<<19);    //time quanta
    btr |= 29;        // prop
    btr |= (10<<7);   // phase 1
    btr |= (10<<13);  // phase 2
    btr |= (3<<27);   // SJW
    // (29+10+10+1)*2 = 100 * 20ns = 2us = 500 Kbit

    // clock 50 MHz
    // BTS 1M
    // printf("CAN Init @1M with clock 50MHz \n");
    // btr = (1<<19);    //time quanta
    // btr |= 29;        // prop
    // btr |= (10<<7);   // phase 1
    // btr |= (10<<13);  // phase 2
    // btr |= (3<<27);   // SJW
    // (29+10+10+1)*1 = 50 * 20ns = 1us = 1 Mbit
    

    MEM(CAN_BTR_ADDR) = btr;
    printf("Can CAN BTR (expected 0x1821451D): 0x%08X\r\n", MEM(CAN_BTR_ADDR));

    //MEM(CAN_REG_SETTING_MODE) |= 1 << 21; // enable ILBP
}

void can_enable(void){
    printf("Check CAN FD Fault State before enabling: %s\r\n",fault_states[MEM(FAULT_STATE) & 0x07]);

    printf("CAN Enable \n");
    MEM(CAN_REG_SETTING_MODE) |= 1 << 22;

    do {
        printf("Check CAN FD Fault State: %s\r\n", fault_states[MEM(FAULT_STATE) & 0x07]);
    } while ((MEM(FAULT_STATE) & 0x0007) != 0x0001);
    printf("Integration DONE (FAULT_STATE expected = ERA): %s\r\n", fault_states[MEM(FAULT_STATE) & 0x07]);
    printf("Can CAN REG SETTING MODE after ENABLE (expected 0x02400210): 0x%08X\r\n", MEM(CAN_REG_SETTING_MODE));
}

void can_tx(const uint32_t *payload_w32, uint8_t dlc) {
    unsigned int frame_format_word = 0;
    unsigned int pattern = BSWAP32(0xAABBCCDD);
    unsigned int pattern1 = BSWAP32(0x12345678);
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

    uint8_t tx_status = MEM(TX_STATUS) & 0X0F;
    printf("CAN TX STATUS register before TX COMMAND: %s\r\n", tx_states[tx_status]);


    printf("CAN command Tx \n");
    MEM(CAN_TX_COMMAND_ADDR) = command;
    
    tx_status = MEM(TX_STATUS) & 0X0F;
    printf("CAN TX STATUS register after TX COMMAND: %s\r\n", tx_states[tx_status]);

    uint8_t fault_state_check = MEM(FAULT_STATE) & 0x07;
    printf("CAN FAULT STATE register: %s\r\n", fault_states[fault_state_check]);

    do{
        printf("Polling on TX STATUS REGISTER: %s\r\n", tx_states[MEM(TX_STATUS) & 0x0F]);
    }while(((MEM(TX_STATUS) & 0x0F) == 0x01));
    printf("CAN TX STATUS register: %s\r\n", tx_states[MEM(TX_STATUS) & 0x0F]);

}
    
void can_rx(void) {
    /* Frame reception in automatic mode */

    printf("CAN Polling RX Buffer \n");
    uint32_t status;
    printf("Can STATUS register before polling: 0x%01X\r\n",MEM(CAN_STATUS_ADDR) & 0x01);
    do {
    status = MEM(CAN_STATUS_ADDR);
    } while ((status & 0x01) == 0);
    printf("Can STATUS register after polling: 0x%01X\r\n",MEM(CAN_STATUS_ADDR) & 0x01);

    /* Read frame from RX buffer */
    uint8_t data[64];
    uint32_t tmp;
    uint32_t ffw = MEM(RX_DATA_ADDR);
    uint32_t id = MEM(RX_DATA_ADDR);
    uint32_t ts_l = MEM(RX_DATA_ADDR);
    uint32_t ts_h = MEM(RX_DATA_ADDR);
    uint32_t rwcnt = (ffw >> 11) & 0x1F;
    for(uint32_t i = 0; i < rwcnt; i++){
        tmp = MEM(RX_DATA_ADDR);
        data[i*4] = tmp & 0xFF;
        data[i*4+1] = (tmp >> 8) & 0xFF;
        data[i*4+2] = (tmp >> 16) & 0xFF;
        data[i*4+3] = (tmp >> 24) & 0xFF;
    }
    if(!(ffw & (0 << 6))) id = (id >> 18) & 0x7FF; // standard IDE

    printf("FFW: 0x%08X\r\n", ffw);
    for(uint32_t i=0; i<rwcnt*2; i++){
        if (i == 0) printf("IDENTIFIER: 0X%08X\r\n", id);
        else if (i == 1) printf("TIMESTAMP: 0X%08X%08X\r\n", ts_h, ts_l);
        else printf("%02X", data[i-2]);
    }
    printf("\n");
}

int main(void) {
    uart_setup();
    can_test();
    return 0;
}