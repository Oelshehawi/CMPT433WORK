#include "hal/LEDStart.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

void LED_init(void)
{
    FILE *triggerFile = fopen(LED_TRIGGER, "w");
    if (triggerFile == NULL)
    {
        perror("Error opening LED trigger file");
        exit(EXIT_FAILURE);
    }
    fprintf(triggerFile, "none");
    fclose(triggerFile);
}

void LED_start(void)
{
    // Flash LED pattern 4 times
    for (int i = 0; i < 4; i++)
    {
        set_led_state(GREEN_LED_BRIGHTNESS, 1);
        delay_ms(250);
        set_led_state(GREEN_LED_BRIGHTNESS, 0);

        set_led_state(RED_LED_BRIGHTNESS, 1);
        delay_ms(250);
        set_led_state(RED_LED_BRIGHTNESS, 0);
    }
}

void LED_cleanup(void)
{
    set_led_state(GREEN_LED_BRIGHTNESS, 0);
    set_led_state(RED_LED_BRIGHTNESS, 0);
}

void delay_ms(int ms)
{
    struct timespec reqDelay = {
        .tv_sec = ms / 1000,
        .tv_nsec = (ms % 1000) * 1000000L};
    nanosleep(&reqDelay, NULL);
}

void set_led_state(const char *path, int state)
{
    FILE *ledFile = fopen(path, "w");
    if (ledFile == NULL)
    {
        perror("Error opening LED file");
        exit(EXIT_FAILURE);
    }
    fprintf(ledFile, "%d", state);
    fclose(ledFile);
}