/*
 * main.c
 *
 *  Created on: Feb 13, 2026
 *      Author: Fabio Gorini
 */

#include "regs/cheshire.h"
#include <stdio.h>
#include <stdint.h>
#include "dif/clint.h"
#include "dif/uart.h"
#include "dif/can.h"
#include "params.h"
#include "util.h"
#include "printf.h"

#define TX_WORDS 2

/* uart_setup
 @ description : Initializes the UART peripheral using the system's RTC
 frequency. The function reads the current RTC frequency,
 computes the core reset frequency, and configures the UART
 with the desired baud rate. After initialization, it sends
 a startup message ("Can Bus Test") and flushes the TX buffer
 to ensure all characters are transmitted.*/
void uart_setup(void){
    uint32_t rtc_freq = *reg32(&__base_regs, CHESHIRE_RTC_FREQ_REG_OFFSET);
    uint64_t reset_freq = clint_get_core_freq(rtc_freq, 2500);
    uart_init(&__base_uart, reset_freq, __BOOT_BAUDRATE);
    char init[] = "UART Setup\r\n";
    uart_write_str(&__base_uart, init, sizeof(init) - 1);
    uart_write_flush(&__base_uart);
}

/* can_print_frame
 @ description : prints the contents of a received CAN frame.*/
void can_print_frame(CTU_CAN_FD_rx_frame *frame) {
    printf("FFW             : 0x%08X\r\n", frame->ffw_rx.all);
    if (!frame->ffw_rx.bits.ide) printf("IDENTIFIER (STD): 0x%03X\r\n", frame->id_rx.bits.identifier_base);
    else printf("IDENTIFIER (EXT): 0x%08X\r\n", frame->id_rx.all);
    printf("TIMESTAMP       : 0x%08X%08X\r\n", frame->timestamp_h_w, frame->timestamp_l_w);
    printf("DATA (%d words) :\r\n", frame->ffw_rx.bits.rwcnt - 3U);
    for(uint32_t i = 0; i < frame->ffw_rx.bits.rwcnt - 3U; i++)
        printf("%08X ", frame->data_w[i]);
    printf("\r\n");
}

/* test_statistics
 @ description : Sends a trigger frame and receives echo frames from 3 STM32 nodes.
 Counts received and correct frames per node and prints statistics.*/
void test_statistics(void) {
    union ffw_CTU_CAN_FD tx_ffw;
    union id_w_CTU_CAN_FD tx_id;
    CTU_CAN_FD_rx_frame rx_frame;
    tx_ffw.all = 0;
    tx_id.all  = 0;

    uint32_t tx_payload[TX_WORDS] = { 0xDEADBEEF, 0xCAFEBABE };
    tx_ffw.bits.dlc = bytes_to_dlc_fd(sizeof(tx_payload));
    tx_ffw.bits.rtr = 0;
    tx_ffw.bits.ide = 0;
    tx_ffw.bits.fdf = 0;
    tx_ffw.bits.brs = 0;
    tx_ffw.bits.esi_rsv = 0;
    tx_id.bits.identifier_base = 55; // 0x37

    can_tx(tx_payload, &tx_ffw, &tx_id);

    int ok_336 = 0, ok_436 = 0, ok_536 = 0;
    int tot_336 = 0, tot_436 = 0, tot_536 = 0;
    uint32_t expected[TX_WORDS] = { 0xDEADBEEF, 0xCAFEBABE };

    for (int i = 0; i < 30; i++) // discard firsts 30 frames
        can_rx(&rx_frame);

    int empty = 0;
    while (empty < 11) {
        if (can_rx_nb(&rx_frame) == 0) {
            empty = 0;  // recieved frame
            int data_ok = (rx_frame.data_w[0] == expected[0]) &&
                        (rx_frame.data_w[1] == expected[1]);
            switch (rx_frame.id_rx.bits.identifier_base) {
                case 0x336: tot_336++; if (data_ok) ok_336++; break;
                case 0x436: tot_436++; if (data_ok) ok_436++; break;
                case 0x536: tot_536++; if (data_ok) ok_536++; break;
            }
        } else {  // empty buffer
            empty++;
            wait_ms(1000); //---50000 1 ms ---1000 20us
        }
    }
    float total = ok_336+ ok_436 + ok_536;
    float total_rate = total / (float)3000 * (float)100;
    printf("STM32 #1 (0x336): ricevuti %d/1000, corretti %d/1000\r\n", tot_336, ok_336);
    printf("STM32 #2 (0x436): ricevuti %d/1000, corretti %d/1000\r\n", tot_436, ok_436);
    printf("STM32 #3 (0x536): ricevuti %d/1000, corretti %d/1000\r\n", tot_536, ok_536);
    printf("FRAME DELIVERY RATIO: %0.3f %%\r\n", total_rate);
    }

int main(void) {
    can_bitrate_t btr, btr_fd;
    btr    = CAN_BITRATE_1M;
    btr_fd = CAN_BITRATE_1M;

    uart_setup();
    can_test();
    can_reset();
    can_btr(btr, btr_fd);
    can_enable();

    test_statistics();

    return 0;
}