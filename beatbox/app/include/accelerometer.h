// This file is used to handle the accelerometer
// and read the raw values from the accelerometer
#ifndef APP_ACCELEROMETER_H
#define APP_ACCELEROMETER_H

#include <stdbool.h>
#include "hal/accelerometer.h"
#include "drumSounds.h"
#include "periodTimer.h"

bool AccelerometerApp_init(void);

void AccelerometerApp_cleanup(void);

// Process accelerometer data 
void AccelerometerApp_process(void);

#endif 