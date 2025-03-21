// This file is used to display the LCD screen
// and update the status, audio timing, and accelerometer timing
#include "hal/lcdDisplay.h"
#include "DEV_Config.h"
#include "LCD_1in54.h"
#include "GUI_Paint.h"
#include "GUI_BMP.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h> 

// Display buffer
static UWORD *s_frameBuffer = NULL;
static bool isInitialized = false;

// Current screen type
static LcdScreenType currentScreen = LCD_SCREEN_STATUS;

// Status screen data
static char beatName[32] = "None";
static int currentVolume = 80;
static int currentBpm = 120;

// Audio timing data
static double audioMinMs = 0.0;
static double audioMaxMs = 0.0;
static double audioAvgMs = 0.0;

// Accel timing data
static double accelMinMs = 0.0;
static double accelMaxMs = 0.0;
static double accelAvgMs = 0.0;

// Initialize the LCD display
void LcdDisplay_init(void)
{
    if (isInitialized)
        return;

    printf("LCD: Starting initialization...\n");

    if (DEV_ModuleInit() != 0)
    {
        fprintf(stderr, "LCD: Module initialization failed\n");
        DEV_ModuleExit();
        return;
    }

    // LCD initialization
    printf("LCD: Initializing display...\n");
    DEV_Delay_ms(2000);
    LCD_1IN54_Init(HORIZONTAL);
    LCD_1IN54_Clear(WHITE);
    LCD_SetBacklight(1023);

    // Allocate frame buffer
    UDOUBLE imageSize = LCD_1IN54_HEIGHT * LCD_1IN54_WIDTH * 2;
    s_frameBuffer = (UWORD *)malloc(imageSize);
    if (s_frameBuffer == NULL)
    {
        fprintf(stderr, "LCD: Failed to allocate LCD frame buffer memory\n");
        DEV_ModuleExit();
        return;
    }

    isInitialized = true;
    printf("LCD: Initialization complete\n");

    LcdDisplay_updateStatus("ROCK", 80, 120);
}

void LcdDisplay_cleanup(void)
{
    if (!isInitialized)
        return;

    printf("LCD: Cleaning up resources...\n");

    if (s_frameBuffer != NULL)
    {
        free(s_frameBuffer);
        s_frameBuffer = NULL;
    }

    DEV_ModuleExit();
    isInitialized = false;
    printf("LCD: Cleanup complete\n");
}

// Set debug output mode
void LcdDisplay_setDebug(bool enable)
{
    (void)enable;

}

// Switch to the next screen (cyclic)
void LcdDisplay_nextScreen(void)
{
    if (!isInitialized)
        return;

    // Cycle through screens
    currentScreen = (currentScreen + 1) % LCD_SCREEN_COUNT;

    // Refresh the display with new content
    LcdDisplay_refresh();

    printf("LCD: Switched to screen %d\n", currentScreen);
}

// Get the current screen type
LcdScreenType LcdDisplay_getCurrentScreen(void)
{
    return currentScreen;
}

// Update the status screen information
void LcdDisplay_updateStatus(const char *name, int volume, int bpm)
{
    if (!isInitialized)
        return;

    // Update the state
    strncpy(beatName, name, sizeof(beatName) - 1);
    beatName[sizeof(beatName) - 1] = '\0';
    currentVolume = volume;
    currentBpm = bpm;

    // Only refresh if we're on the status screen
    if (currentScreen == LCD_SCREEN_STATUS)
    {
        LcdDisplay_refresh();
    }
}

// Update audio timing information
void LcdDisplay_updateAudioTiming(double min, double max, double avg)
{
    if (!isInitialized)
        return;

    audioMinMs = min;
    audioMaxMs = max;
    audioAvgMs = avg;

    // Only refresh if we're on the audio timing screen
    if (currentScreen == LCD_SCREEN_AUDIO_TIMING)
    {
        LcdDisplay_refresh();
    }
}

// Update accelerometer timing information
void LcdDisplay_updateAccelTiming(double min, double max, double avg)
{
    if (!isInitialized)
        return;

    accelMinMs = min;
    accelMaxMs = max;
    accelAvgMs = avg;

    // Only refresh if we're on the accelerometer timing screen
    if (currentScreen == LCD_SCREEN_ACCEL_TIMING)
    {
        LcdDisplay_refresh();
    }
}

