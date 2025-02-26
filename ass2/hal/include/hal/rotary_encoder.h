#ifndef _ROTARY_ENCODER_H_
#define _ROTARY_ENCODER_H_

// Initialize/cleanup the rotary encoder module
void RotaryEncoder_init(void);
void RotaryEncoder_cleanup(void);

// Process any pending rotary encoder events
// Returns:
//   1 if clockwise rotation
//   -1 if counter-clockwise rotation
//   0 if no rotation
int RotaryEncoder_process(void);

#endif 