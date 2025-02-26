#ifndef _TERMINAL_DISPLAY_H_
#define _TERMINAL_DISPLAY_H_

// Initialize the terminal display module
// Starts a thread that updates the display every second
void TerminalDisplay_init(void);

// Cleanup the terminal display module
// Stops the display thread and performs cleanup
void TerminalDisplay_cleanup(void);

#endif 