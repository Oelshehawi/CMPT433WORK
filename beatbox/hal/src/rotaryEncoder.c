// This file is used to handle the rotary encoder
// and button press for the beatbox
#include "hal/rotaryEncoder.h"
#include "hal/buttonStateMachine.h"
#include "hal/gpio.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

// GPIO configuration for rotary encoder
#define BUTTON_GPIO_CHIP GPIO_CHIP_0
#define BUTTON_GPIO_LINE 10 // GPIO10 for button press on rotary encoder
#define CLK_GPIO_CHIP GPIO_CHIP_2
#define CLK_GPIO_LINE 7 // GPIO16 (Line A)
#define DT_GPIO_CHIP GPIO_CHIP_2
#define DT_GPIO_LINE 8 // GPIO17 (Line B)

// BPM limits and change amount
#define DEFAULT_BPM 120
#define MIN_BPM 40
#define MAX_BPM 300
#define BPM_CHANGE_AMOUNT 5
#define DEBOUNCE_TIME_MS 150 

// This is now shared with buttonStateMachine.c
BeatMode_t currentBeatMode = BEAT_MODE_ROCK;
static bool is_initialized = false;
static pthread_t encoder_thread;
static pthread_t button_thread;
static volatile bool keep_running = false;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// BPM variable and tracking
static int currentBPM = DEFAULT_BPM;
static long lastEncoderChangeTime = 0;

// Rotary encoder state variables
static struct GpioLine *clk_gpio = NULL;
static struct GpioLine *dt_gpio = NULL;

static long getCurrentTimeMs()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

// Thread function just for rotary encoder rotation
static void *encoder_rotation_thread_function(void *arg)
{
    (void)arg; 

    // Create the event monitoring structures
    struct gpiod_line_bulk encoderBulk;
    struct gpiod_line_bulk encoderEvents;
    gpiod_line_bulk_init(&encoderBulk);

    // Add only CLK and DT lines to the bulk for monitoring rotation
    if (clk_gpio != NULL && dt_gpio != NULL)
    {
        gpiod_line_bulk_add(&encoderBulk, (struct gpiod_line *)clk_gpio);
        gpiod_line_bulk_add(&encoderBulk, (struct gpiod_line *)dt_gpio);
    }
    else
    {
        printf("ERROR: Rotary encoder GPIO lines not initialized\n");
        return NULL;
    }

    int clk_last_state = -1;
    int dt_last_state = -1;
    bool first_reading = true;

    while (keep_running)
    {
        // Wait for events on either CLK or DT lines
        int result = gpiod_line_event_wait_bulk(&encoderBulk, NULL, &encoderEvents);
        if (result <= 0)
        {
            usleep(5000); 
            continue;
        }

        // Process the rotation events
        int numEvents = gpiod_line_bulk_num_lines(&encoderEvents);
        for (int i = 0; i < numEvents; i++)
        {
            struct gpiod_line *line = gpiod_line_bulk_get_line(&encoderEvents, i);
            struct gpiod_line_event event;
            unsigned int line_offset = gpiod_line_offset(line);

            if (gpiod_line_event_read(line, &event) != 0)
            {
                continue; 
            }

            // Only care about CLK and DT lines for rotation detection
            bool is_clk = (line_offset == CLK_GPIO_LINE);
            bool is_rising = (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE);

            if (is_clk)
            {
                clk_last_state = is_rising ? 1 : 0;
            }
            else
            {
                dt_last_state = is_rising ? 1 : 0;
            }

            if (first_reading)
            {
                first_reading = false;
                continue;
            }

            // Debounce - only process if enough time has passed
            long currentTime = getCurrentTimeMs();
            if (currentTime - lastEncoderChangeTime < DEBOUNCE_TIME_MS)
            {
                continue; 
            }

            // Detect rotation direction based on the sequence pattern
            // For clockwise: CLK falls before DT
            // For counter-clockwise: DT falls before CLK
            if (is_clk && !is_rising)
            { 
                if (dt_last_state == 1)
                { 
                    pthread_mutex_lock(&mutex);
                    currentBPM += BPM_CHANGE_AMOUNT;
                    if (currentBPM > MAX_BPM)
                    {
                        currentBPM = MAX_BPM;
                    }
                    pthread_mutex_unlock(&mutex);
                    printf("BPM increased to: %d\n", currentBPM);
                    lastEncoderChangeTime = currentTime;
                }
            }
            else if (!is_clk && !is_rising)
            { 
                if (clk_last_state == 1)
                { 
                    pthread_mutex_lock(&mutex);
                    currentBPM -= BPM_CHANGE_AMOUNT;
                    if (currentBPM < MIN_BPM)
                    {
                        currentBPM = MIN_BPM;
                    }
                    pthread_mutex_unlock(&mutex);
                    printf("BPM decreased to: %d\n", currentBPM);
                    lastEncoderChangeTime = currentTime;
                }
            }
        }
    }

    return NULL;
}

// Thread function for button press (mode changes)
static void *button_thread_function(void *arg)
{
    (void)arg; 

    while (keep_running)
    {
        BtnStateMachine_doState();

        usleep(5000); 
    }

    return NULL;
}

void RotaryEncoder_init(void)
{
    assert(!is_initialized);

    // Set up GPIO for the rotary encoder CLK and DT pins
    clk_gpio = Gpio_openForEvents(CLK_GPIO_CHIP, CLK_GPIO_LINE);
    dt_gpio = Gpio_openForEvents(DT_GPIO_CHIP, DT_GPIO_LINE);

    if (clk_gpio == NULL || dt_gpio == NULL)
    {
        printf("ERROR: Failed to initialize rotary encoder GPIO pins\n");
    }

    // Initialize BPM to default
    currentBPM = DEFAULT_BPM;
    lastEncoderChangeTime = getCurrentTimeMs();

    keep_running = true;

    if (pthread_create(&button_thread, NULL, button_thread_function, NULL) != 0)
    {
        perror("Failed to create button thread");
        exit(1);
    }

    if (pthread_create(&encoder_thread, NULL, encoder_rotation_thread_function, NULL) != 0)
    {
        perror("Failed to create encoder thread");
        exit(1);
    }

    is_initialized = true;
}

void RotaryEncoder_cleanup(void)
{
    if (!is_initialized)
        return;

    keep_running = false;

    usleep(100000); 

    // Join the threads to ensure they're fully stopped
    pthread_join(encoder_thread, NULL);
    pthread_join(button_thread, NULL);

    // Close GPIO resources
    if (clk_gpio != NULL)
    {
        Gpio_close(clk_gpio);
        clk_gpio = NULL;
    }

    if (dt_gpio != NULL)
    {
        Gpio_close(dt_gpio);
        dt_gpio = NULL;
    }

    is_initialized = false;
}

int RotaryEncoder_process(void)
{
    return 0; 
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

// Get current BPM
int RotaryEncoder_getBPM(void)
{
    int bpm;
    pthread_mutex_lock(&mutex);
    bpm = currentBPM;
    pthread_mutex_unlock(&mutex);
    return bpm;
}

// Set BPM with validation
void RotaryEncoder_setBPM(int bpm)
{
    if (bpm < MIN_BPM)
    {
        bpm = MIN_BPM;
    }
    else if (bpm > MAX_BPM)
    {
        bpm = MAX_BPM;
    }

    pthread_mutex_lock(&mutex);
    currentBPM = bpm;
    pthread_mutex_unlock(&mutex);
}