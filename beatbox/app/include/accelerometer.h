#ifndef APP_ACCELEROMETER_H
#define APP_ACCELEROMETER_H

#include <stdbool.h>
#include "hal/accelerometer.h"
#include "drumSounds.h"
#include "periodTimer.h"

// Initialize the accelerometer application
bool AccelerometerApp_init(void);

// Cleanup the accelerometer application
void AccelerometerApp_cleanup(void);

// Process accelerometer data and handle application logic
void AccelerometerApp_process(void);

#endif // APP_ACCELEROMETER_H 