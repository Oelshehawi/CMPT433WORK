// Module for handling periodic display updates on the console
// Manages timing statistics and handles the periodical output of system status
#include "terminalDisplay.h"
#include "periodTimer.h"
#include "hal/audioMixer.h"
#include "beatPlayer.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

// Update the console status every second
#define CONSOLE_UPDATE_MS 1000

// Thread variables
static pthread_t displayThreadId;
static volatile bool isRunning = false;     
static volatile bool isShuttingDown = false; 
// Private function declarations
static void *displayThread(void *arg);
static void updateConsoleOutput(void);

// Register with the app-wide shutdown flag
void TerminalDisplay_registerShutdown(volatile bool *appIsRunning)
{
    isShuttingDown = !(*appIsRunning);
}

// Display thread function
static void *displayThread(void *arg)
{
    (void)arg;

    // Thread timing variables
    struct timespec lastConsoleUpdate;
    clock_gettime(CLOCK_MONOTONIC, &lastConsoleUpdate);

    while (isRunning && !isShuttingDown)
    {
        struct timespec currentTime;
        clock_gettime(CLOCK_MONOTONIC, &currentTime);

        // Calculate time since last console update in milliseconds
        long elapsedMs = (currentTime.tv_sec - lastConsoleUpdate.tv_sec) * 1000 +
                         (currentTime.tv_nsec - lastConsoleUpdate.tv_nsec) / 1000000;

        // Update console output every second
        if (elapsedMs >= CONSOLE_UPDATE_MS)
        {
            updateConsoleOutput();
            lastConsoleUpdate = currentTime;
        }

        usleep(10000);
    }

    Period_cleanup();

    printf("Terminal display thread exited\n");
    return NULL;
}

// Update the console with periodic statistics
static void updateConsoleOutput(void)
{
    if (isShuttingDown)
    {
        return;
    }

    // Get audio statistics
    Period_statistics_t audioStats;
    Period_getStatisticsAndClear(PERIOD_EVENT_AUDIO, &audioStats);

    // Get accelerometer statistics
    Period_statistics_t accelStats;
    Period_getStatisticsAndClear(PERIOD_EVENT_ACCEL, &accelStats);

    // Get current beat mode, tempo, and volume
    BeatMode_t beatMode = BeatPlayer_getMode();
    int tempo = BeatPlayer_getTempo();
    int volume = AudioMixer_getVolume();

    // Format the beat mode as "M<number>"
    char modeStr[8];
    sprintf(modeStr, "M%d", (int)beatMode);

    // Output formatting
    printf("%s %dbpm vol:%d Audio[%.3f, %.3f] avg %.3f/%d Accel[%.3f, %.3f] avg %.3f/%d\n",
           modeStr,
           tempo,
           volume,
           audioStats.minPeriodInMs,
           audioStats.maxPeriodInMs,
           audioStats.avgPeriodInMs,
           audioStats.numSamples,
           accelStats.minPeriodInMs,
           accelStats.maxPeriodInMs,
           accelStats.avgPeriodInMs,
           accelStats.numSamples);

    fflush(stdout);
}


void TerminalDisplay_init(void)
{
    isRunning = true;
    isShuttingDown = false;
    pthread_create(&displayThreadId, NULL, displayThread, NULL);

    printf("Terminal display initialized\n");
}

void TerminalDisplay_cleanup(void)
{
    printf("Terminal display cleanup starting...\n");

    if (isRunning)
    {
        isRunning = false;
        isShuttingDown = true;

        usleep(50000); 

        // Join with standard pthread_join
        int join_result = pthread_join(displayThreadId, NULL);
        if (join_result != 0)
        {
            printf("WARNING: Terminal display thread join failed\n");
        }
        else
        {
            printf("Terminal display thread joined successfully\n");
        }
    }

    printf("Terminal display cleanup complete\n");
}

void TerminalDisplay_markAudioEvent(void)
{
    Period_markEvent(PERIOD_EVENT_AUDIO);
}

void TerminalDisplay_markAccelerometerEvent(void)
{
    Period_markEvent(PERIOD_EVENT_ACCEL);
}