#ifndef _PWM_LED_H_
#define _PWM_LED_H_

// Module to control the LED emitter on the Zen Hat using PWM.
// Controls the flashing of the LED at frequencies between 0 and 500Hz.
// For frequencies below 3Hz, the LED is turned off due to hardware limitations.
// Uses the system PWM hardware via /dev/hat/pwm/GPIO12 interface files.

// Initialize/cleanup the PWM LED module
void PwmLed_init(void);
void PwmLed_cleanup(void);

// Set/get the LED flash frequency in Hz
void PwmLed_setFrequency(double freq_hz);
double PwmLed_getFrequency(void);

#endif