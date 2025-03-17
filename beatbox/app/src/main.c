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
#include "udpServer.h"

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

    Period_init();
    AudioMixer_init();
    DrumSounds_init();
    BeatPlayer_init();
    DisplayManager_init();
    InputHandler_init();
    TerminalDisplay_init();
    UdpServer_init();

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
        // Check if UDP server requested shutdown
        if (UdpServer_shouldStop())
        {
            printf("Shutdown requested via UDP\n");
            isRunning = false;
            break;
        }

        // Get current time
        struct timespec currentTime;
        clock_gettime(CLOCK_MONOTONIC, &currentTime);

        // Calculate elapsed time since last update
        long elapsedMs = (currentTime.tv_sec - lastUpdateTime.tv_sec) * 1000 +
                         (currentTime.tv_nsec - lastUpdateTime.tv_nsec) / 1000000;

        InputHandler_processJoystick();

        // Update display every second
        if (elapsedMs >= 1000)
        {
            // Update displays
            DisplayManager_updateDisplay();

            // Remember when we did the update
            lastUpdateTime = currentTime;
        }

        usleep(50000); // 50ms sleep
    }

    // Cleanup
    printf("Cleaning up...\n");

    UdpServer_cleanup();
    InputHandler_cleanup();
    DisplayManager_cleanup();
    TerminalDisplay_cleanup();
    BeatPlayer_cleanup();
    DrumSounds_cleanup();
    AudioMixer_cleanup();

    printf("BeatBox Application terminated.\n");
    return 0;
}