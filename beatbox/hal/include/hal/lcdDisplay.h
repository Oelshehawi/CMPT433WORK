// This file is used to display the LCD screen
// and update the status, audio timing, and accelerometer timing
#ifndef _LCD_DISPLAY_H_
#define _LCD_DISPLAY_H_
#include <stdbool.h>

// Screen types
typedef enum {
    LCD_SCREEN_STATUS = 0,     // Main status screen with beat name, volume, BPM
    LCD_SCREEN_AUDIO_TIMING,   // Audio timing statistics (not used in simplified version)
    LCD_SCREEN_ACCEL_TIMING,   // Accelerometer timing statistics (not used in simplified version)
    LCD_SCREEN_COUNT           // Total number of screens
} LcdScreenType;

void LcdDisplay_init(void);

void LcdDisplay_cleanup(void);

void LcdDisplay_nextScreen(void);

LcdScreenType LcdDisplay_getCurrentScreen(void);

void LcdDisplay_updateStatus(const char* beatName, int volume, int bpm);

void LcdDisplay_updateAudioTiming(double min, double max, double avg);

void LcdDisplay_updateAccelTiming(double min, double max, double avg);

void LcdDisplay_refresh(void);

void LcdDisplay_setDebug(bool enable);

#endif 