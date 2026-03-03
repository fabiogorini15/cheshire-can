/*
 * can.h
 *
 *  Created on: Feb 11, 2026
 *      Author: Fabio Gorini
 */

#ifndef INC_CAN_H_
#define INC_CAN_H_

#include "regs/cheshire.h"
#include <stdint.h>
#include "params.h"
#include "util.h"



#define HIGH 1
#define LOW 0

#define true 1
#define false 0

#define MEM(addr) (*(volatile uint32_t *)(addr))
#define BSWAP32(x) (((x)<<24) | (((x)&0xFF00)<<8) | (((x)>>8)&0xFF00) | ((x)>>24)) 
//__attribute__((packed, aligned(4)))
/*CTU_CAN_FD Regs define */
#define CAN_BASE_ADDRESS        0x0300A000
#define CAN_REG_DEVICE_ID       (CAN_BASE_ADDRESS + 0x000)
#define CAN_REG_SETTING_MODE    (CAN_BASE_ADDRESS + 0x004)
#define CAN_STATUS_ADDR         (CAN_BASE_ADDRESS + 0x008)
#define COMMAND_ADDR            (CAN_BASE_ADDRESS + 0x00C)
#define CAN_INT_STAT            (CAN_BASE_ADDRESS + 0x010)
#define CAN_INT_ENA_SET         (CAN_BASE_ADDRESS + 0x014)
#define CAN_INT_ENA_CLR         (CAN_BASE_ADDRESS + 0x018)
#define CAN_INT_MASK_SET        (CAN_BASE_ADDRESS + 0x01C)
#define CAN_INT_MASK_CLR        (CAN_BASE_ADDRESS + 0x020)
#define CAN_BTR_ADDR            (CAN_BASE_ADDRESS + 0x024)
#define CAN_BTR_FD_ADDR         (CAN_BASE_ADDRESS + 0x028)
#define FAULT_STATE             (CAN_BASE_ADDRESS + 0X02E)
#define RX_MEM_INFO_ADDR        (CAN_BASE_ADDRESS + 0x060) 
#define RX_STATUS_ADDR          (CAN_BASE_ADDRESS + 0x068)
#define RX_DATA_ADDR            (CAN_BASE_ADDRESS + 0x06C)
#define TX_STATUS               (CAN_BASE_ADDRESS + 0x070)
#define CAN_TX_COMMAND_ADDR     (CAN_BASE_ADDRESS + 0x074) 
#define RETR_CTR                (CAN_BASE_ADDRESS + 0x07D)
#define DEBUG_REGISTER          (CAN_BASE_ADDRESS + 0x08C)   
#define CAN_REG_YOLO            (CAN_BASE_ADDRESS + 0x090)
#define CAN_TXT_BUFFER_1_BASE   (CAN_BASE_ADDRESS + 0x100)


/*DLC to bytes lookup table */
/*Classical CAN: DLC 9-15 are capped at 8 bytes */
static const uint8_t dlc_to_bytes_classic[16] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 8, 8, 8, 8, 8, 8, 8
};
/*CAN FD: DLC 9-15 map to 12, 16, 20, 24, 32, 48, 64 bytes */
static const uint8_t dlc_to_bytes_fd[16] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64
};
/*Returns DLC code from number of bytes for CAN FD*/
static inline uint8_t bytes_to_dlc_fd(uint8_t n_bytes) {
    if (n_bytes <= 8)  return n_bytes;
    if (n_bytes <= 12) return 9;
    if (n_bytes <= 16) return 10;
    if (n_bytes <= 20) return 11;
    if (n_bytes <= 24) return 12;
    if (n_bytes <= 32) return 13;
    if (n_bytes <= 48) return 14;
    return 15;
}

