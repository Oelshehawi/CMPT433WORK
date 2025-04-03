#ifndef MEMORY_HANDLER_H
#define MEMORY_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

// Initialize and set up shared memory mapping
bool MemoryHandler_init(void);

// Write color values to the LED array at the correct offset
void MemoryHandler_writeColors(uint32_t* colors, int numLeds);

// Clean up shared memory resources
void MemoryHandler_cleanup(void);

#endif // MEMORY_HANDLER_H 