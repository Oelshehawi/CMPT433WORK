// This file is used to read the joystick data 
// from the I2C bus and return the data as a struct
#include "hal/joystick.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <math.h>     
#include "hal/gpio.h" // Add GPIO header
#include <gpiod.h>

// I2C configuration for Zen HAT joystick
#define I2CDRV_LINUX_BUS "/dev/i2c-1"
#define I2C_DEVICE_ADDRESS 0x48
#define REG_CONFIGURATION 0x01
#define REG_DATA 0x00

// Channel 0 for X-axis (up/down)
#define JOYSTICK_X_CHANNEL 0x83C2 

// GPIO configuration for joystick button
#define JOYSTICK_BUTTON_CHIP GPIO_CHIP_2
#define JOYSTICK_BUTTON_PIN 15 

// Thresholds for joystick 
#define UP_THRESHOLD 300    // Below this is UP
#define DOWN_THRESHOLD 1500 // Above this is DOWN

// Thread control variables
static pthread_t joystickThreadId;
static pthread_t buttonThreadId;
static volatile bool isJoystickRunning = false;
static volatile bool isButtonRunning = false;

// Joystick state variables
static pthread_mutex_t joystickMutex = PTHREAD_MUTEX_INITIALIZER;
static JoystickDirection currentDirection = JOYSTICK_NONE;
static bool buttonPressed = false;

// I2C and GPIO handles
static int i2c_file_desc = -1;
static struct GpioLine *button_gpio = NULL;

// Private function prototypes
static int init_i2c_bus(char *bus, int address);
static void write_i2c_reg16(int i2c_file_desc, uint8_t reg_addr, uint16_t value);
static uint16_t read_i2c_reg16(int i2c_file_desc, uint8_t reg_addr);
static void *joystickSamplingThread(void *arg);
static void *buttonSamplingThread(void *arg);

// Initialize I2C bus for joystick
static int init_i2c_bus(char *bus, int address)
{
    int i2c_file_desc = open(bus, O_RDWR);
    if (i2c_file_desc == -1)
    {
        printf("I2C: Unable to open bus for read/write (%s)\n", bus);
        perror("Error is:");
        return -1;
    }
    if (ioctl(i2c_file_desc, I2C_SLAVE, address) == -1)
    {
        perror("I2C: Unable to set device to slave address");
        close(i2c_file_desc);
        return -1;
    }
    return i2c_file_desc;
}

// Write to I2C register
static void write_i2c_reg16(int i2c_file_desc, uint8_t reg_addr, uint16_t value)
{
    int tx_size = 1 + sizeof(value);
    uint8_t buff[tx_size];
    buff[0] = reg_addr;
    buff[1] = (value & 0xFF);
    buff[2] = (value & 0xFF00) >> 8;

    if (write(i2c_file_desc, buff, tx_size) != tx_size)
    {
        perror("I2C write failure");
    }
}

// Read from I2C register
static uint16_t read_i2c_reg16(int i2c_file_desc, uint8_t reg_addr)
{
    if (write(i2c_file_desc, &reg_addr, 1) != 1)
    {
        perror("I2C register select failed");
        return 0;
    }
    uint16_t value = 0;
    int bytes_read = read(i2c_file_desc, &value, sizeof(value));
    if (bytes_read != sizeof(value))
    {
        perror("I2C read failed");
        return 0;
    }
    return value;
}

// Thread for continuously reading joystick position
static void *joystickSamplingThread(void *arg)
{
    (void)arg; 

    if (i2c_file_desc >= 0)
    {
        write_i2c_reg16(i2c_file_desc, REG_CONFIGURATION, JOYSTICK_X_CHANNEL);
    }

    JoystickDirection lastRawDirection = JOYSTICK_NONE;
    int stableCount = 0;

    while (isJoystickRunning)
    {
        if (i2c_file_desc < 0)
        {
            usleep(100000);
            continue;
        }

        uint16_t raw_value = read_i2c_reg16(i2c_file_desc, REG_DATA);

        uint16_t value = ((raw_value & 0xFF) << 8) | ((raw_value & 0xFF00) >> 8);
        value >>= 4;

        // Determine direction from value
        JoystickDirection rawDirection = JOYSTICK_NONE;
        if (value < UP_THRESHOLD)
        {
            rawDirection = JOYSTICK_UP;
        }
        else if (value > DOWN_THRESHOLD)
        {
            rawDirection = JOYSTICK_DOWN;
        }

        // Simple debouncing
        if (rawDirection == lastRawDirection)
        {
            stableCount++;
            if (stableCount >= 3)
            {
                pthread_mutex_lock(&joystickMutex);
                if (currentDirection != rawDirection)
                {
                    currentDirection = rawDirection;
                }
                pthread_mutex_unlock(&joystickMutex);
            }
        }
        else
        {
            stableCount = 0;
            lastRawDirection = rawDirection;
        }

        usleep(10000); 
    }

    return NULL;
}