/*Register define setup*/
typedef enum {
    TXT_NOT_EXIST = 0,  // 0
    TXT_READY,        // 1
    TXT_TRAN,       // 2
    TXT_ABTP,       // 3
    TXT_TOK,        // 4
    TXT_UNUSED,         // 5
    TXT_ERR,        // 6
    TXT_ABT,        // 7
    TXT_ETY,        // 8
    TXT_PER,        // 9
}tx_state_t;

typedef enum {
    FAULT_UNUSED = 0,   // 0
    ERA,            // 1
    ERP,            // 2
    FAULT_UNUSED_2, // 3
    BOF,            // 4
}fault_state_t;

/*Mode Settings register definitions */
///Mode Settings register bits
union Mode_Settings_CTU_CAN_FD {
    ///Access all bits
    uint32_t all;

    ///Access individual bits
    struct BitField_Mode_Settings_CTU_CAN_FD {
        uint32_t rst :1; 
        uint32_t bmm :1;
        uint32_t stm :1;
        uint32_t afm :1;
        uint32_t fde :1;
        uint32_t tttm :1;
        uint32_t rom :1;
        uint32_t acf :1;
        uint32_t tstm :1;
        uint32_t rxbam :1;
        uint32_t txbbm :1;
        uint32_t sam :1;
        uint32_t erfm :1;
        uint32_t reserved1 :3;
        uint32_t rtrle :1;
        uint32_t rtrth :4;
        uint32_t ilbp :1;
        uint32_t ena :1;
        uint32_t nisofd :1;
        uint32_t pex :1;
        uint32_t tbfbo :1;
        uint32_t fdrf :1;
        uint32_t pchke :1;
        uint32_t reserved2 :4;
    } bits;
};

/*Status register definitions */
///Status register bits
union Status_CTU_CAN_FD {
    ///Access all bits
    uint32_t all;

    ///Access individual bits
    struct BitField_Status_CTU_CAN_FD {
        uint32_t rxne :1; 
        uint32_t dor :1;
        uint32_t txnf :1;
        uint32_t eft :1;
        uint32_t rxs :1;
        uint32_t txs :1;
        uint32_t ewl :1;
        uint32_t idle :1;
        uint32_t pexs :1;
        uint32_t rxpe :1;
        uint32_t txpe :1;
        uint32_t txdpe :1;
        uint32_t reserved1 :4;
        uint32_t stcnt :1;
        uint32_t strgs :1;
        uint32_t sprt :4;
        uint32_t reserved2 :13;
    } bits;
};

/*Interrupt Status register definitions */
///Interrupt Status register bits
union Interrupt_Status_CTU_CAN_FD {
    ///Access all bits
    uint32_t all;

    ///Access individual bits
    struct BitField_Interrupt_Status_CTU_CAN_FD {
        uint32_t rxi :1; 
        uint32_t txi :1;
        uint32_t ewli :1;
        uint32_t doi :1;
        uint32_t fcsi :1;
        uint32_t ali :1;
        uint32_t bei :1;
        uint32_t ofi :1;
        uint32_t rxfi :1;
        uint32_t bsi:1;
        uint32_t rbnei :1;
        uint32_t txbhci :1;
        uint32_t reserved1 :4;
        uint32_t reserved2 :16;
    } bits;
};

/*Bitrate register definitions */
///Bitrate register bits
union Bitrate_CTU_CAN_FD {
    ///Access all bits
    uint32_t all;

    ///Access individual bits
    struct BitField_Bitrate_CTU_CAN_FD {
        uint32_t prop :7; 
        uint32_t ph1 :6;
        uint32_t ph2 :6;
        uint32_t brp :8;
        uint32_t sjw :5;
    } bits;
};

/*Bitrate_FD register definitions */
///Bitrate register bits
union Bitrate_FD_CTU_CAN_FD {
    ///Access all bits
    uint32_t all;

    ///Access individual bits
    struct BitField_Bitrate_FD_CTU_CAN_FD {
        uint32_t prop_fd :6;
        uint32_t reserved1 :1; 
        uint32_t ph1_fd :5;
        uint32_t reserved2 :1;
        uint32_t ph2_fd :5;
        uint32_t reserved3 :1;
        uint32_t brp_fd :8;
        uint32_t sjw_fd :5;
    } bits;
};

