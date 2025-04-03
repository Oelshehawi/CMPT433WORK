// This file is used to handle the rotary encoder
// and button press for the beatbox
#ifndef _ROTARY_ENCODER_H_
#define _ROTARY_ENCODER_H_

#include "../../../app/include/beatPlayer.h" // For BeatMode_t

// Default BPM
#define DEFAULT_BPM 120

// Function prototypes
void RotaryEncoder_init(void);
void RotaryEncoder_cleanup(void);
int RotaryEncoder_process(void);
BeatMode_t RotaryEncoder_getBeatMode(void);
int RotaryEncoder_getBPM(void);
void RotaryEncoder_setBPM(int bpm);

#endif 