#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include "accelerometer.h"

// Thresholds and timing
#define MOVEMENT_THRESHOLD 4000   
#define DEBOUNCE_TIME_MS 100      

// Static variables
static uint64_t last_trigger_time[3] = {0, 0, 0};
static int16_t prev_accel[3] = {0, 0, 0};

static uint64_t get_time_ms(void)
{
    return (clock() * 1000) / CLOCKS_PER_SEC;
}

bool AccelerometerApp_init(void)
{
    // Initialize the HAL layer
    if (!Accelerometer_init()) {
        return false;
    }
    return true;
}

void AccelerometerApp_cleanup(void)
{
    Accelerometer_cleanup();
}

void AccelerometerApp_process(void)
{
    // Mark the start of accelerometer processing
    Period_markEvent(PERIOD_EVENT_ACCEL);

    int16_t x, y, z;
    if (!Accelerometer_readRaw(&x, &y, &z)) {
        return;
    }

    uint64_t current_time = get_time_ms();
    
    // Check each axis for movement
    int16_t deltas[3] = {
        abs(x - prev_accel[0]),
        abs(y - prev_accel[1]),
        abs(z - prev_accel[2])
    };

    // Update previous values
    prev_accel[0] = x;
    prev_accel[1] = y;
    prev_accel[2] = z;

    // Check each axis for significant movement
    for (int i = 0; i < 3; i++) {
        if (deltas[i] > MOVEMENT_THRESHOLD) {
            uint64_t time_since_last = current_time - last_trigger_time[i];
            if (time_since_last >= DEBOUNCE_TIME_MS) {
                // Trigger sound based on axis
                switch (i) {
                    case 0: // X-axis
                        DrumSounds_play(DRUM_SOUND_BASE);
                        break;
                    case 1: // Y-axis
                        DrumSounds_play(DRUM_SOUND_SNARE);
                        break;
                    case 2: // Z-axis
                        DrumSounds_play(DRUM_SOUND_HIHAT);
                        break;
                }
                last_trigger_time[i] = current_time;
            }
        }
    }
} 