#ifndef _LCD_DISPLAY_IMPL_H_
#define _LCD_DISPLAY_IMPL_H_

// Module for controlling the 14-segment display on the Zen Hat.
// Provides functionality to initialize, update, and clean up the LCD display.
// Displays important metrics including:
// - Student name
// - LED flash frequency in Hz
// - Number of light dips detected
// - Maximum sampling interval
// The display is updated using the Zen Hat's I2C interface to communicate
// with the display driver. Handles all the necessary initialization commands
// and character display operations.

// Initialize the LCD display hardware
void LcdDisplayImpl_init(void);

// Cleanup the LCD display hardware
void LcdDisplayImpl_cleanup(void);

// Update the LCD display with new values
void LcdDisplayImpl_update(char *message);

#endif