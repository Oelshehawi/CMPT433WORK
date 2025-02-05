#include "hal/joystick.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <time.h>

#define I2CDRV_LINUX_BUS "/dev/i2c-1"
#define I2C_DEVICE_ADDRESS 0x48
#define REG_CONFIGURATION 0x01
#define REG_DATA 0x00
#define TLA2024_CHANNEL_CONF_0 0x83C2

static int i2c_file_desc = -1;

// Fixed calibration values based on measured data
static const uint16_t up_threshold = 865;     // Above this is UP movement
static const uint16_t down_threshold = 830;   // Below this is DOWN movement
static const uint16_t left_threshold = 100;   // Clear threshold for left
static const uint16_t right_threshold = 1500; // Clear threshold for right

static int init_i2c_bus(char *bus, int address)
{
    int i2c_file_desc = open(bus, O_RDWR);

    if (i2c_file_desc == -1)
    {
        printf("I2C DRV: Unable to open bus for read/write (%s)\n", bus);
        perror("Error is:");
        exit(EXIT_FAILURE);
    }

    if (ioctl(i2c_file_desc, I2C_SLAVE, address) == -1)
    {
        perror("Unable to set I2C device to slave address.");
        exit(EXIT_FAILURE);
    }
    return i2c_file_desc;
}

static void write_i2c_reg16(int i2c_file_desc, uint8_t reg_addr, uint16_t value)
{
    int tx_size = 1 + sizeof(value);
    uint8_t buff[tx_size];
    buff[0] = reg_addr;
    buff[1] = (value & 0xFF);
    buff[2] = (value & 0xFF00) >> 8;
    int bytes_written = write(i2c_file_desc, buff, tx_size);
    if (bytes_written != tx_size)
    {
        perror("Unable to write i2c register");
        exit(EXIT_FAILURE);
    }
}

static uint16_t read_i2c_reg16(int i2c_file_desc, uint8_t reg_addr)
{
    int bytes_written = write(i2c_file_desc, &reg_addr, sizeof(reg_addr));
    if (bytes_written != sizeof(reg_addr))
    {
        perror("Unable to write i2c register.");
        exit(EXIT_FAILURE);
    }

    uint16_t value = 0;
    int bytes_read = read(i2c_file_desc, &value, sizeof(value));
    if (bytes_read != sizeof(value))
    {
        perror("Unable to read i2c register");
        exit(EXIT_FAILURE);
    }
    return value;
}

void joystick_init(void)
{
    i2c_file_desc = init_i2c_bus(I2CDRV_LINUX_BUS, I2C_DEVICE_ADDRESS);
    if (i2c_file_desc == -1)
    {
        perror("Failed to open I2C bus");
        exit(EXIT_FAILURE);
    }

    write_i2c_reg16(i2c_file_desc, REG_CONFIGURATION, TLA2024_CHANNEL_CONF_0);
}

JoystickDirection read_joystick_direction(void)
{

    // Read and process joystick value
    uint16_t raw_value = read_i2c_reg16(i2c_file_desc, REG_DATA);
    uint16_t processed_value = ((raw_value & 0xFF) << 8) | ((raw_value & 0xFF00) >> 8);
    processed_value >>= 4;

    // Determine direction using fixed thresholds
    JoystickDirection dir;
    if (processed_value < left_threshold)
    {
        dir = JOYSTICK_LEFT;
    }
    else if (processed_value > right_threshold)
    {
        dir = JOYSTICK_RIGHT;
    }
    else if (processed_value > up_threshold) // Higher values mean UP
    {
        dir = JOYSTICK_UP;
    }
    else if (processed_value < down_threshold) // Lower values mean DOWN
    {
        dir = JOYSTICK_DOWN;
    }
    else 
    {
        dir = JOYSTICK_NONE;
    }

    return dir;
}

