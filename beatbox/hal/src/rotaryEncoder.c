#include "hal/rotaryEncoder.h"
#include "hal/buttonStateMachine.h"
#include "hal/gpio.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>

// GPIO configuration
#define BUTTON_GPIO_CHIP GPIO_CHIP_0
#define BUTTON_GPIO_LINE 10 // GPIO10 for button press on rotary encoder

// This is now shared with buttonStateMachine.c
BeatMode_t currentBeatMode = BEAT_MODE_ROCK;
static bool is_initialized = false;
static pthread_t encoder_thread;
static volatile bool keep_running = false;
static volatile int last_direction = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// This function is no longer needed as buttonStateMachine.c handles it directly
/*
static void on_button_press(void)
{
    // Cycle through beat modes
    pthread_mutex_lock(&mutex);
    switch (currentBeatMode)
    {
    case BEAT_MODE_NONE:
        currentBeatMode = BEAT_MODE_ROCK;
        break;
    case BEAT_MODE_ROCK:
        currentBeatMode = BEAT_MODE_CUSTOM;
        break;
    case BEAT_MODE_CUSTOM:
        currentBeatMode = BEAT_MODE_NONE;
        break;
    default:
        currentBeatMode = BEAT_MODE_NONE;
    }
    pthread_mutex_unlock(&mutex);

    printf("Rotary Encoder: Beat mode changed to: %d\n", currentBeatMode);
}
*/

static void *encoder_thread_function()
{
    printf("DEBUG: Encoder thread started\n");

    while (keep_running)
    {
        // Process button events
        BtnStateMachine_doState();

        // Short sleep to reduce CPU usage but keep responsive
        usleep(5000); // 5ms
    }

    printf("DEBUG: Encoder thread stopped\n");
    return NULL;
}

void RotaryEncoder_init(void)
{
    assert(!is_initialized);

    printf("DEBUG: Initializing Rotary Encoder...\n");

    // NOTE: BtnStateMachine_init() is now called from main.c before this function
    // to avoid opening the same GPIO line twice

    // We don't need to set an action anymore as buttonStateMachine.c handles it directly
    // BtnStateMachine_setAction(on_button_press);

    // Start the encoder thread
    keep_running = true;
    if (pthread_create(&encoder_thread, NULL, encoder_thread_function, NULL) != 0)
    {
        perror("Failed to create encoder thread");
        exit(1);
    }

    is_initialized = true;
    printf("DEBUG: Rotary Encoder initialization complete\n");
}

void RotaryEncoder_cleanup(void)
{
    if (!is_initialized)
        return;

    printf("DEBUG: Starting Rotary Encoder cleanup...\n");

    // Stop the thread
    keep_running = false;

    // Give the thread a chance to clean up
    usleep(100000); // 100ms

    // Join the thread to ensure it's fully stopped
    pthread_join(encoder_thread, NULL);
    printf("DEBUG: Rotary Encoder - Thread joined successfully\n");

    // NOTE: BtnStateMachine_cleanup() is now called from main.c after this function

    is_initialized = false;
    printf("DEBUG: Rotary Encoder - Cleanup complete\n");
}

int RotaryEncoder_process(void)
{
    pthread_mutex_lock(&mutex);
    int direction = last_direction;
    last_direction = 0; // Reset after reading
    pthread_mutex_unlock(&mutex);
    return direction;
}

// Get current beat mode
BeatMode_t RotaryEncoder_getBeatMode(void)
{
    BeatMode_t mode;
    pthread_mutex_lock(&mutex);
    mode = currentBeatMode;
    pthread_mutex_unlock(&mutex);
    return mode;
}

// These functions are simplified and don't actually handle rotation
// but are kept for interface compatibility
int RotaryEncoder_getBPM(void)
{
    return DEFAULT_BPM;
}

void RotaryEncoder_setBPM(int bpm)
{
    // Silence unused parameter warning
    (void)bpm;
    // Simplified implementation - does nothing
}