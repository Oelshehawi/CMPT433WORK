// Sample state machine for one GPIO pin.

#include "hal/buttonStateMachine.h"
#include "hal/gpio.h"
#include "../include/hal/rotaryEncoder.h" // For direct access to beat modes

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>

// Pin config info: GPIO 24 (Rotary Encoder PUSH)
//   $ gpiofind GPIO24
//   >> gpiochip0 10
#define GPIO_CHIP GPIO_CHIP_0
#define GPIO_LINE_NUMBER 10

static bool isInitialized = false;

struct GpioLine *s_lineBtn = NULL;
static atomic_int counter = 0;

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

/*
    START STATEMACHINE
*/
static void on_release(void)
{
    counter++;
    printf("DEBUG: Button press detected (counter=%d)\n", counter);

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

    printf("DEBUG: Beat mode changed to: %d\n", currentBeatMode);
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
    printf("DEBUG: Initializing Button State Machine...\n");
    assert(!isInitialized);
    s_lineBtn = Gpio_openForEvents(GPIO_CHIP, GPIO_LINE_NUMBER);
    if (!s_lineBtn)
    {
        printf("ERROR: Failed to open GPIO line for button state machine\n");
        exit(EXIT_FAILURE);
    }
    isInitialized = true;
    printf("DEBUG: Button State Machine initialized successfully\n");
}

void BtnStateMachine_cleanup()
{
    printf("DEBUG: Cleaning up Button State Machine...\n");
    assert(isInitialized);
    isInitialized = false;
    Gpio_close(s_lineBtn);
    printf("DEBUG: Button State Machine cleanup complete\n");
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

    // printf("\n\nWaiting for an event...\n");
    struct gpiod_line_bulk bulkEvents;
    int numEvents = Gpio_waitForLineChange(s_lineBtn, &bulkEvents);

    if (numEvents > 0)
    {
        printf("DEBUG: Button state machine detected %d events\n", numEvents);
    }

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
        printf("DEBUG: Button event: %s edge\n", isRising ? "RISING" : "FALLING");

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
