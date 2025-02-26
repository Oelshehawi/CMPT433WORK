#ifndef _PWM_LED_H_
#define _PWM_LED_H_

// Initialize/cleanup the PWM LED module
void PwmLed_init(void);
void PwmLed_cleanup(void);

// Set/get the LED flash frequency in Hz
void PwmLed_setFrequency(double freq_hz);
double PwmLed_getFrequency(void);

#endif 