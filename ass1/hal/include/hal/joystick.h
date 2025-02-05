#ifndef _JOYSTICK_H_
#define _JOYSTICK_H_

#include <stdint.h>

typedef enum
{
    JOYSTICK_NONE = 0,
    JOYSTICK_UP,
    JOYSTICK_DOWN,
    JOYSTICK_LEFT,
    JOYSTICK_RIGHT
} JoystickDirection;

// Function prototypes
void joystick_init(void);
void joystick_calibrate(void);
JoystickDirection read_joystick_direction(void);
void joystick_record_successful_move(JoystickDirection dir);
void joystick_test_values(void);

#endif