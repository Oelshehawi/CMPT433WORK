#ifndef HAL_ACCELEROMETER_H
#define HAL_ACCELEROMETER_H

#include <stdbool.h>
#include <stdint.h>

// Initialize the accelerometer hardware
bool Accelerometer_init(void);

// Cleanup the accelerometer hardware
void Accelerometer_cleanup(void);

// Read raw accelerometer values
bool Accelerometer_readRaw(int16_t* x, int16_t* y, int16_t* z);

#endif // HAL_ACCELEROMETER_H 