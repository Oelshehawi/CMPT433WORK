// drum_sounds.h
// Handles loading and access to drum sound files

#ifndef DRUM_SOUNDS_H
#define DRUM_SOUNDS_H

#include "hal/audioMixer.h"

// Sound file paths
#define DRUM_BASE_PATH "wave-files/100051__menegass__gui-drum-bd-hard.wav"
#define DRUM_HIHAT_PATH "wave-files/100053__menegass__gui-drum-cc.wav"
#define DRUM_SNARE_PATH "wave-files/100059__menegass__gui-drum-snare-soft.wav"

// Drum sound types enum
typedef enum {
    DRUM_SOUND_BASE,
    DRUM_SOUND_HIHAT,
    DRUM_SOUND_SNARE,
    DRUM_SOUND_COUNT // Keep this last
} DrumSound_t;

// Initialize drum sounds - loads all WAV files into memory
void DrumSounds_init(void);

// Cleanup drum sounds - frees all allocated memory
void DrumSounds_cleanup(void);

// Get a pointer to the specified drum sound
wavedata_t* DrumSounds_getSound(DrumSound_t sound);

// Play a specific drum sound
void DrumSounds_play(DrumSound_t sound);

#endif // DRUM_SOUNDS_H 