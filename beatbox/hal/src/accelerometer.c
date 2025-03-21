#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <stdbool.h>

#include "hal/accelerometer.h"

// Device bus & address
#define I2CDRV_LINUX_BUS "/dev/i2c-1"
#define ACCEL_ADDR 0x19  // Accelerometer address on I2C bus

// Accelerometer registers
#define ACCEL_CTRL_REG1 0x20
#define ACCEL_OUT_X_L 0x28
#define ACCEL_OUT_X_H 0x29
#define ACCEL_OUT_Y_L 0x2A
#define ACCEL_OUT_Y_H 0x2B
#define ACCEL_OUT_Z_L 0x2C
#define ACCEL_OUT_Z_H 0x2D

// Static variables
static int i2c_file_desc = -1;

// Function declarations
static int init_i2c_bus(char* bus, int address);
static bool write_register(uint8_t reg, uint8_t value);
static uint8_t read_register(uint8_t reg);

// Initialize I2C bus
static int init_i2c_bus(char* bus, int address)
{
    int file_desc = open(bus, O_RDWR);
    if (file_desc == -1) {
        printf("I2C DRV: Unable to open bus for read/write (%s)\n", bus);
        perror("Error is:");
        return -1;
    }
    
    if (ioctl(file_desc, I2C_SLAVE, address) == -1) {
        printf("I2C DRV: Unable to set I2C device to slave address.\n");
        perror("Error is:");
        return -1;
    }
    
    return file_desc;
}

// Write a single byte to a register
static bool write_register(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    if (write(i2c_file_desc, buf, 2) != 2) {
        printf("I2C DRV: Failed to write to register 0x%02X\n", reg);
        return false;
    }
    return true;
}

// Read a single byte from a register
static uint8_t read_register(uint8_t reg)
{
    uint8_t value;
    if (write(i2c_file_desc, &reg, 1) != 1) {
        printf("I2C DRV: Failed to write register address\n");
        return 0;
    }
    if (read(i2c_file_desc, &value, 1) != 1) {
        printf("I2C DRV: Failed to read from register 0x%02X\n", reg);
        return 0;
    }
    return value;
}

bool Accelerometer_init(void)
{
    // Initialize I2C bus
    i2c_file_desc = init_i2c_bus(I2CDRV_LINUX_BUS, ACCEL_ADDR);
    if (i2c_file_desc == -1) {
        return false;
    }

    // Configure accelerometer
    if (!write_register(ACCEL_CTRL_REG1, 0x47)) {  // Normal mode, all axes enabled
        return false;
    }

    return true;
}

void Accelerometer_cleanup(void)
{
    if (i2c_file_desc != -1) {
        close(i2c_file_desc);
        i2c_file_desc = -1;
    }
}

bool Accelerometer_readRaw(int16_t* x, int16_t* y, int16_t* z)
{
    if (i2c_file_desc == -1) {
        return false;
    }

    // Read X axis
    uint8_t x_l = read_register(ACCEL_OUT_X_L);
    uint8_t x_h = read_register(ACCEL_OUT_X_H);
    *x = (int16_t)((x_h << 8) | x_l);

    // Read Y axis
    uint8_t y_l = read_register(ACCEL_OUT_Y_L);
    uint8_t y_h = read_register(ACCEL_OUT_Y_H);
    *y = (int16_t)((y_h << 8) | y_l);

    // Read Z axis
    uint8_t z_l = read_register(ACCEL_OUT_Z_L);
    uint8_t z_h = read_register(ACCEL_OUT_Z_H);
    *z = (int16_t)((z_h << 8) | z_l);

    return true;
} 