#ifndef _JOYSTICK_H_
#define _JOYSTICK_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    JOYSTICK_NONE = 0,
    JOYSTICK_UP,
    JOYSTICK_DOWN,
    JOYSTICK_LEFT,
    JOYSTICK_RIGHT,
    JOYSTICK_PRESS
} JoystickDirection;

// Initialize the joystick hardware
void Joystick_init(void);

// Clean up the joystick hardware
void Joystick_cleanup(void);

// Get the current joystick state (debounced)
JoystickDirection Joystick_getDirection(void);

// Check if the joystick is currently pressed (center button)
bool Joystick_isPressed(void);

// Start the joystick sampling thread
void Joystick_startSampling(void);

// Stop the joystick sampling thread
void Joystick_stopSampling(void);

// Function prototypes
void joystick_calibrate(void);
void joystick_record_successful_move(JoystickDirection dir);
void joystick_test_values(void);

#endif