// This file continually samples the light sensor using a thread,
// with multiple helper functions that compute average, get total number of samples etc.
// also enables the light sensor on zen hat to read light using I2C

#include "hal/sampler.h"
#include "hal/periodTimer.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>

// Device bus & address
#define I2CDRV_LINUX_BUS "/dev/i2c-1"
#define I2C_DEVICE_ADDRESS 0x48

// Register in TLA2024
#define REG_CONFIGURATION 0x01
#define REG_DATA 0x00

// Configuration for channel 2 (E)
#define TLA2024_CHANNEL_CONF_2 0x83E2

// ADC Configuration
#define ADC_MAX_VALUE 4096.0 // 12-bit ADC
#define ADC_VREF 3.3         // Reference voltage is 3.3V

// Sampling configuration
#define MAX_SAMPLES_PER_SECOND 700
#define SMOOTHING_FACTOR 0.999 // 99.9% weight for previous average

// Thread and synchronization
static pthread_t sampling_thread;
static pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;
static int i2c_file_desc = -1;
static volatile bool should_stop = false;
static bool is_initialized = false;

// Sampling data
static double current_samples[MAX_SAMPLES_PER_SECOND];
static int current_sample_count = 0;
static double history_samples[MAX_SAMPLES_PER_SECOND];
static int history_sample_count = 0;
static double current_average = 0.0;
static bool first_sample = true;
static long long total_samples = 0;

// Forward declarations
static int init_i2c_bus(char *bus, int address);
static void write_i2c_reg16(int i2c_file_desc, uint8_t reg_addr, uint16_t value);
static uint16_t read_i2c_reg16(int i2c_file_desc, uint8_t reg_addr);
static double convert_to_voltage(uint16_t raw_value);
static double read_light_value(void);
static void *sampling_thread_function();
static int analyze_dips(void);

// I2C helper functions
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
    uint8_t buff[3];
    buff[0] = reg_addr;
    buff[1] = (value & 0xFF);
    buff[2] = (value & 0xFF00) >> 8;
    if (write(i2c_file_desc, buff, 3) != 3)
    {
        perror("Unable to write i2c register");
        exit(EXIT_FAILURE);
    }
}

static uint16_t read_i2c_reg16(int i2c_file_desc, uint8_t reg_addr)
{
    if (write(i2c_file_desc, &reg_addr, 1) != 1)
    {
        perror("Unable to write i2c register.");
        exit(EXIT_FAILURE);
    }

    uint16_t value = 0;
    if (read(i2c_file_desc, &value, 2) != 2)
    {
        perror("Unable to read i2c register");
        exit(EXIT_FAILURE);
    }
    return value;
}

static double convert_to_voltage(uint16_t raw_value)
{
    // Simple conversion: (3.3V / 4096) * ADC_value
    return (ADC_VREF / ADC_MAX_VALUE) * raw_value;
}

static double read_light_value(void)
{
    // Read raw value (LSB first)
    uint16_t raw_read = read_i2c_reg16(i2c_file_desc, REG_DATA);

    // Convert from LSB first to MSB first
    uint16_t value = ((raw_read & 0xFF) << 8) | ((raw_read & 0xFF00) >> 8);

    // Right align the 12-bit value
    value = value >> 4;

    // Convert to voltage
    return convert_to_voltage(value);
}

static void *sampling_thread_function()
{
    while (!should_stop)
    {
        // Read sensor and update statistics
        double sample = read_light_value();
        Period_markEvent(PERIOD_EVENT_SAMPLE_LIGHT);

        pthread_mutex_lock(&data_mutex);

        // Update total samples count
        total_samples++;

        // Update exponential moving average
        if (first_sample)
        {
            current_average = sample;
            first_sample = false;
        }
        else
        {
            current_average = (SMOOTHING_FACTOR * current_average) +
                              ((1.0 - SMOOTHING_FACTOR) * sample);
        }

        // Store sample in current buffer if there's room
        if (current_sample_count < MAX_SAMPLES_PER_SECOND)
        {
            current_samples[current_sample_count++] = sample;
        }

        pthread_mutex_unlock(&data_mutex);

        // Sleep for 1ms between samples
        usleep(1000);
    }
    return NULL;
}

