#include "hal/btn_statemachine.h"
#include "hal/gpio.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

// Pin config info: GPIO 24 is on gpiochip0 line 10
#define GPIO_CHIP GPIO_CHIP_0
#define GPIO_LINE_NUMBER 10

static bool is_initialized = false;
static struct GpioLine *s_lineBtn = NULL;
static int counter = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t state_thread;
static volatile bool keep_running = true;

/*
    Define the State Machine Data Structures
*/
struct stateEvent
{
    struct state *pNextState;
    void (*action)(void);
};

struct state
{
    struct stateEvent rising;
    struct stateEvent falling;
};

/*
    START STATE MACHINE
*/
static void on_release(void)
{
    pthread_mutex_lock(&mutex);
    counter++;
    pthread_mutex_unlock(&mutex);
}

static struct state states[] = {
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

static struct state *pCurrentState = &states[0];

static void *state_machine_thread()
{
    while (keep_running)
    {
        struct gpiod_line_bulk bulkEvents;

        int numEvents = Gpio_waitForLineChange(s_lineBtn, &bulkEvents);
        if (numEvents <= 0)
        {
            continue; // Error
        }

        // Iterate over the events
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

            // Run the state machine
            bool isRising = event.event_type == GPIOD_LINE_EVENT_RISING_EDGE;
            bool isBtn = this_line_number == GPIO_LINE_NUMBER;
            assert(isBtn);

            struct stateEvent *pStateEvent = isRising ? &pCurrentState->rising : &pCurrentState->falling;

            // Do the action
            if (pStateEvent->action != NULL)
            {
                pStateEvent->action();
            }
            pCurrentState = pStateEvent->pNextState;
        }
    }
    return NULL;
}

void BtnStateMachine_init(void)
{
    assert(!is_initialized);

    // Initialize GPIO
    s_lineBtn = Gpio_openForEvents(GPIO_CHIP, GPIO_LINE_NUMBER);
    if (!s_lineBtn)
    {
        perror("Failed to open GPIO line");
        exit(1);
    }

    // Start the state machine thread
    keep_running = true;
    if (pthread_create(&state_thread, NULL, state_machine_thread, NULL) != 0)
    {
        perror("Failed to create state machine thread");
        Gpio_close(s_lineBtn);
        exit(1);
    }

    is_initialized = true;
}

void BtnStateMachine_cleanup(void)
{
    if (!is_initialized)
        return;

    // Stop the thread
    keep_running = false;
    pthread_join(state_thread, NULL);

    // Cleanup GPIO
    if (s_lineBtn)
    {
        Gpio_close(s_lineBtn);
        s_lineBtn = NULL;
    }

    is_initialized = false;
}

int BtnStateMachine_getValue(void)
{
    pthread_mutex_lock(&mutex);
    int value = counter;
    pthread_mutex_unlock(&mutex);
    return value;
}