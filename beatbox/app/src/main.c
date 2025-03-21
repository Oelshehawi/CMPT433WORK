/*
 * main.c
 * 
 * Main control file for the BeatBox project
 * 
 * Key responsibilities:
 * - System initialization (audio, controls, display)
 * - Main program loop management
 * - Input processing (rotary encoder, buttons, accelerometer)
 * - Shutdown and cleanup
 * 
 * The program runs until receiving a shutdown signal (Ctrl+C) or UDP command,
 * then ensures all components are properly cleaned up before exit.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "hal/audioMixer.h"
#include "hal/rotaryEncoder.h"
#include "hal/buttonStateMachine.h"
#include "hal/gpio.h"
#include "drumSounds.h"
#include "beatPlayer.h"
#include "terminalDisplay.h"
#include "periodTimer.h"
#include "inputHandler.h"
#include "displayManager.h"
#include "udpServer.h"
#include "accelerometer.h"

// Flag to indicate if the application should continue running
volatile bool isRunning = true;

// Signal handler for clean termination
static void sigHandler(int sig)
{
    if (sig == SIGINT || sig == SIGTERM)
    {
        printf("\nReceived termination signal. Shutting down...\n");
        isRunning = false;

        // Update terminal display about shutdown right away
        TerminalDisplay_registerShutdown(&isRunning);

        fflush(stdout);

        usleep(100000); 
    }
}

int main(void)
{
    printf("BeatBox Application Starting...\n");

    // Set up signal handlers for clean termination
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);

    // For tracking mode changes
    BeatMode_t lastEncoderMode = BEAT_MODE_ROCK;

    // Initialize components in correct order
    Gpio_initialize();
    Period_init();
    AudioMixer_init();
    DrumSounds_init();
    BeatPlayer_init();
    DisplayManager_init();
    InputHandler_init();
    TerminalDisplay_init();
    TerminalDisplay_registerShutdown(&isRunning);
    UdpServer_init();

    // Initialize the button state machine and rotary encoder
    BtnStateMachine_init();
    RotaryEncoder_init();

    // Initialize accelerometer
    AccelerometerApp_init();

    // Default mode and settings
    BeatPlayer_setMode(BEAT_MODE_ROCK);
    BeatPlayer_setTempo(DEFAULT_BPM);

    // Start playback
    BeatPlayer_start();

    // Main loop timing
    struct timespec lastUpdateTime;
    clock_gettime(CLOCK_MONOTONIC, &lastUpdateTime);

    printf("Ready - press Ctrl+C to exit\n");

    // Set up a watchdog timeout for forced exit
    time_t start_time = time(NULL);
    const int MAX_RUNTIME = 3600; 

    while (isRunning)
    {
        // Check if UDP server requested shutdown
        if (UdpServer_shouldStop())
        {
            printf("Shutdown requested via UDP\n");
            isRunning = false;

            TerminalDisplay_registerShutdown(&isRunning);

            usleep(100000); 

            break;
        }

        // Get current time
        struct timespec currentTime;
        clock_gettime(CLOCK_MONOTONIC, &currentTime);

        // Calculate elapsed time since last update
        long elapsedMs = (currentTime.tv_sec - lastUpdateTime.tv_sec) * 1000 +
                         (currentTime.tv_nsec - lastUpdateTime.tv_nsec) / 1000000;

        // Get current modes for both systems
        BeatMode_t encoderMode = RotaryEncoder_getBeatMode();
        BeatMode_t playerMode = BeatPlayer_getMode();

        if (encoderMode != playerMode)
        {
            if (encoderMode != lastEncoderMode)
            {
                BeatPlayer_setMode(encoderMode);
            }
            else
            {
                extern BeatMode_t currentBeatMode; 
                currentBeatMode = playerMode;
            }
        }

        lastEncoderMode = encoderMode;

        // Process all input devices
        int encoderBPM = RotaryEncoder_getBPM();
        BeatPlayer_setTempo(encoderBPM);
        AccelerometerApp_process();

        // Update display every second
        if (elapsedMs >= 1000)
        {
            DisplayManager_updateDisplay();
            lastUpdateTime = currentTime;
        }

        usleep(50000); 

        // Check if the application has been running for too long
        time_t current_time = time(NULL);
        if (difftime(current_time, start_time) > MAX_RUNTIME)
        {
            printf("Application running for too long. Forcing shutdown.\n");
            isRunning = false;
            break;
        }
    }

    // Cleanup in reverse order of initialization
    printf("Cleaning up...\n");

    // Stop the beat player first to avoid audio glitches during cleanup
    BeatPlayer_stop();

    RotaryEncoder_cleanup();
    BtnStateMachine_cleanup();
    UdpServer_cleanup();
    InputHandler_cleanup();
    DisplayManager_cleanup();
    TerminalDisplay_cleanup();
    BeatPlayer_cleanup();
    DrumSounds_cleanup();
    AudioMixer_cleanup();
    AccelerometerApp_cleanup();
    Gpio_cleanup();

    printf("\033[0m"); 
    printf("\nBeatBox Application terminated.\n");
    return 0;
}