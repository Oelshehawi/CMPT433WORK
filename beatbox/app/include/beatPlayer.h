// Handles playing different drum beat patterns

#ifndef BEAT_PLAYER_H
#define BEAT_PLAYER_H

#include <stdbool.h>

// Beat modes
typedef enum {
    BEAT_MODE_NONE,
    BEAT_MODE_ROCK,
    BEAT_MODE_CUSTOM,
    BEAT_MODE_COUNT 
} BeatMode_t;

// BPM range
#define MIN_BPM 40
#define MAX_BPM 300
#define DEFAULT_BPM 120

void BeatPlayer_init(void);

void BeatPlayer_start(void);

void BeatPlayer_stop(void);

void BeatPlayer_cleanup(void);

// Set the beat mode
bool BeatPlayer_setMode(BeatMode_t newMode);

BeatMode_t BeatPlayer_getMode(void);

// Set the tempo (BPM)
bool BeatPlayer_setTempo(int newBPM);

// Get the current tempo (BPM)
int BeatPlayer_getTempo(void);

// Check if the beat player is currently playing
bool BeatPlayer_isPlaying(void);

#endif 