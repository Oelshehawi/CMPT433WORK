// This file is used to handle the state machine 
// for the rotary encoder button press

#include "hal/buttonStateMachine.h"
#include "hal/gpio.h"
#include "../../app/include/beatPlayer.h" // For directly updating the beat player mode

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>

// otary Encoder PUSH
#define GPIO_CHIP GPIO_CHIP_0
#define GPIO_LINE_NUMBER 10

#define DEBOUNCE_TIMEOUT_MS 250 

static bool isInitialized = false;
struct GpioLine *s_lineBtn = NULL;
static atomic_int counter = 0;
static long lastButtonPressTime = 0;

extern BeatMode_t currentBeatMode;

// Statemachine data structures
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


static void on_release(void)
{
    // Debounce 
    // Check if enough time has passed since last button press
    long currentTime = getCurrentTimeMs();
    if (currentTime - lastButtonPressTime < DEBOUNCE_TIMEOUT_MS)
    {
        return; 
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

struct state *pCurrentState = &states[0];

void BtnStateMachine_init()
{
    assert(!isInitialized);
    s_lineBtn = Gpio_openForEvents(GPIO_CHIP, GPIO_LINE_NUMBER);
    if (!s_lineBtn)
    {
        printf("ERROR: Failed to open GPIO line for button state machine\n");
        exit(EXIT_FAILURE);
    }
    isInitialized = true;
    lastButtonPressTime = getCurrentTimeMs(); // Initialize time
}

void BtnStateMachine_cleanup()
{
    assert(isInitialized);
    isInitialized = false;
    Gpio_close(s_lineBtn);
}

int BtnStateMachine_getValue()
{
    return counter;
}


void BtnStateMachine_setAction(ButtonAction action)
{
    (void)action;
}

// Main function to run the state machine
void BtnStateMachine_doState()
{
    assert(isInitialized);

    struct gpiod_line_bulk bulkEvents;
    int numEvents = Gpio_waitForLineChange(s_lineBtn, &bulkEvents);

    for (int i = 0; i < numEvents; i++)
    {
        
        struct gpiod_line *line_handle = gpiod_line_bulk_get_line(&bulkEvents, i);

        unsigned int this_line_number = gpiod_line_offset(line_handle);

        struct gpiod_line_event event;
        if (gpiod_line_event_read(line_handle, &event) == -1)
        {
            perror("Line Event");
            exit(EXIT_FAILURE);
        }

        bool isRising = event.event_type == GPIOD_LINE_EVENT_RISING_EDGE;

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

        if (pStateEvent->action != NULL)
        {
            pStateEvent->action();
        }
        pCurrentState = pStateEvent->pNextState;
    }
}
