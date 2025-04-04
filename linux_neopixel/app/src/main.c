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
#include "hal/firing_input.h"
#include "hal/shutdown_input.h"
#include "lcd_display_impl.h"
#include "target_game.h"
#include "memory_handler.h"

#define MAX_LCD_MESSAGE 1024
#define NAME "Omar n Wes"

// Signal handler control
static volatile bool should_exit = false;

// Last time the LCD was updated
static unsigned long lastLcdUpdateTime = 0;

// Signal handler for clean exit
void handle_signal(int sig)
{
    printf("\nReceived signal %d, cleaning up...\n", sig);
    should_exit = true;
}

// Helper function to format time as MM:SS
static void formatTime(unsigned long timeMs, char *buffer, size_t bufferSize)
{
    unsigned long totalSeconds = timeMs / 1000;
    unsigned int minutes = totalSeconds / 60;
    unsigned int seconds = totalSeconds % 60;

    if (minutes < 60)
    {
        // Format as MM:SS
        snprintf(buffer, bufferSize, "%02u:%02u", minutes, seconds);
    }
    else
    {
        // Format as HH:MM:SS for longer durations
        unsigned int hours = minutes / 60;
        minutes %= 60;
        snprintf(buffer, bufferSize, "%u:%02u:%02u", hours, minutes, seconds);
    }
}

// Helper function to get current time in milliseconds
static unsigned long getCurrentTimeMs(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

// Helper function to update the LCD with current status
static void update_display(void)
{
    char buff[MAX_LCD_MESSAGE];
    char timeStr[20];
    int hits, misses;
    unsigned long runTimeMs;

    // Get current game stats
    TargetGame_getStats(&hits, &misses, &runTimeMs);

    // Format the runtime
    formatTime(runTimeMs, timeStr, sizeof(timeStr));

    // Format the LCD message
    snprintf(buff, MAX_LCD_MESSAGE,
             "%s\n"
             "Find the Dot Game\n"
             "Hits:%d Misses:%d\n"
             "Time: %s",
             NAME, hits, misses, timeStr);

    // Update the display
    LcdDisplayImpl_update(buff);

    // Remember when we last updated the LCD
    lastLcdUpdateTime = getCurrentTimeMs();
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

    // Initialize firing input (button)
    if (!FiringInput_init())
    {
        printf("Failed to initialize firing input. Exiting.\n");
        Accelerometer_cleanup();
        LcdDisplayImpl_cleanup();
        Gpio_cleanup();
        return EXIT_FAILURE;
    }

    // Initialize shutdown input (joystick center button)
    if (!ShutdownInput_init())
    {
        printf("Failed to initialize shutdown input. Exiting.\n");
        FiringInput_cleanup();
        Accelerometer_cleanup();
        LcdDisplayImpl_cleanup();
        Gpio_cleanup();
        return EXIT_FAILURE;
    }

    // Initialize shared memory
    if (!MemoryHandler_init())
    {
        printf("Failed to setup shared memory. Exiting.\n");
        ShutdownInput_cleanup();
        FiringInput_cleanup();
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

    // Initialize LCD update time
    lastLcdUpdateTime = getCurrentTimeMs();

    // Set initial delay
    uint32_t delay_ms = 20; // 20ms delay for more responsive controls and animations

    // Main loop
    while (!should_exit)
    {
        // Get current time
        unsigned long currentTime = getCurrentTimeMs();

        // Check if shutdown was requested via joystick button
        if (ShutdownInput_isShutdownRequested())
        {
            printf("Shutdown requested via joystick press.\n");
            should_exit = true; // Signal loop to terminate
        }

        // Read accelerometer data
        int16_t rawX, rawY, rawZ;
        float pointingX = 0.0f, pointingY = 0.0f;

        Accelerometer_readRaw(&rawX, &rawY, &rawZ);

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

        // Debug print (less frequently to reduce console spam)
        static int debugCounter = 0;
        if (debugCounter++ % 50 == 0)
        { // Print only every 50th iteration
            printf("Pointing: X=%.2f, Y=%.2f (Raw: X=%d, Y=%d, Z=%d)\n",
                   pointingX, pointingY, rawX, rawY, rawZ);
        }

        // Create buffer for LED colors
        uint32_t output_colors[NEO_NUM_LEDS] = {0};

        // Track if we need to update the LCD immediately
        bool needLcdUpdate = false;

        // Check if button was pressed (firing input)
        if (FiringInput_wasButtonPressed())
        {
            // Process the firing action
            TargetGame_fire(pointingX, pointingY);
            needLcdUpdate = true; // Update LCD immediately on hit/miss
        }

        // First try to update animations
        bool animationActive = TargetGame_updateAnimations(output_colors, NEO_NUM_LEDS);

        // If no animation is active, process regular pointing
        if (!animationActive)
        {
            TargetGame_processPointing(pointingX, pointingY, output_colors, NEO_NUM_LEDS);
        }

        // Write the colors to shared memory
        MemoryHandler_writeColors(output_colors, NEO_NUM_LEDS);

        // Update the display if needed (on event or every second)
        if (needLcdUpdate || (currentTime - lastLcdUpdateTime >= 1000))
        {
            update_display();
        }

        // Sleep for the specified delay
        usleep(delay_ms * 1000);
    }

    printf("Starting cleanup...\n");

    // Turn off NeoPixels before cleaning up shared memory
    printf("Turning off NeoPixels...\n");
    uint32_t off_colors[NEO_NUM_LEDS] = {0}; // Array of zeros
    MemoryHandler_writeColors(off_colors, NEO_NUM_LEDS);
    usleep(50000); // Give R5 time to process the 'off' frame (50ms)

    // Clean up in reverse order of initialization
    printf("Cleaning up shared memory...\n");
    MemoryHandler_cleanup();

    printf("Stopping shutdown input...\n");
    ShutdownInput_cleanup();

    printf("Stopping firing input...\n");
    FiringInput_cleanup();

    printf("Stopping accelerometer...\n");
    Accelerometer_cleanup();

    printf("Stopping LCD display...\n");
    LcdDisplayImpl_cleanup();

    // Clean up GPIO last
    printf("Stopping GPIO...\n");
    Gpio_cleanup();

    printf("Cleanup complete. Program terminated.\n");
    return 0;
}