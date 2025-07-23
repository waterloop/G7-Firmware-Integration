<<<<<<< HEAD
#include "mpu6050.h"

static I2C_HandleTypeDef i2c_handler;

void MPU6050_Init (I2C_HandleTypeDef hi2c){
    i2c_handler = hi2c;

    uint8_t data = 0b00000000; // moves out of sleep mode
    HAL_I2C_Mem_Write(&i2c_handler, MPU6050_ADDR, PWR_MGMT_1_REG, 1, &data, 1, TIMEOUT);

    data = 0b00011000; // sets accelerometer range to +/- 16g
    HAL_I2C_Mem_Write(&i2c_handler, MPU6050_ADDR, ACCEL_CONFIG_REG, 1, &data, 1, TIMEOUT);

    data = 0b00000000; // sets gyroscope range to +/- 250deg/s
    HAL_I2C_Mem_Write(&i2c_handler, MPU6050_ADDR, GYRO_CONFIG_REG, 1, &data, 1, TIMEOUT);

    data = 0b00000111; // sets polling rate to 1kHZ
    HAL_I2C_Mem_Write(&i2c_handler, MPU6050_ADDR, SMPLRT_DIV_REG, 1, &data, 1, TIMEOUT);
}

void MPU6050_Read_Accel(int16_t* x_accel, int16_t* y_accel) {
    uint8_t rec_data[4] = {0}; // rec_data = {x_high, x_low, y_high, y_low}
    HAL_I2C_Mem_Read(&i2c_handler, MPU6050_ADDR, ACCEL_XOUT_H_REG, 1, rec_data, 4, TIMEOUT);

    int16_t accel_x_raw = (int16_t)(rec_data[0] << 8 | rec_data[1]);
    *x_accel = accel_x_raw * LSB_TO_G * G_TO_MS2;

    int16_t accel_y_raw = (int16_t)(rec_data[2] << 8 | rec_data[3]);
    *y_accel = accel_y_raw * LSB_TO_G * G_TO_MS2;
}

void MPU6050_Read_Gyro(int8_t* x_gyro, int8_t* y_gyro, int8_t* z_gyro){
	uint8_t rec_data[6] = {0}; // rec_data = {x_high, x_low, y_high, y_low, z_high, z_low}
	HAL_I2C_Mem_Read(&i2c_handler, MPU6050_ADDR, GYRO_XOUT_H_REG, 1, rec_data, 6, TIMEOUT);

	int16_t gyro_x_raw = (int16_t)(rec_data[0] << 8 | rec_data[1]);
	*x_gyro = (int16_t)(gyro_x_raw * LSB_TO_RAD_S); // the value never goes above 256

	int16_t gyro_y_raw = (int16_t)(rec_data[2] << 8 | rec_data[3]);
	*y_gyro = (int16_t)(gyro_y_raw * LSB_TO_RAD_S);

	int16_t gyro_z_raw = (int16_t)(rec_data[4] << 8 | rec_data[5]);
	*z_gyro = (int16_t)(gyro_z_raw * LSB_TO_RAD_S);
}
=======
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


>>>>>>> 952a9c6 (Sensor Recovered)
