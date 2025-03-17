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
#include <unistd.h> // For sleep

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

    // Module initialization
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

    // Mark as initialized
    isInitialized = true;
    printf("LCD: Initialization complete\n");

    // Initial display update
    LcdDisplay_updateStatus("ROCK", 80, 120);
}

// Clean up the LCD display
void LcdDisplay_cleanup(void)
{
    if (!isInitialized)
        return;

    printf("LCD: Cleaning up resources...\n");

    // Free frame buffer
    if (s_frameBuffer != NULL)
    {
        free(s_frameBuffer);
        s_frameBuffer = NULL;
    }

    // Module cleanup
    DEV_ModuleExit();
    isInitialized = false;
    printf("LCD: Cleanup complete\n");
}

// Set debug output mode
void LcdDisplay_setDebug(bool enable)
{
    // Prevent unused parameter warning
    (void)enable;

    // No-op in simplified version
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

    // Update the audio timing data
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

    // Update the accelerometer timing data
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
    // Format the message for status screen
    char message[256];
    snprintf(message, sizeof(message),
             "BeatBox\n"
             "%s\n"
             "Vol: %d\n"
             "BPM: %d",
             beatName,
             currentVolume,
             currentBpm);

    // Draw each line
    int x = 5;
    int y = 5;
    int line_count = 0;
    char line[100]; // Buffer for each line
    const char *current = message;

    while (*current != '\0' && line_count < 10)
    {
        // Copy characters until newline or end of string
        int i = 0;
        while (current[i] != '\n' && current[i] != '\0' && i < 99)
        {
            line[i] = current[i];
            i++;
        }
        line[i] = '\0'; // Null terminate the line

        // Use larger font for title, smaller for data
        if (line_count == 0)
        {
            // Title - larger font
            Paint_DrawString_EN(x, y, line, &Font20, BLACK, WHITE);
            y += 30; // More space after title
        }
        else if (line_count == 1)
        {
            // Beat name - large and centered
            int center_x = (LCD_1IN54_WIDTH - strlen(line) * 16) / 2;
            if (center_x < 0)
                center_x = 5;
            Paint_DrawString_EN(center_x, y + 20, line, &Font24, BLACK, WHITE);
            y += 60; // More space for the important beat name
        }
        else
        {
            // Regular text - smaller font
            Paint_DrawString_EN(x, y, line, &Font16, BLACK, WHITE);
            y += 25; // Space between lines
        }

        line_count++;

        // Move to next line if we hit a newline
        if (current[i] == '\n')
        {
            current += i + 1;
        }
        else
        {
            current += i;
        }
    }
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
        // Default to status screen
        render_status_screen();
        break;
    }

    // Update the display
    LCD_1IN54_Display(s_frameBuffer);
}