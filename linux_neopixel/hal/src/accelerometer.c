// This file is used to read the accelerometer data
// from the I2C bus and return the data
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>

#include "hal/accelerometer.h"

// Device bus & address
#define I2CDRV_LINUX_BUS "/dev/i2c-1"
#define ACCEL_ADDR 0x19

// Accelerometer registers
#define ACCEL_CTRL_REG1 0x20
#define ACCEL_OUT_X_L 0x28
#define ACCEL_OUT_X_H 0x29
#define ACCEL_OUT_Y_L 0x2A
#define ACCEL_OUT_Y_H 0x2B
#define ACCEL_OUT_Z_L 0x2C
#define ACCEL_OUT_Z_H 0x2D

// Thread control
static pthread_t accel_thread;
static volatile bool keep_running = false;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static int i2c_file_desc = -1;
static bool is_initialized = false;

static int16_t current_x = 0;
static int16_t current_y = 0;
static int16_t current_z = 0;

static int init_i2c_bus(char *bus, int address);
static bool write_register(uint8_t reg, uint8_t value);
static uint8_t read_register(uint8_t reg);
static void *accelerometer_thread_function(void *arg);

// Initialize I2C bus
static int init_i2c_bus(char *bus, int address)
{
    int file_desc = open(bus, O_RDWR);
    if (file_desc == -1)
    {
        printf("I2C DRV: Unable to open bus for read/write (%s)\n", bus);
        perror("Error is:");
        return -1;
    }

    if (ioctl(file_desc, I2C_SLAVE, address) == -1)
    {
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
    if (write(i2c_file_desc, buf, 2) != 2)
    {
        printf("I2C DRV: Failed to write to register 0x%02X\n", reg);
        return false;
    }
    return true;
}

// Read a single byte from a register
static uint8_t read_register(uint8_t reg)
{
    uint8_t value;
    if (write(i2c_file_desc, &reg, 1) != 1)
    {
        printf("I2C DRV: Failed to write register address\n");
        return 0;
    }
    if (read(i2c_file_desc, &value, 1) != 1)
    {
        printf("I2C DRV: Failed to read from register 0x%02X\n", reg);
        return 0;
    }
    return value;
}

// Thread function to read the accelerometer data
static void *accelerometer_thread_function(void *arg)
{
    (void)arg;

    while (keep_running)
    {

        int16_t x, y, z;

        uint8_t x_l = read_register(ACCEL_OUT_X_L);
        uint8_t x_h = read_register(ACCEL_OUT_X_H);
        x = (int16_t)((x_h << 8) | x_l);

        uint8_t y_l = read_register(ACCEL_OUT_Y_L);
        uint8_t y_h = read_register(ACCEL_OUT_Y_H);
        y = (int16_t)((y_h << 8) | y_l);

        uint8_t z_l = read_register(ACCEL_OUT_Z_L);
        uint8_t z_h = read_register(ACCEL_OUT_Z_H);
        z = (int16_t)((z_h << 8) | z_l);

        pthread_mutex_lock(&mutex);
        current_x = x;
        current_y = y;
        current_z = z;
        pthread_mutex_unlock(&mutex);

        usleep(10000);
    }

    return NULL;
}

// Initialize the accelerometer
bool Accelerometer_init(void)
{
    if (is_initialized)
    {
        return true;
    }

    i2c_file_desc = init_i2c_bus(I2CDRV_LINUX_BUS, ACCEL_ADDR);
    if (i2c_file_desc == -1)
    {
        return false;
    }

    if (!write_register(ACCEL_CTRL_REG1, 0x47))
    {
        return false;
    }

    keep_running = true;
    if (pthread_create(&accel_thread, NULL, accelerometer_thread_function, NULL) != 0)
    {
        printf("ERROR: Failed to create accelerometer thread\n");
        keep_running = false;
        return false;
    }

    is_initialized = true;
    return true;
}

void Accelerometer_cleanup(void)
{
    if (!is_initialized)
    {
        return;
    }

    // Stop the thread
    keep_running = false;
    pthread_join(accel_thread, NULL);

    if (i2c_file_desc != -1)
    {
        close(i2c_file_desc);
        i2c_file_desc = -1;
    }

    is_initialized = false;
}

// Read the raw accelerometer data
bool Accelerometer_readRaw(int16_t *x, int16_t *y, int16_t *z)
{
    if (!is_initialized || i2c_file_desc == -1)
    {
        return false;
    }

    // Get the current values with mutex protection
    pthread_mutex_lock(&mutex);
    *x = current_x;
    *y = current_y;
    *z = current_z;
    pthread_mutex_unlock(&mutex);

    return true;
}
