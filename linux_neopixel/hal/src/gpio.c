#include "hal/gpio.h"
#include <stdio.h>

void Gpio_initialize(void)
{
    printf("GPIO module initialized\n");
}

void Gpio_cleanup(void)
{
    printf("GPIO module cleaned up\n");
}