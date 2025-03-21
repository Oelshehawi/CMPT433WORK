// Module for managing the LCD display
// Handles updating the display with current status information
#include "displayManager.h"
#include "hal/lcdDisplay.h"
#include "hal/audioMixer.h"
#include "beatPlayer.h"
#include "periodTimer.h"
#include <stdio.h>

// Initialize the display manager
void DisplayManager_init(void)
{
    LcdDisplay_init();
    printf("Display manager initialized\n");
}

// Clean up display manager resources
void DisplayManager_cleanup(void)
{
    LcdDisplay_cleanup();
}

// Update the LCD display with current system information
void DisplayManager_updateDisplay(void)
{
    // Get current beat mode, tempo, and volume
    BeatMode_t beatMode = BeatPlayer_getMode();
    int tempo = BeatPlayer_getTempo();
    int volume = AudioMixer_getVolume();

    // Get audio statistics
    Period_statistics_t audioStats;
    Period_getStatisticsAndClear(PERIOD_EVENT_AUDIO, &audioStats);

    // Get accelerometer statistics
    Period_statistics_t accelStats;
    Period_getStatisticsAndClear(PERIOD_EVENT_ACCEL, &accelStats);

    // Get beat mode name
    const char *modeName;
    switch (beatMode)
    {
    case BEAT_MODE_NONE:
        modeName = "NONE";
        break;
    case BEAT_MODE_ROCK:
        modeName = "ROCK";
        break;
    case BEAT_MODE_CUSTOM:
        modeName = "CUST";
        break;
    default:
        modeName = "????";
        break;
    }

    // Update LCD with current status for all screens
    LcdDisplay_updateStatus(modeName, volume, tempo);
    LcdDisplay_updateAudioTiming(audioStats.minPeriodInMs, audioStats.maxPeriodInMs, audioStats.avgPeriodInMs);
    LcdDisplay_updateAccelTiming(accelStats.minPeriodInMs, accelStats.maxPeriodInMs, accelStats.avgPeriodInMs);
}