// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include "hal/sampler.h"
#include "hal/udp_server.h"
#include "hal/periodTimer.h"
#include "hal/pwm_led.h"
#include "hal/gpio.h"
#include "hal/btn_statemachine.h"
#include "hal/rotary_encoder.h"
#include "lcd_display_impl.h"
#include "hal/terminal_display.h"

#define MAX_LCD_MESSAGE 1024 // Increased buffer size like example
#define NAME "Omar E"
#define MAX_FREQUENCY 500.0

static void process_rotary(void)
{
    int direction = RotaryEncoder_process();
    if (direction != 0)
    {
        // Get current frequency and adjust by 1Hz
        double freq = PwmLed_getFrequency();
        freq += direction;

        // Clamp frequency between 0 and MAX_FREQUENCY Hz
        if (freq < 0)
        {
            freq = 0;
        }
        if (freq > MAX_FREQUENCY)
        {
            freq = MAX_FREQUENCY;
        }

        // Only update if frequency changed
        if (freq != PwmLed_getFrequency())
        {
            PwmLed_setFrequency(freq);
        }
    }
}

int main()
{
    printf("Starting Light Sensor Sampling...\n");

    // Initialize all modules; HAL modules first
    Gpio_initialize();
    Period_init();
    Sampler_init();
    UdpServer_init();
    PwmLed_init();
    LcdDisplayImpl_init();
    TerminalDisplay_init();
    BtnStateMachine_init();
    RotaryEncoder_init();

    // Main loop: Process rotary encoder and update LCD
    while (!UdpServer_shouldStop())
    {

        // Process rotary encoder (now non-blocking)
        process_rotary();

        // Get statistics and update display
        Period_statistics_t stats;
        Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, &stats);

        // Get and verify values
        double freq = PwmLed_getFrequency();
        int dips = Sampler_getDips();

        // Format message for LCD with more information
        char buff[MAX_LCD_MESSAGE];
        snprintf(buff, MAX_LCD_MESSAGE,
                 "%s\n"
                 "Flash @ %.0f Hz\n"
                 "Dips = %d\n"
                 "Max ms: %.1f",
                 NAME, freq, dips, stats.maxPeriodInMs);

        LcdDisplayImpl_update(buff);

        // Move samples to history for next update
        Sampler_moveCurrentDataToHistory();

        sleep(1);
    }

    // Cleanup all modules (HAL modules last)
    RotaryEncoder_cleanup();
    BtnStateMachine_cleanup();
    TerminalDisplay_cleanup();
    LcdDisplayImpl_cleanup();
    PwmLed_cleanup();
    UdpServer_cleanup();
    Sampler_cleanup();
    Period_cleanup();
    Gpio_cleanup();

    printf("Program terminated.\n");
    return 0;
}