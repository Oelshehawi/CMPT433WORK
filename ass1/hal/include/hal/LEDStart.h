#ifndef LEDSTART_H
#define LEDSTART_H

// Paths for LED brightness and trigger
#define GREEN_LED_BRIGHTNESS "/sys/class/leds/ACT/brightness"
#define RED_LED_BRIGHTNESS "/sys/class/leds/PWR/brightness"
#define LED_TRIGGER "/sys/class/leds/ACT/trigger"

// Function prototypes
void LED_init(void);
void LED_start(void);
void LED_cleanup(void);
void set_led_state(const char *path, int state);
void delay_ms(int ms);

#endif