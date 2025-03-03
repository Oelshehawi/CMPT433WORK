#ifndef _ROTARY_ENCODER_H_
#define _ROTARY_ENCODER_H_

// Module to interact with the rotary encoder on the Zen Hat.
// Provides functionality to detect and process rotation events in both
// clockwise and counter-clockwise directions.
// Uses GPIO input pins to detect state changes on the encoder,
// Each full rotation generates 24 state changes.                   

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