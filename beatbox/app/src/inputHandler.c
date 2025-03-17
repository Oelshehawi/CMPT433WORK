#include "inputHandler.h"
#include "hal/joystick.h"
#include "hal/audioMixer.h"
#include "hal/lcdDisplay.h"

#include <stdbool.h>
#include <stdio.h>

// Private variables
static JoystickDirection lastDirection = JOYSTICK_NONE;
static bool lastButtonState = false; // Track previous button state for edge detection
static int holdCounter = 0;          // Counter for handling continuous adjustments
#define INITIAL_DELAY_COUNT 5        // Reduced initial delay for faster response
#define CONTINUOUS_RATE 2            // Adjust volume every time when holding

// Initialize the input handler
void InputHandler_init(void)
{
    // Initialize joystick for input handling
    Joystick_init();
    Joystick_startSampling();

    // Set default volume
    AudioMixer_setVolume(80);

    printf("Input handler initialized\n");
}

// Clean up input handler resources
void InputHandler_cleanup(void)
{
    Joystick_stopSampling();
    Joystick_cleanup();
}

// Process joystick input for volume control and screen switching
void InputHandler_processJoystick(void)
{
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
    if (direction == JOYSTICK_UP)
    {
        // Either direction just changed OR we're holding long enough for continuous adjustment
        if (directionChanged || (holdCounter >= INITIAL_DELAY_COUNT && holdCounter % CONTINUOUS_RATE == 0))
        {
            int currentVolume = AudioMixer_getVolume();
            int newVolume = currentVolume + 5;
            if (newVolume > AUDIOMIXER_MAX_VOLUME)
            {
                newVolume = AUDIOMIXER_MAX_VOLUME;
            }
            AudioMixer_setVolume(newVolume);
        }

        // Increment hold counter when holding in a direction
        holdCounter++;
    }
    else if (direction == JOYSTICK_DOWN)
    {
        // Either direction just changed OR we're holding long enough for continuous adjustment
        if (directionChanged || (holdCounter >= INITIAL_DELAY_COUNT && holdCounter % CONTINUOUS_RATE == 0))
        {
            int currentVolume = AudioMixer_getVolume();
            int newVolume = currentVolume - 5;
            if (newVolume < 0)
            {
                newVolume = 0;
            }
            AudioMixer_setVolume(newVolume);
        }

        // Increment hold counter when holding in a direction
        holdCounter++;
    }
    else
    {
        // Reset hold counter when in neutral position
        holdCounter = 0;
    }

    // Update last states for next time
    lastDirection = direction;
    lastButtonState = buttonPressed;
}