// Render the status screen
static void render_status_screen(void)
{
    // Draw the title 
    Paint_DrawString_EN(5, 25, "BeatBox - Status", &Font20, BLACK, WHITE);

    // Center and draw beat name with larger font
    int center_x = (LCD_1IN54_WIDTH - strlen(beatName) * 16) / 2;
    if (center_x < 0)
        center_x = 5;
    Paint_DrawString_EN(center_x, 65, beatName, &Font24, BLACK, WHITE);

    // Format volume and BPM strings
    char volumeStr[20], bpmStr[20];
    snprintf(volumeStr, sizeof(volumeStr), "Vol: %d", currentVolume);
    snprintf(bpmStr, sizeof(bpmStr), "BPM: %d", currentBpm);

    // Draw volume in bottom left
    Paint_DrawString_EN(5, LCD_1IN54_HEIGHT - 30, volumeStr, &Font16, BLACK, WHITE);

    // Draw BPM in bottom right
    int bpm_width = strlen(bpmStr) * 10;
    int bpm_x = LCD_1IN54_WIDTH - bpm_width - 10;

    if (bpm_x < LCD_1IN54_WIDTH / 2 + 10)
        bpm_x = LCD_1IN54_WIDTH / 2 + 10;

    Paint_DrawString_EN(bpm_x, LCD_1IN54_HEIGHT - 30, bpmStr, &Font16, BLACK, WHITE);
}

// Render the audio timing screen
static void render_audio_timing_screen(void)
{
    // Title at the top
    Paint_DrawString_EN(5, 5, "Audio Timing", &Font20, BLACK, WHITE);

    // Format timing data
    char minStr[32], maxStr[32], avgStr[32];
    snprintf(minStr, sizeof(minStr), "Min: %.3f ms", audioMinMs);
    snprintf(maxStr, sizeof(maxStr), "Max: %.3f ms", audioMaxMs);
    snprintf(avgStr, sizeof(avgStr), "Avg: %.3f ms", audioAvgMs);

    // Display timing data
    Paint_DrawString_EN(10, 40, minStr, &Font16, BLACK, WHITE);
    Paint_DrawString_EN(10, 70, maxStr, &Font16, BLACK, WHITE);
    Paint_DrawString_EN(10, 100, avgStr, &Font16, BLACK, WHITE);
}

// Render the accelerometer timing screen
static void render_accel_timing_screen(void)
{
    // Title at the top
    Paint_DrawString_EN(5, 5, "Accel. Timing", &Font20, BLACK, WHITE);

    // Format timing data
    char minStr[32], maxStr[32], avgStr[32];
    snprintf(minStr, sizeof(minStr), "Min: %.3f ms", accelMinMs);
    snprintf(maxStr, sizeof(maxStr), "Max: %.3f ms", accelMaxMs);
    snprintf(avgStr, sizeof(avgStr), "Avg: %.3f ms", accelAvgMs);

    // Display timing data
    Paint_DrawString_EN(10, 40, minStr, &Font16, BLACK, WHITE);
    Paint_DrawString_EN(10, 70, maxStr, &Font16, BLACK, WHITE);
    Paint_DrawString_EN(10, 100, avgStr, &Font16, BLACK, WHITE);
}

// Force a screen refresh
void LcdDisplay_refresh(void)
{
    if (!isInitialized || s_frameBuffer == NULL)
        return;

    // Initialize the RAM frame buffer to be blank (white)
    Paint_NewImage(s_frameBuffer, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, 0, WHITE, 16);
    Paint_Clear(WHITE);

    // Render the appropriate screen based on current screen type
    switch (currentScreen)
    {
    case LCD_SCREEN_STATUS:
        render_status_screen();
        break;

    case LCD_SCREEN_AUDIO_TIMING:
        render_audio_timing_screen();
        break;

    case LCD_SCREEN_ACCEL_TIMING:
        render_accel_timing_screen();
        break;

    default:
        render_status_screen();
        break;
    }

    LCD_1IN54_Display(s_frameBuffer);
}