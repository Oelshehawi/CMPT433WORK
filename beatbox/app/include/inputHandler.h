#ifndef _INPUT_HANDLER_H_
#define _INPUT_HANDLER_H_

#include <stdbool.h>

// Module for handling joystick input for the application
// Manages volume control with joystick up/down

// Initialize the input handler
void InputHandler_init(void);

// Clean up input handler resources
void InputHandler_cleanup(void);


#endif