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
#include "hal/gpio.h" // Add GPIO header

// I2C configuration for Zen HAT joystick
#define I2CDRV_LINUX_BUS "/dev/i2c-1"
#define I2C_DEVICE_ADDRESS 0x48
#define REG_CONFIGURATION 0x01
#define REG_DATA 0x00

// Channel configuration for TLA2024 ADC
#define JOYSTICK_X_CHANNEL 0x83C2 // Channel 0 for X-axis (up/down)

// GPIO configuration for joystick button
#define JOYSTICK_BUTTON_CHIP GPIO_CHIP_2
#define JOYSTICK_BUTTON_PIN 15 // GPIO15 on pin 15 from gpioget readings

// Thresholds for joystick directions
#define UP_THRESHOLD 500    // X axis - below this is UP
#define DOWN_THRESHOLD 1500 // X axis - above this is DOWN

// Debounce timeout in milliseconds
#define DEBOUNCE_TIMEOUT_MS 50 // Reduced for immediate response

// Joystick state information
static pthread_t joystickThreadId;
static pthread_mutex_t joystickMutex = PTHREAD_MUTEX_INITIALIZER;
static bool isRunning = false;
static int i2c_file_desc = -1;
static struct GpioLine *button_gpio = NULL; // GPIO line for button

// Current debounced joystick state
static JoystickDirection currentDirection = JOYSTICK_NONE;
static bool buttonPressed = false;

// Debounce timer
static struct timespec lastDirectionChange;
static struct timespec lastButtonChange;

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
    // To read a register, must first write the address
    if (write(i2c_file_desc, &reg_addr, 1) != 1)
    {
        perror("I2C register select failed");
        return 0;
    }
    // Now read the value and return it
    uint16_t value = 0;
    int bytes_read = read(i2c_file_desc, &value, sizeof(value));
    if (bytes_read != sizeof(value))
    {
        perror("I2C read failed");
        return 0;
    }
    return value;
}

// Read joystick axis value
static uint16_t read_joystick_axis(uint16_t channel_config)
{
    if (i2c_file_desc < 0)
        return 0;

    write_i2c_reg16(i2c_file_desc, REG_CONFIGURATION, channel_config);
    uint16_t raw_value = read_i2c_reg16(i2c_file_desc, REG_DATA);

    // Process raw value - swap bytes and shift right 4 bits
    uint16_t processed_value = ((raw_value & 0xFF) << 8) | ((raw_value & 0xFF00) >> 8);
    return processed_value >> 4;
}

// Get time difference in milliseconds
static long get_time_diff_ms(struct timespec *start, struct timespec *end)
{
    return (end->tv_sec - start->tv_sec) * 1000 + (end->tv_nsec - start->tv_nsec) / 1000000;
}

// Get current time
static void get_current_time(struct timespec *time)
{
    clock_gettime(CLOCK_MONOTONIC, time);
}

// Raw reading of joystick direction using only X axis for volume
static JoystickDirection read_raw_joystick_direction(void)
{
    uint16_t x_value = read_joystick_axis(JOYSTICK_X_CHANNEL);

    // For debugging - uncomment if needed to see actual values
    // printf("Joystick X value: %d\n", x_value);

    if (x_value < UP_THRESHOLD)
    {
        return JOYSTICK_UP;
    }
    else if (x_value > DOWN_THRESHOLD)
    {
        return JOYSTICK_DOWN;
    }

    return JOYSTICK_NONE;
}

// Check if the joystick button is pressed (using GPIO)
static bool read_raw_button_press(void)
{
    if (button_gpio == NULL)
    {
        // Debug printing only if null
        static int null_counter = 0;
        if (++null_counter % 100 == 0)
        {
            printf("Joystick button GPIO not initialized\n");
        }
        return false;
    }

    // Read GPIO value
    int value = Gpio_getValue(button_gpio);

    // Debug printing - show GPIO values to troubleshoot button press
    static int counter = 0;
    if (++counter % 50 == 0)
    { // Print every 50 calls to avoid flooding the console
        printf("Joystick button GPIO value: %d (button %s)\n",
               value,
               (value == 0) ? "PRESSED" : "NOT PRESSED");
    }

    // Return true if button is pressed (active-low: 0 = pressed)
    return (value == 0);
}

// Joystick sampling thread function
static void *joystickSamplingThread()
{
    struct timespec currentTime;
    JoystickDirection rawDirection;
    bool rawButtonPress;

    while (isRunning)
    {
        get_current_time(&currentTime);

        // Read raw values
        rawDirection = read_raw_joystick_direction();
        rawButtonPress = read_raw_button_press();

        // Lock mutex to update shared state
        pthread_mutex_lock(&joystickMutex);

        // Handle direction debouncing
        if (rawDirection != currentDirection)
        {
            if (get_time_diff_ms(&lastDirectionChange, &currentTime) > DEBOUNCE_TIMEOUT_MS)
            {
                currentDirection = rawDirection;
                lastDirectionChange = currentTime;
            }
        }

        // Handle button press debouncing
        if (rawButtonPress != buttonPressed)
        {
            if (get_time_diff_ms(&lastButtonChange, &currentTime) > DEBOUNCE_TIMEOUT_MS)
            {
                buttonPressed = rawButtonPress;
                lastButtonChange = currentTime;
            }
        }

        pthread_mutex_unlock(&joystickMutex);

        // Sleep to avoid consuming too much CPU
        usleep(10000); // 10ms
    }

    return NULL;
}

// Public functions

void Joystick_init(void)
{
    // Initialize I2C for the joystick
    i2c_file_desc = init_i2c_bus(I2CDRV_LINUX_BUS, I2C_DEVICE_ADDRESS);
    if (i2c_file_desc < 0)
    {
        printf("Joystick initialization failed - I2C error\n");
    }

    // Initialize GPIO for button
    Gpio_initialize();
    button_gpio = Gpio_openForEvents(JOYSTICK_BUTTON_CHIP, JOYSTICK_BUTTON_PIN);
    if (button_gpio == NULL)
    {
        printf("Joystick button GPIO initialization failed - GPIO%d might be in use by another process\n",
               JOYSTICK_BUTTON_PIN);
        // Continue without button functionality
    }
    else
    {
        printf("Joystick button GPIO%d initialized successfully\n", JOYSTICK_BUTTON_PIN);
    }

    // Initialize the debouncing timestamps
    get_current_time(&lastDirectionChange);
    get_current_time(&lastButtonChange);
}

void Joystick_cleanup(void)
{
    Joystick_stopSampling();

    if (i2c_file_desc != -1)
    {
        close(i2c_file_desc);
        i2c_file_desc = -1;
    }

    // Clean up GPIO
    if (button_gpio != NULL)
    {
        Gpio_close(button_gpio);
        button_gpio = NULL;
    }
    Gpio_cleanup();
}

JoystickDirection Joystick_getDirection(void)
{
    JoystickDirection result;

    pthread_mutex_lock(&joystickMutex);
    result = currentDirection;
    pthread_mutex_unlock(&joystickMutex);

    return result;
}

bool Joystick_isPressed(void)
{
    bool result;

    pthread_mutex_lock(&joystickMutex);
    result = buttonPressed;
    pthread_mutex_unlock(&joystickMutex);

    return result;
}

void Joystick_startSampling(void)
{
    if (!isRunning && i2c_file_desc >= 0)
    {
        isRunning = true;
        pthread_create(&joystickThreadId, NULL, joystickSamplingThread, NULL);
    }
}

void Joystick_stopSampling(void)
{
    if (isRunning)
    {
        isRunning = false;
        pthread_join(joystickThreadId, NULL);
    }
}
