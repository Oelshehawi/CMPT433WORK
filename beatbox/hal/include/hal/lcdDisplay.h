#ifndef _LCD_DISPLAY_H_
#define _LCD_DISPLAY_H_

// LCD Display module for the Zen Hat
// Handles initializing and updating the LCD display
// Shows the current beat name, volume, and BPM

#include <stdbool.h>

// Screen types - kept for API compatibility
typedef enum {
    LCD_SCREEN_STATUS = 0,     // Main status screen with beat name, volume, BPM
    LCD_SCREEN_AUDIO_TIMING,   // Audio timing statistics (not used in simplified version)
    LCD_SCREEN_ACCEL_TIMING,   // Accelerometer timing statistics (not used in simplified version)
    LCD_SCREEN_COUNT           // Total number of screens
} LcdScreenType;

// Initialize the LCD display
void LcdDisplay_init(void);

// Clean up the LCD display
void LcdDisplay_cleanup(void);

// Switch to the next screen (no-op in simplified version)
void LcdDisplay_nextScreen(void);

// Get the current screen type (always returns STATUS in simplified version)
LcdScreenType LcdDisplay_getCurrentScreen(void);

// Update the status screen information
void LcdDisplay_updateStatus(const char* beatName, int volume, int bpm);

// Update audio timing information (no-op in simplified version)
void LcdDisplay_updateAudioTiming(double min, double max, double avg);

// Update accelerometer timing information (no-op in simplified version)
void LcdDisplay_updateAccelTiming(double min, double max, double avg);

// Force a screen refresh
void LcdDisplay_refresh(void);

// Enable or disable debug output (no-op in simplified version)
void LcdDisplay_setDebug(bool enable);

#endif 