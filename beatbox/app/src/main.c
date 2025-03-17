#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "hal/audioMixer.h"
#include "drumSounds.h"
#include "beatPlayer.h"
#include "terminalDisplay.h"
#include "periodTimer.h"
#include "inputHandler.h"
#include "displayManager.h"

// Flag to indicate if the application should continue running
volatile bool isRunning = true;

// Signal handler for clean termination
static void sigHandler(int sig)
{
    if (sig == SIGINT || sig == SIGTERM)
    {
        printf("\nShutting down...\n");
        isRunning = false;
    }
}

int main(void)
{
    printf("BeatBox Application Starting...\n");

    // Set up signal handlers for clean termination
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);

    // Initialize Period module first (for timing statistics)
    Period_init();

    // Initialize audio mixer
    AudioMixer_init();
    printf("Audio mixer initialized\n");

    // Initialize drum sounds
    DrumSounds_init();
    printf("Drum sounds initialized\n");

    // Initialize beat player
    BeatPlayer_init();
    printf("Beat player initialized\n");

    // Initialize display manager first (LCD)
    DisplayManager_init();
    printf("Display initialized\n");

    // Initialize input handler
    InputHandler_init();
    printf("Input handler initialized\n");

    // Initialize terminal display last (after LCD is fully initialized)
    TerminalDisplay_init();
    printf("Terminal display initialized\n");

    // Default mode and settings
    BeatPlayer_setMode(BEAT_MODE_ROCK);
    BeatPlayer_setTempo(DEFAULT_BPM);

    // Start playback
    BeatPlayer_start();

    // Main loop timing
    struct timespec lastUpdateTime;
    clock_gettime(CLOCK_MONOTONIC, &lastUpdateTime);

    printf("Ready - press Ctrl+C to exit\n");

    // Main loop
    while (isRunning)
    {
        // Get current time
        struct timespec currentTime;
        clock_gettime(CLOCK_MONOTONIC, &currentTime);

        // Calculate elapsed time since last update
        long elapsedMs = (currentTime.tv_sec - lastUpdateTime.tv_sec) * 1000 +
                         (currentTime.tv_nsec - lastUpdateTime.tv_nsec) / 1000000;

        // Process joystick input much more frequently (every ~50ms)
        // This makes volume control much more responsive
        InputHandler_processJoystick();

        // Update display every second
        if (elapsedMs >= 1000)
        {
            // Update displays
            DisplayManager_updateDisplay();

            // Remember when we did the update
            lastUpdateTime = currentTime;
        }

        usleep(50000); // 50ms sleep - increased from 10ms
    }

    // Cleanup
    printf("Cleaning up...\n");

    InputHandler_cleanup();
    DisplayManager_cleanup();
    TerminalDisplay_cleanup();
    BeatPlayer_cleanup();
    DrumSounds_cleanup();
    AudioMixer_cleanup();

    printf("BeatBox Application terminated.\n");
    return 0;
}