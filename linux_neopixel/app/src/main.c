// Main program for NeoPixel control application
// Has main(); does initialization and cleanup
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#include "sharedDataLayout.h"
#include "hal/gpio.h"
#include "hal/accelerometer.h"
#include "lcd_display_impl.h"
#include "target_game.h"
#include "memory_handler.h"

#define MAX_LCD_MESSAGE 1024
#define NAME "Omar n Wes"

// Signal handler control
static volatile bool should_exit = false;

// Signal handler for clean exit
void handle_signal(int sig)
{
    printf("\nReceived signal %d, cleaning up...\n", sig);
    should_exit = true;
}

// Helper function to update the LCD with current status
static void update_display(void)
{
    // For now, just display a static message
    char buff[MAX_LCD_MESSAGE];
    snprintf(buff, MAX_LCD_MESSAGE,
             "%s\n"
             "Find the Dot Game\n"
             "Tilt to aim!\n"
             "Running...",
             NAME);

    LcdDisplayImpl_update(buff);
}

int main()
{
    // Set up signal handler for Ctrl+C
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    printf("Starting Find the Dot Game...\n");

    // Initialize modules we need
    Gpio_initialize();
    LcdDisplayImpl_init();

    // Initialize accelerometer
    if (!Accelerometer_init())
    {
        printf("Failed to initialize accelerometer. Exiting.\n");
        LcdDisplayImpl_cleanup();
        Gpio_cleanup();
        return EXIT_FAILURE;
    }
    printf("Accelerometer initialized successfully\n");

    // Initialize shared memory
    if (!MemoryHandler_init())
    {
        printf("Failed to setup shared memory. Exiting.\n");
        Accelerometer_cleanup();
        LcdDisplayImpl_cleanup();
        Gpio_cleanup();
        return EXIT_FAILURE;
    }
    printf("Shared memory initialized successfully\n");

    // Initialize target game
    TargetGame_init();
    printf("Game initialized. Find the target!\n");
    printf("Press Ctrl+C to exit\n");

    // Set initial delay
    uint32_t delay_ms = 50; // 50ms delay for more responsive controls

    // Main loop
    while (!should_exit)
    {
        // Read accelerometer data
        int16_t rawX, rawY, rawZ;
        float pointingX = 0.0f, pointingY = 0.0f;

        Accelerometer_readRaw(&rawX, &rawY, &rawZ);

        // --- Process Accelerometer Data ---
        // Mapping based on experiment:
        // Pointing X (Left/Right) corresponds to raw Y
        // Pointing Y (Down/Up) corresponds to NEGATED raw X
        const float SCALING_FACTOR = 16384.0f; // Adjust if needed!

        pointingX = (float)rawY / SCALING_FACTOR;  // Left/Right uses rawY
        pointingY = -(float)rawX / SCALING_FACTOR; // Down/Up uses inverted rawX

        // Clamp values to the expected range [-1.0, 1.0]
        if (pointingX > 1.0f)
            pointingX = 1.0f;
        if (pointingX < -1.0f)
            pointingX = -1.0f;
        if (pointingY > 1.0f)
            pointingY = 1.0f;
        if (pointingY < -1.0f)
            pointingY = -1.0f;

        // Debug print
        printf("Pointing: X=%.2f, Y=%.2f (Raw: X=%d, Y=%d, Z=%d)\n",
               pointingX, pointingY, rawX, rawY, rawZ);

        // Process pointing data and update LEDs
        uint32_t output_colors[NEO_NUM_LEDS] = {0};
        bool targetHit = TargetGame_processPointing(pointingX, pointingY, output_colors, NEO_NUM_LEDS);

        // Write the colors to shared memory
        MemoryHandler_writeColors(output_colors, NEO_NUM_LEDS);

        // If target hit, generate a new target after a short delay
        if (targetHit)
        {
            printf("Target hit! Generating new target...\n");
            usleep(1000000); // 1 second delay to celebrate
            TargetGame_generateNewTarget();
        }

        // Update the display
        update_display();

        // Sleep for the specified delay
        usleep(delay_ms * 1000);
    }

    printf("Starting cleanup...\n");

    printf("Stopping LCD display...\n");
    LcdDisplayImpl_cleanup();

    printf("Cleaning up shared memory...\n");
    MemoryHandler_cleanup();

    printf("Stopping accelerometer...\n");
    Accelerometer_cleanup();

    printf("Stopping GPIO...\n");
    Gpio_cleanup();

    printf("Cleanup complete. Program terminated.\n");
    return 0;
}