#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include "hal/firing_input.h"
#include "hal/gpio.h"

// GPIO configuration for button input
#define GPIO_CHIP GPIO_CHIP_0
#define GPIO_LINE_NUMBER 10 // Rotary Encoder PUSH button

// Debounce timing
#define DEBOUNCE_TIMEOUT_MS 250

// State variables
static bool is_initialized = false;
static pthread_t button_thread;
static volatile bool keep_running = false;
static volatile bool button_was_pressed = false;
static long last_press_time = 0;
static struct GpioLine *s_lineBtn = NULL;

// Mutex for protecting shared state
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// State machine data structures
struct stateEvent
{
    struct state *pNextState;
    void (*action)();
};

struct state
{
    struct stateEvent rising;
    struct stateEvent falling;
};

// Helper function to get current time in milliseconds
static long getCurrentTimeMs(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

// Button press action
static void on_release(void)
{
    // Debounce - Check if enough time has passed since last button press
    long currentTime = getCurrentTimeMs();
    if (currentTime - last_press_time < DEBOUNCE_TIMEOUT_MS)
    {
        return;
    }

    // Update last press time
    last_press_time = currentTime;

    // Set the button pressed flag
    pthread_mutex_lock(&mutex);
    button_was_pressed = true;
    pthread_mutex_unlock(&mutex);

    printf("Button press detected!\n");
}

// State machine states
struct state states[] = {
    {
        // Not pressed
        .rising = {&states[0], NULL},
        .falling = {&states[1], NULL},
    },
    {
        // Pressed
        .rising = {&states[0], on_release}, // Action on release (rising edge)
        .falling = {&states[1], NULL},
    },
};

// Current state pointer
static struct state *pCurrentState = &states[0];

// Thread function for monitoring button presses
static void *button_thread_function(void *arg)
{
    (void)arg; // Unused parameter

    while (keep_running)
    {
        // Wait for a GPIO event
        struct gpiod_line_bulk bulkEvents;
        int numEvents = Gpio_waitForLineChange(s_lineBtn, &bulkEvents);

        // Process all events
        for (int i = 0; i < numEvents; i++)
        {
            struct gpiod_line *line_handle = gpiod_line_bulk_get_line(&bulkEvents, i);
            unsigned int this_line_number = gpiod_line_offset(line_handle);

            struct gpiod_line_event event;
            if (gpiod_line_event_read(line_handle, &event) == -1)
            {
                perror("Line Event");
                continue;
            }

            bool isRising = event.event_type == GPIOD_LINE_EVENT_RISING_EDGE;
            bool isBtn = this_line_number == GPIO_LINE_NUMBER;

            if (!isBtn)
            {
                continue;
            }

            // Process the event through the state machine
            struct stateEvent *pStateEvent = isRising ? &pCurrentState->rising : &pCurrentState->falling;

            if (pStateEvent->action != NULL)
            {
                pStateEvent->action();
            }
            pCurrentState = pStateEvent->pNextState;
        }
    }

    return NULL;
}

// Initialize the firing input
bool FiringInput_init(void)
{
    if (is_initialized)
    {
        return true; // Already initialized
    }

    // Open GPIO line for events
    s_lineBtn = Gpio_openForEvents(GPIO_CHIP, GPIO_LINE_NUMBER);
    if (!s_lineBtn)
    {
        printf("ERROR: Failed to open GPIO line for button input\n");
        return false;
    }

    // Initialize state variables
    button_was_pressed = false;
    pCurrentState = &states[0];
    last_press_time = getCurrentTimeMs();
    keep_running = true;

    // Create button monitoring thread
    if (pthread_create(&button_thread, NULL, button_thread_function, NULL) != 0)
    {
        printf("Error: Failed to create button monitoring thread\n");
        Gpio_close(s_lineBtn);
        return false;
    }

    is_initialized = true;
    printf("Firing input initialized successfully\n");
    return true;
}

// Clean up resources
void FiringInput_cleanup(void)
{
    if (!is_initialized)
    {
        return; // Not initialized, nothing to clean up
    }

    // Stop the thread
    keep_running = false;

    // Wait for thread to terminate
    pthread_join(button_thread, NULL);

    // Close GPIO resources
    Gpio_close(s_lineBtn);

    is_initialized = false;
    printf("Firing input cleaned up\n");
}

// Check if button was pressed since last call
bool FiringInput_wasButtonPressed(void)
{
    bool was_pressed = false;

    // Safely access and reset the button_was_pressed flag
    pthread_mutex_lock(&mutex);
    was_pressed = button_was_pressed;
    if (was_pressed)
    {
        button_was_pressed = false; // Reset the flag
    }
    pthread_mutex_unlock(&mutex);

    return was_pressed;
}