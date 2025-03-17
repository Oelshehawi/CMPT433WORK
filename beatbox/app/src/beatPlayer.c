#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#include "beatPlayer.h"
#include "drumSounds.h"

// Current beat mode
static BeatMode_t currentMode = BEAT_MODE_NONE;

// Current tempo in BPM
static int currentBPM = DEFAULT_BPM;

// Thread for playing the beat
static pthread_t beatThreadId;
static bool isRunning = false;
static bool threadCreated = false;

// Mutex to protect shared variables
static pthread_mutex_t beatMutex = PTHREAD_MUTEX_INITIALIZER;

// Forward declarations
static void *beatThread(void *arg);
static void playRockBeat(int beatCount);
static void playCustomBeat(int beatCount);
static long long calculateHalfBeatTimeMs(int bpm);

// Initialize the beat player
void BeatPlayer_init(void)
{
    pthread_mutex_lock(&beatMutex);
    currentMode = BEAT_MODE_NONE;
    currentBPM = DEFAULT_BPM;
    isRunning = false;
    threadCreated = false;
    pthread_mutex_unlock(&beatMutex);
}

// Start playing the beat
void BeatPlayer_start(void)
{
    pthread_mutex_lock(&beatMutex);

    if (!threadCreated)
    {
        isRunning = true;
        threadCreated = true;
        pthread_create(&beatThreadId, NULL, beatThread, NULL);
    }
    else if (!isRunning)
    {
        isRunning = true;
    }

    pthread_mutex_unlock(&beatMutex);
}

// Stop playing the beat
void BeatPlayer_stop(void)
{
    pthread_mutex_lock(&beatMutex);
    isRunning = false;
    pthread_mutex_unlock(&beatMutex);
}

// Cleanup and free resources
void BeatPlayer_cleanup(void)
{
    pthread_mutex_lock(&beatMutex);

    if (threadCreated)
    {
        isRunning = false;
        pthread_mutex_unlock(&beatMutex);
        pthread_join(beatThreadId, NULL);
        pthread_mutex_lock(&beatMutex);
        threadCreated = false;
    }

    pthread_mutex_unlock(&beatMutex);
}

// Set the beat mode
bool BeatPlayer_setMode(BeatMode_t newMode)
{
    if (newMode < 0 || newMode >= BEAT_MODE_COUNT)
    {
        return false;
    }

    pthread_mutex_lock(&beatMutex);
    currentMode = newMode;
    pthread_mutex_unlock(&beatMutex);

    return true;
}

// Get the current beat mode
BeatMode_t BeatPlayer_getMode(void)
{
    // No need for mutex since reading an enum is atomic
    return currentMode;
}

// Set the tempo
bool BeatPlayer_setTempo(int newBPM)
{
    if (newBPM < MIN_BPM || newBPM > MAX_BPM)
    {
        return false;
    }

    pthread_mutex_lock(&beatMutex);
    currentBPM = newBPM;
    pthread_mutex_unlock(&beatMutex);

    return true;
}

// Get the current tempo
int BeatPlayer_getTempo(void)
{
    // No need for mutex since reading an int is atomic
    return currentBPM;
}

// Check if the beat player is currently playing
bool BeatPlayer_isPlaying(void)
{
    // No need for mutex since reading a bool is atomic
    return isRunning;
}

// Thread function for playing the beat
static void *beatThread(void *arg)
{
    (void)arg; // Unused parameter

    int beatCount = 0;

    while (1)
    {
        // Check if we should exit
        pthread_mutex_lock(&beatMutex);
        bool shouldRun = isRunning;
        BeatMode_t mode = currentMode;
        int bpm = currentBPM;
        pthread_mutex_unlock(&beatMutex);

        if (!shouldRun)
        {
            usleep(100000); // Sleep for 100ms when not running
            continue;
        }

        // Play the appropriate beat pattern based on the current mode
        switch (mode)
        {
        case BEAT_MODE_NONE:
            // No beat to play
            break;

        case BEAT_MODE_ROCK:
            playRockBeat(beatCount);
            break;

        case BEAT_MODE_CUSTOM:
            playCustomBeat(beatCount);
            break;

        default:
            // Unknown mode, do nothing
            break;
        }

        // Calculate sleep time for half a beat
        long long sleepTimeMs = calculateHalfBeatTimeMs(bpm);

        // Sleep for half a beat
        usleep(sleepTimeMs * 1000); // Convert to microseconds

        // Increment beat count (modulo 8 for a 4-beat pattern)
        beatCount = (beatCount + 1) % 8;
    }

    return NULL; // Never reached
}

// Play the standard rock beat pattern
static void playRockBeat(int beatCount)
{
    /*
    Rock Beat Pattern:
    Beat | Base | Snare | Hi-Hat
    1     X               X
    1.5                   X
    2           X         X
    2.5                   X
    3     X               X
    3.5                   X
    4           X         X
    4.5                   X
    */

    switch (beatCount)
    {
    case 0: // Beat 1
        DrumSounds_play(DRUM_SOUND_BASE);
        DrumSounds_play(DRUM_SOUND_HIHAT);
        break;

    case 1: // Beat 1.5
        DrumSounds_play(DRUM_SOUND_HIHAT);
        break;

    case 2: // Beat 2
        DrumSounds_play(DRUM_SOUND_SNARE);
        DrumSounds_play(DRUM_SOUND_HIHAT);
        break;

    case 3: // Beat 2.5
        DrumSounds_play(DRUM_SOUND_HIHAT);
        break;

    case 4: // Beat 3
        DrumSounds_play(DRUM_SOUND_BASE);
        DrumSounds_play(DRUM_SOUND_HIHAT);
        break;

    case 5: // Beat 3.5
        DrumSounds_play(DRUM_SOUND_HIHAT);
        break;

    case 6: // Beat 4
        DrumSounds_play(DRUM_SOUND_SNARE);
        DrumSounds_play(DRUM_SOUND_HIHAT);
        break;

    case 7: // Beat 4.5
        DrumSounds_play(DRUM_SOUND_HIHAT);
        break;
    }
}

// Play a custom beat pattern (different from the rock beat)
static void playCustomBeat(int beatCount)
{
    /*
    Custom Beat Pattern (just an example):
    Beat | Base | Snare | Hi-Hat
    1     X      X
    1.5
    2                    X
    2.5          X
    3     X
    3.5                  X
    4           X        X
    4.5    X
    */

    switch (beatCount)
    {
    case 0: // Beat 1
        DrumSounds_play(DRUM_SOUND_BASE);
        DrumSounds_play(DRUM_SOUND_SNARE);
        break;

    case 1: // Beat 1.5
        // Silent
        break;

    case 2: // Beat 2
        DrumSounds_play(DRUM_SOUND_HIHAT);
        break;

    case 3: // Beat 2.5
        DrumSounds_play(DRUM_SOUND_SNARE);
        break;

    case 4: // Beat 3
        DrumSounds_play(DRUM_SOUND_BASE);
        break;

    case 5: // Beat 3.5
        DrumSounds_play(DRUM_SOUND_HIHAT);
        break;

    case 6: // Beat 4
        DrumSounds_play(DRUM_SOUND_SNARE);
        DrumSounds_play(DRUM_SOUND_HIHAT);
        break;

    case 7: // Beat 4.5
        DrumSounds_play(DRUM_SOUND_BASE);
        break;
    }
}

// Calculate the time for half a beat in milliseconds
static long long calculateHalfBeatTimeMs(int bpm)
{
    // Time for half beat [ms] = 60,000 [ms/min] / BPM / 2 [half-beats per beat]
    return 60000LL / bpm / 2;
}