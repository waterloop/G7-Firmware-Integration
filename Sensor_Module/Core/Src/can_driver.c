#include "can_driver.h"

#define WARN_OFFSET             0xFF

static uint32_t tx_mailbox;
#define WARN_OFFSET 0xFF

CAN_Frame_t CAN_frame_init(CAN_HandleTypeDef* handler, uint32_t id) {
    // default conditions
    CAN_Frame_t ret = {
        .hcan = handler,
        .id_type = CAN_ID_STD,
        .rtr = CAN_RTR_DATA,
        .data_length = MAX_BYTES,
        .time_stamp = 0,
        .id = id,
        .data = {0}
        };

    return ret;
}

void CAN_send_frame(CAN_Frame_t self) {
    CAN_TxHeaderTypeDef tx_header = {
        .IDE = self.id_type,
        .RTR = self.rtr,
        .DLC = self.data_length,
        .StdId = self.id,
        .ExtId = self.id,
        .TransmitGlobalTime = DISABLE
    };

    if (HAL_CAN_AddTxMessage(self.hcan, &tx_header, self.data, &tx_mailbox) != HAL_OK) {
        Error_Handler();
    }
}

CAN_Frame_t CAN_get_frame(CAN_HandleTypeDef* handler, uint32_t fifo_number) {
    CAN_Frame_t ret = CAN_frame_init(handler, 0xFFFFFFFF);

    CAN_RxHeaderTypeDef rx_header;
    if (HAL_CAN_GetRxMessage(handler, fifo_number, &rx_header, ret.data) != HAL_OK) {
        Error_Handler();
    }
    
    ret.id_type = rx_header.IDE;
    ret.rtr = rx_header.RTR;
    ret.data_length = rx_header.DLC;
    ret.time_stamp = rx_header.Timestamp;

    if (rx_header.IDE == CAN_ID_STD) {
        ret.id = rx_header.StdId;
    } else {
        ret.id = rx_header.ExtId;
    }
    
    return ret;
}

uint32_t CAN_get_segment(CAN_Frame_t self, Data_Segment_t segment) {
    // return if IDs don't match
    if ((self.id != segment.id) && ((self.id - WARN_OFFSET) != segment.id)) {
        return 0xFFFFFFFF;
    }

    uint32_t ret = 0;
    uint8_t length = (segment.end - segment.start) + 1; // segment numbering is 1-based indexing
    for(uint8_t i = 0; i < length; i++) {
        ret |= (self.data[(segment.start - 1) + i] << (8 * i)); // converting from little endian format
    }

    return ret;
}

uint8_t CAN_set_segment(CAN_Frame_t* self, Data_Segment_t segment, uint32_t data) {
    // return if IDs don't match
    if ((self->id != segment.id) && ((self->id << WARN_OFFSET) != segment.id)) {
        return 1;
    }

    uint8_t length = (segment.end - segment.start) + 1; // segment numbering is 1-based indexing
    for(uint8_t i = 0; i < length; i++) {
        self -> data[(segment.start - 1) + i] = ((data >> (8 * i)) & 0xFF); // converting to little endian format
    }
    
    return 0;
}
