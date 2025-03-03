#include "hal/rotary_encoder.h"
#include "hal/gpio.h"
#include "hal/udp_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>

// Pin config info for newer board:
// Try GPIO16 and GPIO17 on gpiochip2
#define GPIO_CHIP GPIO_CHIP_2 // Changed to gpiochip2
#define GPIO_LINE_A 7         // GPIO16 is on line 7
#define GPIO_LINE_B 8         // GPIO17 is on line 8

static bool is_initialized = false;
static struct GpioLine *s_lineA = NULL;
static struct GpioLine *s_lineB = NULL;
static pthread_t encoder_thread;
static volatile bool keep_running = true;
static volatile int last_direction = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// State machine states and events
struct stateEvent
{
    struct state *pNextState;
    void (*action)(void);
};

struct state
{
    struct stateEvent a_rising;
    struct stateEvent a_falling;
    struct stateEvent b_rising;
    struct stateEvent b_falling;
};

static void on_clockwise(void)
{
    pthread_mutex_lock(&mutex);
    last_direction = 1; // Clockwise
    pthread_mutex_unlock(&mutex);
}

static void on_counter_clockwise(void)
{
    pthread_mutex_lock(&mutex);
    last_direction = -1; // Counter-clockwise
    pthread_mutex_unlock(&mutex);
}

// Define the state machine states
static struct state states[] = {
    // STATE_REST (0)
    {
        .a_falling = {&states[1], NULL}, // Go to CW_BEGIN
        .b_falling = {&states[3], NULL}, // Go to CCW_BEGIN
        .a_rising = {&states[0], NULL},  // Stay in REST
        .b_rising = {&states[0], NULL},  // Stay in REST
    },
    // STATE_CW_BEGIN (1)
    {
        .b_falling = {&states[2], NULL}, // Go to CW_FINAL
        .a_rising = {&states[0], NULL},  // Back to REST
        .a_falling = {&states[1], NULL}, // Stay in CW_BEGIN
        .b_rising = {&states[0], NULL},  // Back to REST
    },
    // STATE_CW_FINAL (2)
    {
        .a_rising = {&states[0], on_clockwise}, // Complete CW rotation
        .a_falling = {&states[0], NULL},        // Back to REST
        .b_rising = {&states[0], NULL},         // Back to REST
        .b_falling = {&states[2], NULL},        // Stay in CW_FINAL
    },
    // STATE_CCW_BEGIN (3)
    {
        .a_falling = {&states[4], NULL}, // Go to CCW_FINAL
        .b_rising = {&states[0], NULL},  // Back to REST
        .a_rising = {&states[0], NULL},  // Back to REST
        .b_falling = {&states[3], NULL}, // Stay in CCW_BEGIN
    },
    // STATE_CCW_FINAL (4)
    {
        .b_rising = {&states[0], on_counter_clockwise}, // Complete CCW rotation
        .b_falling = {&states[4], NULL},                // Stay in CCW_FINAL
        .a_rising = {&states[0], NULL},                 // Back to REST
        .a_falling = {&states[0], NULL},                // Back to REST
    },
};

static struct state *pCurrentState = &states[0];

static void *encoder_thread_function()
{
    struct timespec timeout;
    timeout.tv_sec = 0;
    timeout.tv_nsec = 100000000; // 100ms timeout

    while (keep_running && !UdpServer_shouldStop())
    {
        // Check stop conditions before accessing GPIO
        if (!keep_running || UdpServer_shouldStop())
        {
            break;
        }

        struct gpiod_line_bulk bulkEvents;
        struct gpiod_line_bulk bulk;
        gpiod_line_bulk_init(&bulk);

        // Check stop conditions before continuing
        if (!keep_running || UdpServer_shouldStop())
        {
            break;
        }

        gpiod_line_bulk_add(&bulk, (struct gpiod_line *)s_lineA);
        gpiod_line_bulk_add(&bulk, (struct gpiod_line *)s_lineB);

        // Request both edges events for both lines
        gpiod_line_request_bulk_both_edges_events(&bulk, "Rotary Events");

        // Use timeout to avoid blocking forever during shutdown
        int result = gpiod_line_event_wait_bulk(&bulk, &timeout, &bulkEvents);

        // Check if we should exit
        if (!keep_running || UdpServer_shouldStop())
        {
            // Release any bulk events properly before exiting
            gpiod_line_release((struct gpiod_line *)s_lineA);
            gpiod_line_release((struct gpiod_line *)s_lineB);
            break;
        }

        if (result <= 0)
        {
            continue;
        }

        int numEvents = gpiod_line_bulk_num_lines(&bulkEvents);

        // Process each event
        for (int i = 0; i < numEvents; i++)
        {
            // Check stop conditions before processing each event
            if (!keep_running || UdpServer_shouldStop())
            {
                break;
            }

            struct gpiod_line *line = gpiod_line_bulk_get_line(&bulkEvents, i);
            unsigned int line_number = gpiod_line_offset(line);

            struct gpiod_line_event event;
            if (gpiod_line_event_read(line, &event) == -1)
            {
                continue;
            }

            bool isRising = event.event_type == GPIOD_LINE_EVENT_RISING_EDGE;
            bool isA = line_number == GPIO_LINE_A;

            // Determine which state event to use based on the input
            struct stateEvent *pStateEvent;
            if (isA)
            {
                pStateEvent = isRising ? &pCurrentState->a_rising : &pCurrentState->a_falling;
            }
            else
            {
                pStateEvent = isRising ? &pCurrentState->b_rising : &pCurrentState->b_falling;
            }

            // Execute the state transition
            if (pStateEvent->action)
            {
                pStateEvent->action();
            }
            pCurrentState = pStateEvent->pNextState;
        }
    }

    // If UDP server requested stop, update our flag too
    if (UdpServer_shouldStop())
    {
        keep_running = false;
    }

    return NULL;
}

void RotaryEncoder_init(void)
{
    assert(!is_initialized);

    // Open GPIO lines for rotary encoder
    s_lineA = Gpio_openForEvents(GPIO_CHIP, GPIO_LINE_A);
    if (!s_lineA)
    {
        perror("Failed to open GPIO line A");
        exit(1);
    }

    s_lineB = Gpio_openForEvents(GPIO_CHIP, GPIO_LINE_B);
    if (!s_lineB)
    {
        perror("Failed to open GPIO line B");
        Gpio_close(s_lineA);
        exit(1);
    }

    // Start the encoder thread
    keep_running = true;
    if (pthread_create(&encoder_thread, NULL, encoder_thread_function, NULL) != 0)
    {
        perror("Failed to create encoder thread");
        Gpio_close(s_lineA);
        Gpio_close(s_lineB);
        exit(1);
    }

    is_initialized = true;
}

void RotaryEncoder_cleanup(void)
{
    if (!is_initialized)
        return;

    // Stop the thread
    keep_running = false;

    usleep(200000); // 200ms

    // Join the thread to ensure it's fully stopped
    pthread_join(encoder_thread, NULL);
    printf("Rotary Encoder - Thread joined\n");

    if (s_lineA)
    {
        Gpio_close(s_lineA);
        s_lineA = NULL;
    }
    if (s_lineB)
    {
        Gpio_close(s_lineB);
        s_lineB = NULL;
    }

    is_initialized = false;
    printf("Rotary Encoder - Cleanup complete\n");
}

int RotaryEncoder_process(void)
{
    pthread_mutex_lock(&mutex);
    int direction = last_direction;
    last_direction = 0; // Reset after reading
    pthread_mutex_unlock(&mutex);
    return direction;
}