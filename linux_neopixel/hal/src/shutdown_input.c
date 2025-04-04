#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <gpiod.h>

#include "hal/shutdown_input.h"
#include "hal/gpio.h"

// GPIO configuration for joystick center button
#define SHUTDOWN_BUTTON_CHIP GPIO_CHIP_2
#define SHUTDOWN_BUTTON_PIN 15

// Debounce time in milliseconds
#define DEBOUNCE_TIME_MS 100

// Thread control variables
static pthread_t button_thread;
static volatile bool is_running = false;

// Button state variables
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile bool shutdown_requested = false;
static long last_press_time = 0;

// GPIO handle
static struct GpioLine *button_gpio = NULL;

// Helper function to get current time in milliseconds
static long get_current_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

// Thread for monitoring button presses
static void *button_thread_function(void *arg)
{
    (void)arg; // Unused parameter

    // Initialize last press time
    last_press_time = get_current_time_ms();

    while (is_running && button_gpio != NULL)
    {
        // Wait for button events
        struct gpiod_line_bulk bulk_events;
        int num_events = Gpio_waitForLineChange(button_gpio, &bulk_events);

        for (int i = 0; i < num_events; i++)
        {
            struct gpiod_line *line = gpiod_line_bulk_get_line(&bulk_events, i);
            unsigned int pin = gpiod_line_offset(line);

            // Make sure this is our button
            if (pin != SHUTDOWN_BUTTON_PIN)
            {
                continue;
            }

            // Read the event
            struct gpiod_line_event event;
            if (gpiod_line_event_read(line, &event) != 0)
            {
                perror("Failed to read line event");
                continue;
            }

            // We only care about falling edge (button press)
            if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE)
            {
                // Apply debouncing
                long current_time = get_current_time_ms();
                if (current_time - last_press_time > DEBOUNCE_TIME_MS)
                {
                    // Set the shutdown requested flag
                    pthread_mutex_lock(&mutex);
                    shutdown_requested = true;
                    pthread_mutex_unlock(&mutex);

                    printf("Shutdown button pressed\n");
                    last_press_time = current_time;
                }
            }
        }

        // Small sleep to prevent excessive CPU usage if no events
        usleep(10000); // 10ms
    }

    return NULL;
}

// Initialize the shutdown input
bool ShutdownInput_init(void)
{
    // Check if already initialized
    if (button_gpio != NULL)
    {
        return true;
    }

    // Initialize GPIO for button
    button_gpio = Gpio_openForEvents(SHUTDOWN_BUTTON_CHIP, SHUTDOWN_BUTTON_PIN);
    if (button_gpio == NULL)
    {
        printf("ERROR: Shutdown button GPIO initialization failed\n");
        return false;
    }

    // Initialize state
    shutdown_requested = false;
    is_running = true;

    // Create background thread for monitoring
    if (pthread_create(&button_thread, NULL, button_thread_function, NULL) != 0)
    {
        printf("ERROR: Failed to create shutdown button monitoring thread\n");
        Gpio_close(button_gpio);
        button_gpio = NULL;
        return false;
    }

    printf("Shutdown button input initialized successfully\n");
    return true;
}

// Clean up resources
void ShutdownInput_cleanup(void)
{
    // Stop the thread
    if (is_running)
    {
        is_running = false;
        pthread_join(button_thread, NULL);
    }

    // Clean up GPIO
    if (button_gpio != NULL)
    {
        Gpio_close(button_gpio);
        button_gpio = NULL;
    }

    printf("Shutdown button input cleaned up\n");
}

// Check if shutdown was requested
bool ShutdownInput_isShutdownRequested(void)
{
    bool result = false;

    // Safely check and reset the flag
    pthread_mutex_lock(&mutex);
    if (shutdown_requested)
    {
        result = true;
        shutdown_requested = false; // Reset the flag
    }
    pthread_mutex_unlock(&mutex);

    return result;
}