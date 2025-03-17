#ifndef _DISPLAY_MANAGER_H_
#define _DISPLAY_MANAGER_H_

// Module for managing the LCD display
// Handles updating the display with current status information

// Initialize the display manager
void DisplayManager_init(void);

// Clean up display manager resources
void DisplayManager_cleanup(void);

// Update the LCD display with current system information
// This function should be called periodically from the main loop
void DisplayManager_updateDisplay(void);

#endif