#include "inputHandler.h"
#include "hal/joystick.h"
#include "hal/audioMixer.h"
#include "hal/lcdDisplay.h"

#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h> 

// Private variables
static JoystickDirection lastDirection = JOYSTICK_NONE;
static bool lastButtonState = false;    
static int holdCounter = 0;             
static int directionChangeCooldown = 0; 

// Timing constants for press vs hold distinction
#define INITIAL_DELAY_COUNT 100       
#define CONTINUOUS_RATE 10            
#define VOLUME_CHANGE_AMOUNT 5        
#define HOLD_VOLUME_CHANGE_AMOUNT 1   
#define DIRECTION_CHANGE_COOLDOWN 100 

// Threading variables
static pthread_t joystick_thread;
static volatile bool keep_thread_running = false;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Thread function for joystick processing
static void *joystick_thread_function(void *arg)
{
    (void)arg; 

    printf("Joystick thread started\n");

    while (keep_thread_running)
    {
        pthread_mutex_lock(&mutex);

        // Get current joystick direction and button state
        JoystickDirection direction = Joystick_getDirection();
        bool buttonPressed = Joystick_isPressed();

        bool directionChanged = (direction != lastDirection);

        // Handle button press for screen switching (on button press, not release)
        if (buttonPressed && !lastButtonState)
        {
            LcdDisplay_nextScreen();
        }

        // Handle volume control with up/down
        if (direction == JOYSTICK_UP || direction == JOYSTICK_DOWN)
        {

            bool shouldChangeVolume = false;
            bool isHolding = false;

            if (directionChanged && directionChangeCooldown == 0)
            {
                // Case 1: Direction just changed and no cooldown active
                shouldChangeVolume = true;
                directionChangeCooldown = DIRECTION_CHANGE_COOLDOWN; 
                holdCounter = 0;                                     
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

        pthread_mutex_unlock(&mutex);

        usleep(10000); 
    }

    printf("Joystick thread stopped\n");
    return NULL;
}

// Initialize the input handler
void InputHandler_init(void)
{
    Joystick_init();
    Joystick_startSampling();
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

    keep_thread_running = false;

    usleep(100000); 

    // Join the thread
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
