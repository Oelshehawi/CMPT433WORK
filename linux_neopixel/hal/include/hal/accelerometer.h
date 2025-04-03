#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

#include <stdbool.h>
#include <stdint.h>

// Initialize the accelerometer hardware and start background reading thread
bool Accelerometer_init(void);

// Read the latest accelerometer values
bool Accelerometer_readRaw(int16_t* x, int16_t* y, int16_t* z);

// Cleanup accelerometer resources
void Accelerometer_cleanup(void);

#endif // ACCELEROMETER_H
