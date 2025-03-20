// Sample state machine for one GPIO pin.

#include "hal/buttonStateMachine.h"
#include "hal/gpio.h"
#include "../include/hal/rotaryEncoder.h" // For direct access to beat modes
#include "../../app/include/beatPlayer.h" // For directly updating the beat player mode

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>

// Pin config info: GPIO 24 (Rotary Encoder PUSH)
//   $ gpiofind GPIO24
//   >> gpiochip0 10
#define GPIO_CHIP GPIO_CHIP_0
#define GPIO_LINE_NUMBER 10

// Debounce parameters
#define DEBOUNCE_TIMEOUT_MS 250 // 250ms between button actions

static bool isInitialized = false;
struct GpioLine *s_lineBtn = NULL;
static atomic_int counter = 0;
static long lastButtonPressTime = 0;

// Current beat mode (shared with rotaryEncoder)
extern BeatMode_t currentBeatMode;

/*
    Define the Statemachine Data Structures
*/
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

// Get current time in milliseconds
static long getCurrentTimeMs(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

/*
    START STATEMACHINE
*/
static void on_release(void)
{
    // Debounce - check if enough time has passed since last button press
    long currentTime = getCurrentTimeMs();
    if (currentTime - lastButtonPressTime < DEBOUNCE_TIMEOUT_MS)
    {
        return; // Ignore this press (debouncing)
    }

    // Update last press time
    lastButtonPressTime = currentTime;
    counter++;

    // Directly cycle through beat modes
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

    // Update the BeatPlayer with the new mode
    BeatPlayer_setMode(currentBeatMode);
    printf("Beat mode changed to: %d\n", currentBeatMode);
}

struct state states[] = {
    {
        // Not pressed
        .rising = {&states[0], NULL},
        .falling = {&states[1], NULL},
    },

    {
        // Pressed
        .rising = {&states[0], on_release},
        .falling = {&states[1], NULL},
    },
};
/*
    END STATEMACHINE
*/

struct state *pCurrentState = &states[0];

void BtnStateMachine_init()
{
    printf("Initializing Button State Machine...\n");
    assert(!isInitialized);
    s_lineBtn = Gpio_openForEvents(GPIO_CHIP, GPIO_LINE_NUMBER);
    if (!s_lineBtn)
    {
        printf("ERROR: Failed to open GPIO line for button state machine\n");
        exit(EXIT_FAILURE);
    }
    isInitialized = true;
    lastButtonPressTime = getCurrentTimeMs(); // Initialize time
    printf("Button State Machine initialized successfully\n");
}

void BtnStateMachine_cleanup()
{
    printf("Cleaning up Button State Machine...\n");
    assert(isInitialized);
    isInitialized = false;
    Gpio_close(s_lineBtn);
    printf("Button State Machine cleanup complete\n");
}

int BtnStateMachine_getValue()
{
    return counter;
}

// We'll keep this for compatibility but it's not used anymore
void BtnStateMachine_setAction(ButtonAction action)
{
    (void)action; // Unused parameter
    // We're using the direct on_release implementation now
}

void BtnStateMachine_doState()
{
    assert(isInitialized);

    struct gpiod_line_bulk bulkEvents;
    int numEvents = Gpio_waitForLineChange(s_lineBtn, &bulkEvents);

    // Iterate over the event
    for (int i = 0; i < numEvents; i++)
    {
        // Get the line handle for this event
        struct gpiod_line *line_handle = gpiod_line_bulk_get_line(&bulkEvents, i);

        // Get the number of this line
        unsigned int this_line_number = gpiod_line_offset(line_handle);

        // Get the line event
        struct gpiod_line_event event;
        if (gpiod_line_event_read(line_handle, &event) == -1)
        {
            perror("Line Event");
            exit(EXIT_FAILURE);
        }

        // Run the state machine
        bool isRising = event.event_type == GPIOD_LINE_EVENT_RISING_EDGE;

        // Can check with line it is, if you have more than one...
        bool isBtn = this_line_number == GPIO_LINE_NUMBER;
        assert(isBtn);

        struct stateEvent *pStateEvent = NULL;
        if (isRising)
        {
            pStateEvent = &pCurrentState->rising;
        }
        else
        {
            pStateEvent = &pCurrentState->falling;
        }

        // Do the action
        if (pStateEvent->action != NULL)
        {
            pStateEvent->action();
        }
        pCurrentState = pStateEvent->pNextState;
    }
}
