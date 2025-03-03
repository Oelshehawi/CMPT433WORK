#ifndef _TERMINAL_DISPLAY_H_
#define _TERMINAL_DISPLAY_H_

// Module to manage the terminal output display for the light sampler application.
// Runs a background thread that updates the display once per second.
// Displays critical information including:
// - Number of samples taken in the previous second
// - Current LED flash frequency in Hz
// - Average light level in volts
// - Number of light dips detected
// - Timing statistics for light sampling (min, max, avg)
// - 10 evenly spaced light samples from the previous second
// Also triggers the Sampler module to move current data to history each second.

// Initialize the terminal display module
// Starts a thread that updates the display every second
void TerminalDisplay_init(void);

// Cleanup the terminal display module
// Stops the display thread and performs cleanup
void TerminalDisplay_cleanup(void);

#endif