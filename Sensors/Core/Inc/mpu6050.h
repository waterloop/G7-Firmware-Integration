#pragma once
#include "i2c.h"

#define MPU6050_ADDR        0x68 << 1

#define ACCEL_XOUT_H_REG    0x3B
#define ACCEL_XOUT_L_REG    0x3B
#define ACCEL_YOUT_H_REG    0x3D
#define ACCEL_YOUT_L_REG    0x3E

#define GYRO_XOUT_H_REG     0x43
#define GYRO_XOUT_L_REG     0x44
#define GYRO_YOUT_H_REG     0x45
#define GYRO_YOUT_L_REG     0x46
#define GYRO_ZOUT_H_REG     0x47
#define GYRO_ZOUT_L_REG     0x48

#define WHO_AM_I_REG        0x75
#define PWR_MGMT_1_REG      0x6B
#define SMPLRT_DIV_REG      0x19
#define ACCEL_CONFIG_REG    0x1C
#define GYRO_CONFIG_REG     0x1B

#define LSB_TO_G            1 / 2048
#define	G_TO_MS2            9.80665

#define LSB_TO_RAD_S        1 / 131

#define TIMEOUT             100

void MPU6050_Init(I2C_HandleTypeDef hi2c);
void MPU6050_Read_Accel(int16_t* x_accel, int16_t* y_accel);
void MPU6050_Read_Gyro(int8_t* x_gyro, int8_t* y_gyro, int8_t* z_gyro);
