#include "hal/pwm_led.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#define PWM_PATH "/dev/hat/pwm/GPIO12"
#define MAX_PERIOD_NS 469754879
#define MIN_PERIOD_NS 1000000 // 1ms minimum period (1000Hz max)

static bool is_initialized = false;
static double current_frequency = 10.0; // Start at 10Hz

// File descriptors for PWM control
static int fd_enable = -1;
static int fd_period = -1;
static int fd_duty = -1;

static void write_pwm_value(int fd, long value)
{
    // Convert value to string
    char str[32];
    snprintf(str, sizeof(str), "%ld", value);

    // Seek to start of file
    if (lseek(fd, 0, SEEK_SET) == -1)
    {
        perror("Error seeking in PWM file");
        return;
    }

    // Write the value
    if (write(fd, str, strlen(str)) == -1)
    {
        perror("Error writing to PWM file");
        return;
    }
}

static void set_pwm_values(long period_ns, long duty_ns)
{
    // Always set duty to 0 first
    write_pwm_value(fd_duty, 0);

    // Set new period
    write_pwm_value(fd_period, period_ns);

    // Set new duty cycle
    write_pwm_value(fd_duty, duty_ns);
}

void PwmLed_init(void)
{
    printf("PWM LED - Initializing\n");

    // Open PWM control files
    char path[128];

    snprintf(path, sizeof(path), "%s/enable", PWM_PATH);
    fd_enable = open(path, O_WRONLY);

    snprintf(path, sizeof(path), "%s/period", PWM_PATH);
    fd_period = open(path, O_WRONLY);

    snprintf(path, sizeof(path), "%s/duty_cycle", PWM_PATH);
    fd_duty = open(path, O_WRONLY);

    if (fd_enable == -1 || fd_period == -1 || fd_duty == -1)
    {
        perror("Error opening PWM files");
        exit(1);
    }

    // Initialize with LED off
    write_pwm_value(fd_duty, 0);

    // Set initial frequency (10Hz)
    long period_ns = 100000000;   // 100ms = 10Hz
    long duty_ns = period_ns / 2; // 50% duty cycle
    set_pwm_values(period_ns, duty_ns);

    // Enable PWM
    write_pwm_value(fd_enable, 1);

    is_initialized = true;
}

void PwmLed_cleanup(void)
{
    printf("PWM LED - Cleanup\n");

    if (is_initialized)
    {
        // Disable PWM
        if (fd_enable != -1)
        {
            write_pwm_value(fd_enable, 0);
            close(fd_enable);
        }

        if (fd_period != -1)
            close(fd_period);
        if (fd_duty != -1)
            close(fd_duty);

        is_initialized = false;
    }
}

void PwmLed_setFrequency(double freq_hz)
{
    if (!is_initialized)
        return;

    // Limit frequency to valid range (0 to 1000Hz)
    if (freq_hz > 1000)
        freq_hz = 1000;

    // For frequencies below 3Hz, turn off the LED
    if (freq_hz < 3.0)
    {
        // Turn off PWM by setting duty cycle to 0
        write_pwm_value(fd_duty, 0);
        current_frequency = freq_hz;
        return;
    }

    // Convert frequency to period in nanoseconds
    long period_ns = (freq_hz > 0) ? (1000000000 / freq_hz) : MAX_PERIOD_NS;

    // Ensure period is within valid range
    if (period_ns > MAX_PERIOD_NS)
        period_ns = MAX_PERIOD_NS;
    if (period_ns < MIN_PERIOD_NS)
        period_ns = MIN_PERIOD_NS;

    // Set 50% duty cycle
    long duty_ns = period_ns / 2;

    // Only update if frequency changed to avoid interruption
    if (freq_hz != current_frequency)
    {
        // Update PWM values
        set_pwm_values(period_ns, duty_ns);
        current_frequency = freq_hz;
    }
}

double PwmLed_getFrequency(void)
{
    return current_frequency;
}