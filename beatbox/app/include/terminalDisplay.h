// Module for handling periodic display updates on the console
// Manages timing statistics and handles the periodical output of system status
#ifndef _TERMINAL_DISPLAY_H_
#define _TERMINAL_DISPLAY_H_
#include <stdbool.h>

void TerminalDisplay_init(void);

void TerminalDisplay_cleanup(void);

// Register the application-wide shutdown flag
void TerminalDisplay_registerShutdown(volatile bool *appIsRunning);

// Mark an audio buffer refill event
void TerminalDisplay_markAudioEvent(void);

// Mark an accelerometer sample event
void TerminalDisplay_markAccelerometerEvent(void);

#endif