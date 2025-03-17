#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>

#include "drumSounds.h"
#include "hal/audioMixer.h"

// Array of drum sounds
static wavedata_t drumSounds[DRUM_SOUND_COUNT];

// Initialize drum sounds
void DrumSounds_init(void)
{
    // Load base drum
    AudioMixer_readWaveFileIntoMemory(DRUM_BASE_PATH, &drumSounds[DRUM_SOUND_BASE]);

    // Load hi-hat
    AudioMixer_readWaveFileIntoMemory(DRUM_HIHAT_PATH, &drumSounds[DRUM_SOUND_HIHAT]);

    // Load snare
    AudioMixer_readWaveFileIntoMemory(DRUM_SNARE_PATH, &drumSounds[DRUM_SOUND_SNARE]);

    printf("Drum sounds loaded successfully.\n");
}

// Cleanup drum sounds
void DrumSounds_cleanup(void)
{
    // Free all drum sound data
    for (int i = 0; i < DRUM_SOUND_COUNT; i++)
    {
        AudioMixer_freeWaveFileData(&drumSounds[i]);
    }
}

// Get a pointer to the specified drum sound
wavedata_t *DrumSounds_getSound(DrumSound_t sound)
{
    if (sound < 0 || sound >= DRUM_SOUND_COUNT)
    {
        fprintf(stderr, "ERROR: Invalid drum sound index: %d\n", sound);
        return NULL;
    }

    return &drumSounds[sound];
}

// Play a specific drum sound
void DrumSounds_play(DrumSound_t sound)
{
    if (sound < 0 || sound >= DRUM_SOUND_COUNT)
    {
        fprintf(stderr, "ERROR: Invalid drum sound index: %d\n", sound);
        return;
    }

    AudioMixer_queueSound(&drumSounds[sound]);
}