/*Fault State register definitions */
///Fault State register bits
union Fault_State_CTU_CAN_FD {
    ///Access all bits
    uint32_t all;

    ///Access individual bits
    struct BitField_Fault_State_CTU_CAN_FD {
        uint32_t era :1;
        uint32_t erp :1; 
        uint32_t bof :1;
        uint32_t reserved1 :13;
        uint32_t reserved2 :16;
    } bits;
};

/*TX Status register definitions */
///TX Status register bits
union TX_Status_CTU_CAN_FD {
    ///Access all bits
    uint32_t all;

    ///Access individual bits
    struct BitField_TX_Status_CTU_CAN_FD {
        uint32_t txs1 :4;
        uint32_t txs2 :4;
        uint32_t txs3 :4;
        uint32_t txs4 :4;
        uint32_t txs5 :4;
        uint32_t txs6 :4;
        uint32_t txs7 :4;
        uint32_t txs8 :4;
    } bits;
};

/*TX Command register definitions */
///TX Command register bits
union TX_Command_CTU_CAN_FD {
    ///Access all bits
    uint32_t all;

    ///Access individual bits
    struct BitField_TX_Command_CTU_CAN_FD {
        uint32_t txce :1;       // Issues "set empty" command
        uint32_t txcr :1;       // Issues "set ready" command
        uint32_t txca :1;       // Issues "set abort" command
        uint32_t reserved1 :5;
        uint32_t txb1 :1;
        uint32_t txb2 :1;
        uint32_t txb3 :1;
        uint32_t txb4 :1;
        uint32_t txb5 :1;
        uint32_t txb6 :1;
        uint32_t txb7 :1;
        uint32_t txb8 :1;
        uint32_t reserved2 :16;
    } bits;
};

/*Frame format register definitions */
///Frame format word register bits
union ffw_CTU_CAN_FD {
    ///Access all bits
    uint32_t all;

    ///Access individual bits
    struct BitField_ffw_CTU_CAN_FD {
        uint32_t dlc :4;
        uint32_t reserved1:1;
        uint32_t rtr :1;
        uint32_t ide :1;
        uint32_t fdf :1;
        uint32_t reserved2 :1;
        uint32_t brs :1;
        uint32_t esi_rsv :1;
        uint32_t rwcnt :5;
        uint32_t reserved3 :16;
    } bits;
};

///Identifier word register bits
union id_w_CTU_CAN_FD {
    ///Access all bits
    uint32_t all;

    ///Access individual bits
    struct BitField_id_w_CTU_CAN_FD {
        uint32_t identifier_ext :18;
        uint32_t identifier_base :11;
        uint32_t reserved1 :3;
    } bits;
};

/*CAN FD Frame register definitions */
///CAN FD Frame register bits
typedef struct {
    union ffw_CTU_CAN_FD ffw_rx;    
    union id_w_CTU_CAN_FD id_rx;
    uint32_t timestamp_l_w;
    uint32_t timestamp_h_w;
    uint32_t data_w[16];
    uint32_t frame_test;
} CTU_CAN_FD_rx_frame;

/*Util functions*/
const char* tx_state_to_str(tx_state_t s);
const char* fault_state_to_str(fault_state_t s);

/*Init Functions*/
void wait_ms(volatile unsigned int count);
void can_test(void);
void can_btr(void);
void can_enable(void);

/*Reset Functions*/
void can_reset(void);

/* Can TX RX Functions*/
void can_tx(const uint32_t *payload_w32, const union ffw_CTU_CAN_FD *ffw, const union id_w_CTU_CAN_FD *id_w);
void can_rx(CTU_CAN_FD_rx_frame *can_fd_rx_frame);


#endif /* INC_CAN_H_ */