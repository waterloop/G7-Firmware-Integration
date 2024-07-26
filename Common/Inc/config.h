#pragma once
#include "can_driver.h"

// GENERAL DEFINES
#define MAX_CAN_PAYLOAD_BTYES   8
#define BUFF_SIZE               32
#define WARN_OFFSET             0xFF

// MOTOR CONTROLLER
#define MOTOR_CONTROLLER_K1     0x10F8109A
#define MOTOR_CONTROLLER_K2     0x10F8108D
#define MOTOR_CONTROLLER        0xFF
#define MOTOR_CONTROLLER_WARN   MOTOR_CONTROLLER - WARN_OFFSET

// BMS
#define BMS_BOARD               0xFF
#define BMS_BOARD_WARN          BMS_TEMP - WARN_OFFSET

// SENSORS
#define SENSOR_BOARD_1          0xFF
#define SENSOR_BOARD_1_WARN     SENSOR_BOARD_1 - WARN_OFFSET
#define SENSOR_BOARD_2          0xFF
#define SENSOR_BOARD_2_WARN     SENSOR_BOARD_2 - WARN_OFFSET

// BEGIN KELLY DEFS
// FRAME 1
const Data_Segment_t DRIVING_DIRECTION_K       = {MOTOR_CONTROLLER_K1, 1, 1};
const Data_Segment_t MOTOR_SPEED_K             = {MOTOR_CONTROLLER_K1, 2, 3};
const Data_Segment_t MOTOR_ERROR_CODE_K        = {MOTOR_CONTROLLER_K1, 4, 4}; 
// FRAME 2
const Data_Segment_t BATTERY_VOLTAGE_K         = {MOTOR_CONTROLLER_K2, 1, 2};
const Data_Segment_t BATTERY_CURRENT_K         = {MOTOR_CONTROLLER_K2, 3, 4};
const Data_Segment_t MOTOR_TEMP_K              = {MOTOR_CONTROLLER_K2, 5, 6};
const Data_Segment_t MOTOR_CONTROLLER_TEMP_K   = {MOTOR_CONTROLLER_K2, 7, 8};
// END KELLY DEFS

// BEGIN MOTOR CONTROLLER DEFS
const Data_Segment_t BATTERY_VOLTAGE           = {MOTOR_CONTROLLER, 1, 2};
const Data_Segment_t BATTERY_CURRENT           = {MOTOR_CONTROLLER, 3, 4};
const Data_Segment_t MOTOR_SPEED               = {MOTOR_CONTROLLER, 5, 6};
const Data_Segment_t MOTOR_CONTROLLER_TEMP     = {MOTOR_CONTROLLER, 7, 7};
const Data_Segment_t DRIVING_DIRECTION         = {MOTOR_CONTROLLER, 8, 8};
const Data_Segment_t MOTOR_ERROR_CODE          = {MOTOR_CONTROLLER, 8, 8};
// END MOTOR CONTROLLER DEFS

// BEGIN BMS DEFS
const Data_Segment_t MUX1_TEMP                 = {BMS_BOARD, 1, 1};
const Data_Segment_t MUX2_TEMP                 = {BMS_BOARD, 2, 2};
const Data_Segment_t MUX3_TEMP                 = {BMS_BOARD, 3, 3};
const Data_Segment_t MUX4_TEMP                 = {BMS_BOARD, 4, 4};
const Data_Segment_t MUX5_TEMP                 = {BMS_BOARD, 5, 5};
const Data_Segment_t MUX6_TEMP                 = {BMS_BOARD, 6, 6};
const Data_Segment_t BMS_ERROR_CODE            = {BMS_BOARD, 8, 8};
// END BMS DEFS

// BEGIN SENSORS BOARD DEFS
//FIRST FRAME
const Data_Segment_t PRESSURE                  = {SENSOR_BOARD_1, 1, 2};
const Data_Segment_t LIM_ONE_TEMP              = {SENSOR_BOARD_1, 3, 4};
const Data_Segment_t LIM_TWO_TEMP              = {SENSOR_BOARD_1, 5, 6};
const Data_Segment_t SENSORS_ERROR_CODE_1      = {SENSOR_BOARD_1, 8, 8};
//SECOND (IMU) FRAME
const Data_Segment_t X_ACCEL                   = {SENSOR_BOARD_2, 1, 2}; 
const Data_Segment_t Y_ACCEL                   = {SENSOR_BOARD_2, 3, 4};
const Data_Segment_t X_GYRO                    = {SENSOR_BOARD_2, 5, 5};
const Data_Segment_t Y_GYRO                    = {SENSOR_BOARD_2, 6, 6};
const Data_Segment_t Z_GYRO                    = {SENSOR_BOARD_2, 7, 7};
const Data_Segment_t SENSORS_ERROR_CODE_2      = {SENSOR_BOARD_2, 8, 8};
// END SENSORS BOARD DEFS
