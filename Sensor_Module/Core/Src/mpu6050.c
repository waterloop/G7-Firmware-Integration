#include "mpu6050.h"

void MPU6050_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t data;

    // Wake up the MPU6050 (clear sleep mode bit)
    data = 0x00;
    HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, PWR_MGMT_1_REG, I2C_MEMADD_SIZE_8BIT, &data, 1, MPU6050_TIMEOUT);

    // Set accelerometer range to ±16g
    data = 0x18;  // 0b00011000
    HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, ACCEL_CONFIG_REG, I2C_MEMADD_SIZE_8BIT, &data, 1, MPU6050_TIMEOUT);

    // Set gyroscope range to ±250°/s
    data = 0x00;  // 0b00000000
    HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, GYRO_CONFIG_REG, I2C_MEMADD_SIZE_8BIT, &data, 1, MPU6050_TIMEOUT);

    // Set sample rate to 1kHz / (1 + 7) = 125Hz
    data = 0x07;  // 0b00000111
    HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, SMPLRT_DIV_REG, I2C_MEMADD_SIZE_8BIT, &data, 1, MPU6050_TIMEOUT);
}


void MPU6050_Read(int16_t *ax, int16_t *ay, int16_t *az, int16_t *gx, int16_t *gy, int16_t *gz)
{
    uint8_t data[14];
    int16_t accelX, accelY, accelZ, gyroX, gyroY, gyroZ;

    // Read raw data from MPU6050
    HAL_I2C_Mem_Read(&hi2c2, MPU6050_ADDR, ACCEL_XOUT_H, I2C_MEMADD_SIZE_8BIT, data, 14, 100);

    // Convert raw data
    *ax = (int16_t)(data[0] << 8 | data[1]);
    *ay = (int16_t)(data[2] << 8 | data[3]);
    *az = (int16_t)(data[4] << 8 | data[5]);

    *gx  = (int16_t)(data[8]  << 8 | data[9]);
    *gy  = (int16_t)(data[10] << 8 | data[11]);
    *gz  = (int16_t)(data[12] << 8 | data[13]);

}