// Thread for handling button presses
static void *buttonSamplingThread(void *arg)
{
    (void)arg; 

    while (isButtonRunning && button_gpio != NULL)
    {
        struct gpiod_line_bulk bulkEvents;
        int numEvents = Gpio_waitForLineChange(button_gpio, &bulkEvents);

        if (numEvents > 0)
        {
            // Process each event (should be just one for the button)
            for (int i = 0; i < numEvents; i++)
            {
                struct gpiod_line *line = gpiod_line_bulk_get_line(&bulkEvents, i);
                struct gpiod_line_event event;

                if (gpiod_line_event_read(line, &event) != -1)
                {
                    // FALLING_EDGE = button pressed (active low)
                    // RISING_EDGE = button released (active low)
                    if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE)
                    {
                        pthread_mutex_lock(&joystickMutex);
                        buttonPressed = true;
                        pthread_mutex_unlock(&joystickMutex);
                    }
                    else if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE)
                    {
                        pthread_mutex_lock(&joystickMutex);
                        buttonPressed = false;
                        pthread_mutex_unlock(&joystickMutex);
                    }
                }
            }
        }
    }

    return NULL;
}


void Joystick_init(void)
{
    // Initialize I2C for the joystick
    i2c_file_desc = init_i2c_bus(I2CDRV_LINUX_BUS, I2C_DEVICE_ADDRESS);
    if (i2c_file_desc < 0)
    {
        printf("ERROR: Joystick I2C initialization failed\n");
    }
    else
    {
        // Test I2C communication
        write_i2c_reg16(i2c_file_desc, REG_CONFIGURATION, JOYSTICK_X_CHANNEL);
        usleep(5000); // 5ms delay
        uint16_t test_value = read_i2c_reg16(i2c_file_desc, REG_DATA);
        uint16_t processed = ((test_value & 0xFF) << 8) | ((test_value & 0xFF00) >> 8);
        processed >>= 4;
    }

    // Initialize GPIO for button
    button_gpio = Gpio_openForEvents(JOYSTICK_BUTTON_CHIP, JOYSTICK_BUTTON_PIN);
    if (button_gpio == NULL)
    {
        printf("ERROR: Joystick button GPIO initialization failed\n");
    }
}

void Joystick_startSampling(void)
{
    // Start the joystick thread
    isJoystickRunning = true;
    if (pthread_create(&joystickThreadId, NULL, joystickSamplingThread, NULL) != 0)
    {
        printf("ERROR: Failed to create joystick sampling thread\n");
        isJoystickRunning = false;
    }

    // Start the button thread
    isButtonRunning = true;
    if (button_gpio != NULL)
    {
        if (pthread_create(&buttonThreadId, NULL, buttonSamplingThread, NULL) != 0)
        {
            printf("ERROR: Failed to create button sampling thread\n");
            isButtonRunning = false;
        }
    }
}

void Joystick_stopSampling(void)
{
    // Stop the joystick thread
    if (isJoystickRunning)
    {
        isJoystickRunning = false;
        pthread_join(joystickThreadId, NULL);
    }

    // Stop the button thread
    if (isButtonRunning)
    {
        isButtonRunning = false;
        pthread_join(buttonThreadId, NULL);
    }
}

void Joystick_cleanup(void)
{
    Joystick_stopSampling();

    if (i2c_file_desc != -1)
    {
        close(i2c_file_desc);
        i2c_file_desc = -1;
    }

    if (button_gpio != NULL)
    {
        Gpio_close(button_gpio);
        button_gpio = NULL;
    }
}

JoystickDirection Joystick_getDirection(void)
{
    pthread_mutex_lock(&joystickMutex);
    JoystickDirection direction = currentDirection;
    pthread_mutex_unlock(&joystickMutex);
    return direction;
}

bool Joystick_isPressed(void)
{
    pthread_mutex_lock(&joystickMutex);
    bool pressed = buttonPressed;
    pthread_mutex_unlock(&joystickMutex);
    return pressed;
}
