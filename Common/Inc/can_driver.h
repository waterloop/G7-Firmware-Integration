#pragma once
#include <stdint.h>
#include "can.h"

#define MAX_BYTES 8

typedef struct {
    uint32_t id;
    uint8_t start;
    uint8_t end;
} Data_Segment_t;

typedef struct {
    CAN_HandleTypeDef* hcan;    // CAN handler (hcan1, hcan2 or hcan3)
    uint32_t id_type;           // CAN_ID_STD or CAN_ID_EXT
    uint32_t rtr;               // CAN_RTR_DATA or CAN_RTR_REMOTE
    uint32_t data_length;       // 0 - MAX_BYTES
    uint32_t time_stamp;        // time stamp returned in rx header
    uint32_t id;                // message ID
    uint8_t data[MAX_BYTES];    // message data
} CAN_Frame_t;

CAN_Frame_t CAN_frame_init(CAN_HandleTypeDef* handler, uint32_t id);
void CAN_send_frame(CAN_Frame_t self);
CAN_Frame_t CAN_get_frame(CAN_HandleTypeDef* handler, uint32_t fifo_number);
uint32_t CAN_get_segment(CAN_Frame_t self, Data_Segment_t segment);
uint8_t CAN_set_segment(CAN_Frame_t* self, Data_Segment_t segment, uint32_t bytes);
