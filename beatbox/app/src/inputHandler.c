#include "inputHandler.h"
#include "hal/joystick.h"
#include "hal/audioMixer.h"
#include "hal/lcdDisplay.h"

#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h> // For exit()

// Private variables
static JoystickDirection lastDirection = JOYSTICK_NONE;
static bool lastButtonState = false;    // Track previous button state for edge detection
static int holdCounter = 0;             // Counter for handling continuous adjustments
static int directionChangeCooldown = 0; // Cooldown counter for debouncing direction changes

// Timing constants for press vs hold distinction
#define INITIAL_DELAY_COUNT 100       // Initial delay before continuous adjustment (100ms)
#define CONTINUOUS_RATE 10            // Adjust volume every N cycles when holding (slower)
#define VOLUME_CHANGE_AMOUNT 5        // Standard volume change amount for initial press
#define HOLD_VOLUME_CHANGE_AMOUNT 1   // Smaller volume change when holding
#define DIRECTION_CHANGE_COOLDOWN 100 // Cooldown period after direction change (100ms)

// Threading variables
static pthread_t joystick_thread;
static volatile bool keep_thread_running = false;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Thread function for joystick processing
static void *joystick_thread_function(void *arg)
{
    (void)arg; // Silence unused parameter warning

    printf("Joystick thread started\n");

    while (keep_thread_running)
    {
        // Lock mutex before accessing shared variables
        pthread_mutex_lock(&mutex);

        // Get current joystick direction and button state
        JoystickDirection direction = Joystick_getDirection();
        bool buttonPressed = Joystick_isPressed();

        // Check if direction has changed
        bool directionChanged = (direction != lastDirection);

        // Handle button press for screen switching (on button press, not release)
        if (buttonPressed && !lastButtonState)
        {
            // Button was just pressed, switch to next screen
            LcdDisplay_nextScreen();
        }

        // Handle volume control with up/down
        if (direction == JOYSTICK_UP || direction == JOYSTICK_DOWN)
        {
            // Handle the volume change in three cases:
            // 1. Direction has just changed AND we're not in cooldown
            // 2. User is holding the joystick in a direction
            bool shouldChangeVolume = false;
            bool isHolding = false;

            if (directionChanged && directionChangeCooldown == 0)
            {
                // Case 1: Direction just changed and no cooldown active
                shouldChangeVolume = true;
                directionChangeCooldown = DIRECTION_CHANGE_COOLDOWN; // Start cooldown
                holdCounter = 0;                                     // Reset hold counter when direction changes
            }
            else if (!directionChanged && holdCounter >= INITIAL_DELAY_COUNT &&
                     holdCounter % CONTINUOUS_RATE == 0)
            {
                // Case 2: Holding joystick in a direction
                shouldChangeVolume = true;
                isHolding = true;
            }

            if (shouldChangeVolume)
            {
                int currentVolume = AudioMixer_getVolume();
                int newVolume = currentVolume;
                // Use smaller change amount for holding
                int changeAmount = isHolding ? HOLD_VOLUME_CHANGE_AMOUNT : VOLUME_CHANGE_AMOUNT;

                if (direction == JOYSTICK_UP)
                {
                    newVolume = currentVolume + changeAmount;
                    if (newVolume > AUDIOMIXER_MAX_VOLUME)
                    {
                        newVolume = AUDIOMIXER_MAX_VOLUME;
                    }
                }
                else if (direction == JOYSTICK_DOWN)
                {
                    newVolume = currentVolume - changeAmount;
                    if (newVolume < 0)
                    {
                        newVolume = 0;
                    }
                }

                if (newVolume != currentVolume)
                {
                    AudioMixer_setVolume(newVolume);
                    printf("Volume %s to: %d\n",
                           direction == JOYSTICK_UP ? "increased" : "decreased",
                           newVolume);
                }
            }

            // Increment hold counter only if direction hasn't changed
            if (!directionChanged)
            {
                holdCounter++;
            }
        }
        else
        {
            // Reset hold counter when in neutral position
            holdCounter = 0;
        }

        // Decrement cooldown counter if active
        if (directionChangeCooldown > 0)
        {
            directionChangeCooldown--;
        }

        // Update last states for next time
        lastDirection = direction;
        lastButtonState = buttonPressed;

        // Unlock mutex
        pthread_mutex_unlock(&mutex);

        // Sleep to reduce CPU usage
        usleep(10000); // 10ms for responsive input
    }

    printf("Joystick thread stopped\n");
    return NULL;
}

// Initialize the input handler
void InputHandler_init(void)
{
    // Initialize joystick for input handling
    Joystick_init();
    Joystick_startSampling();

    // Set default volume
    AudioMixer_setVolume(80);

    // Start the joystick processing thread
    keep_thread_running = true;
    if (pthread_create(&joystick_thread, NULL, joystick_thread_function, NULL) != 0)
    {
        perror("Failed to create joystick thread");
        exit(1);
    }

    printf("Input handler initialized\n");
}

// Clean up input handler resources
void InputHandler_cleanup(void)
{
    printf("Starting Input Handler cleanup...\n");

    // Stop the thread
    keep_thread_running = false;

    // Give the thread a chance to clean up
    usleep(100000); // 100ms

    // Join the thread with standard pthread_join
    // We'll set a short timeout and cancel if it takes too long
    int join_result = pthread_join(joystick_thread, NULL);
    if (join_result != 0)
    {
        printf("WARNING: Joystick thread join failed\n");
    }
    else
    {
        printf("Joystick thread joined successfully\n");
    }

    Joystick_stopSampling();
    Joystick_cleanup();

    printf("Input Handler cleanup complete\n");
}
