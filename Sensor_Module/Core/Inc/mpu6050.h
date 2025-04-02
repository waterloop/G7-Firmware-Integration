#pragma once
#include "i2c.h"

#define MPU6050_ADDR        0x68 << 1   // MPU address

#define ACCEL_XOUT_H            0x3B    // Accelerometer X-axis high byte
#define ACCEL_XOUT_L            0x3C    // Accelerometer X-axis low byte
#define ACCEL_YOUT_H            0x3D    // Accelerometer Y-axis high byte
#define ACCEL_YOUT_L            0x3E    // Accelerometer Y-axis low byte
#define ACCEL_ZOUT_H            0x3F    // Accelerometer Z-axis high byte
#define ACCEL_ZOUT_L            0x40    // Accelerometer Z-axis low byte


#define GYRO_XOUT_H             0x43    // Gyroscope X-axis high byte
#define GYRO_XOUT_L             0x44    // Gyroscope X-axis low byte
#define GYRO_YOUT_H             0x45    // Gyroscope Y-axis high byte
#define GYRO_YOUT_L             0x46    // Gyroscope Y-axis low byte
#define GYRO_ZOUT_H             0x47    // Gyroscope Z-axis high byte
#define GYRO_ZOUT_L             0x48    // Gyroscope Z-axis low byte

#define WHO_AM_I_REG        0x75
#define PWR_MGMT_1_REG      0x6B
#define SMPLRT_DIV_REG      0x19
#define ACCEL_CONFIG_REG    0x1C
#define GYRO_CONFIG_REG     0x1B

#define MPU6050_TIMEOUT 100

#define LSB_TO_G            1 / 2048
#define	G_TO_MS2            9.80665

#define LSB_TO_RAD_S        1 / 131

#define TIMEOUT             100

void MPU6050_Init(I2C_HandleTypeDef *hi2c);
void MPU6050_Read_Accel(int16_t* x_accel, int16_t* y_accel);
void MPU6050_Read_Gyro(int8_t* x_gyro, int8_t* y_gyro, int8_t* z_gyro);
