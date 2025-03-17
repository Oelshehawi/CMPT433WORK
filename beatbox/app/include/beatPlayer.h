// beat_player.h
// Handles playing different drum beat patterns

#ifndef BEAT_PLAYER_H
#define BEAT_PLAYER_H

#include <stdbool.h>

// Beat modes
typedef enum {
    BEAT_MODE_NONE,
    BEAT_MODE_ROCK,
    BEAT_MODE_CUSTOM,
    BEAT_MODE_COUNT // Keep this last
} BeatMode_t;

// BPM range
#define MIN_BPM 40
#define MAX_BPM 300
#define DEFAULT_BPM 120

// Initialize the beat player
void BeatPlayer_init(void);

// Start playing the beat
void BeatPlayer_start(void);

// Stop playing the beat
void BeatPlayer_stop(void);

// Cleanup and free resources
void BeatPlayer_cleanup(void);

// Set the beat mode
// Returns true if successful, false otherwise
bool BeatPlayer_setMode(BeatMode_t newMode);

// Get the current beat mode
BeatMode_t BeatPlayer_getMode(void);

// Set the tempo (BPM)
// Returns true if successful, false otherwise
bool BeatPlayer_setTempo(int newBPM);

// Get the current tempo (BPM)
int BeatPlayer_getTempo(void);

// Check if the beat player is currently playing
bool BeatPlayer_isPlaying(void);

#endif // BEAT_PLAYER_H 