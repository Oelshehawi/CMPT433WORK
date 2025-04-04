#ifndef FIRING_INPUT_H
#define FIRING_INPUT_H

#include <stdbool.h>

/**
 * Initialize the firing input button (GPIO24)
 * Sets up GPIO, initializes state, and starts the background thread
 * @return true if initialization succeeds, false otherwise
 */
bool FiringInput_init(void);

/**
 * Clean up resources used by the firing input module
 * Stops the thread and cleans up GPIO
 */
void FiringInput_cleanup(void);

/**
 * Check if the button was pressed since the last call
 * @return true if the button was pressed, false otherwise
 * Note: Resets the flag if it was set
 */
bool FiringInput_wasButtonPressed(void);

#endif // FIRING_INPUT_H 