void Sampler_init(void)
{
    printf("Sampler - Initializing\n");
    assert(!is_initialized);

    // Initialize I2C and configure for channel 2
    i2c_file_desc = init_i2c_bus(I2CDRV_LINUX_BUS, I2C_DEVICE_ADDRESS);
    write_i2c_reg16(i2c_file_desc, REG_CONFIGURATION, TLA2024_CHANNEL_CONF_2);

    // Reset all counters and flags
    should_stop = false;
    current_sample_count = 0;
    history_sample_count = 0;
    first_sample = true;
    total_samples = 0;

    // Start sampling thread
    pthread_create(&sampling_thread, NULL, sampling_thread_function, NULL);

    is_initialized = true;
}

void Sampler_cleanup(void)
{
    printf("Sampler - Cleanup\n");
    assert(is_initialized);

    // Stop sampling thread
    should_stop = true;
    pthread_join(sampling_thread, NULL);

    // Cleanup I2C
    if (i2c_file_desc >= 0)
    {
        close(i2c_file_desc);
        i2c_file_desc = -1;
    }

    is_initialized = false;
}

void Sampler_moveCurrentDataToHistory(void)
{
    pthread_mutex_lock(&data_mutex);

    // Copy current samples to history
    memcpy(history_samples, current_samples, current_sample_count * sizeof(double));
    history_sample_count = current_sample_count;

    // Reset current sample buffer
    current_sample_count = 0;

    pthread_mutex_unlock(&data_mutex);
}

int Sampler_getHistorySize(void)
{
    pthread_mutex_lock(&data_mutex);
    int size = history_sample_count;
    pthread_mutex_unlock(&data_mutex);
    return size;
}

double *Sampler_getHistory(int *size)
{
    pthread_mutex_lock(&data_mutex);

    // Allocate and copy history data
    double *copy = malloc(history_sample_count * sizeof(double));
    memcpy(copy, history_samples, history_sample_count * sizeof(double));
    *size = history_sample_count;

    pthread_mutex_unlock(&data_mutex);
    return copy;
}

double Sampler_getAverageReading(void)
{
    pthread_mutex_lock(&data_mutex);
    double avg = current_average;
    pthread_mutex_unlock(&data_mutex);
    return avg;
}

long long Sampler_getNumSamplesTaken(void)
{
    pthread_mutex_lock(&data_mutex);
    long long count = total_samples;
    pthread_mutex_unlock(&data_mutex);
    return count;
}

// Returns number of dips detected in the history buffer
static int analyze_dips(void)
{
    const double DIP_THRESHOLD = 0.1; // Voltage must drop by 0.1V to count as dip
    const double HYSTERESIS = 0.03;   // Must rise by 0.07V (0.1 - 0.03) before can trigger again
    const double RESET_THRESHOLD = DIP_THRESHOLD - HYSTERESIS;

    int dip_count = 0;
    bool waiting_for_reset = false;

    pthread_mutex_lock(&data_mutex);

    // Get current average for reference
    double avg = current_average;

    // Look for dips in history buffer
    for (int i = 0; i < history_sample_count; i++)
    {
        double diff = avg - history_samples[i];

        if (!waiting_for_reset)
        {
            // Looking for start of dip
            if (diff >= DIP_THRESHOLD)
            {
                dip_count++;
                waiting_for_reset = true;
            }
        }
        else
        {
            // Waiting for voltage to rise before we can detect another dip
            if (diff <= RESET_THRESHOLD)
            {
                waiting_for_reset = false;
            }
        }
    }

    pthread_mutex_unlock(&data_mutex);
    return dip_count;
}

int Sampler_getDips(void)
{
    return analyze_dips();
}