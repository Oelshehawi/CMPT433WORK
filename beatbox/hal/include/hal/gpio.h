// Low-level GPIO access using gpiod
#ifndef _GPIO_H_
#define _GPIO_H_

#include <stdbool.h>
#include <stdint.h>

// Opaque struct to hide the gpiod dependency
// Passes a ptr to struct gpiod_line* around without exposing it to clients
typedef void GpioLine;

// We'll need the bulk structure still
// This does expose gpiod library indirectly, which is not great
#include <gpiod.h>

enum eGpioChips
{
    GPIO_CHIP_0 = 0,
    GPIO_CHIP_1,
    GPIO_CHIP_2,
    GPIO_NUM_CHIPS,
};

// Must initialize before calling any other functions.
void Gpio_initialize(void);
void Gpio_cleanup(void);

// Opening a pin gives us a "line" that we later work with.
//  chip: such as GPIO_CHIP_0
//  pinNumber: such as 15
struct GpioLine *Gpio_openForEvents(enum eGpioChips chip, int pinNumber);

// Get the current value of a GPIO line (non-blocking)
// Returns 0 for LOW, 1 for HIGH
int Gpio_getValue(struct GpioLine *line);

int Gpio_waitForLineChange(
    struct GpioLine *line1,
    struct gpiod_line_bulk *bulkEvents);

void Gpio_close(struct GpioLine *line);

#endif