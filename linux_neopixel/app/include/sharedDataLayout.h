#ifndef SHARED_DATA_LAYOUT_H
#define SHARED_DATA_LAYOUT_H

#include <stdint.h>
#include <stdbool.h>

// NeoPixel configuration
#define NEO_NUM_LEDS 8
#define NEO_COLOR_ARRAY_OFFSET 0x6000
#define NEO_SHARED_MEM_SIZE (NEO_COLOR_ARRAY_OFFSET + NEO_NUM_LEDS * sizeof(uint32_t))

// Helper macros for memory access
#define MEM_UINT8(addr) *(volatile uint8_t *)(pSharedMem + (addr))
#define MEM_UINT32(addr) *(volatile uint32_t *)(pSharedMem + (addr))

#endif // SHARED_DATA_LAYOUT_H