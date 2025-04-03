// Module for managing the LCD display
// Handles updating the display with current status information
#ifndef _DISPLAY_MANAGER_H_
#define _DISPLAY_MANAGER_H_

void DisplayManager_init(void);

void DisplayManager_cleanup(void);

// Update the LCD display with current system information
void DisplayManager_updateDisplay(void);

#endif