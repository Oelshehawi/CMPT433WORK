#ifndef TARGET_GAME_H
#define TARGET_GAME_H

#include <stdint.h>
#include <stdbool.h>

// Animation state enum
typedef enum {
    ANIM_NONE,      // No animation running
    ANIM_HIT,       // Hit explosion animation
    ANIM_MISS       // Miss animation
} AnimationState_t;

// Initialize the target game
void TargetGame_init(void);

// Generate a new random target
void TargetGame_generateNewTarget(void);

// Process pointing coordinates and update LED colors
// Returns true if the target is hit
bool TargetGame_processPointing(float pointingX, float pointingY, uint32_t* outputColors, int numLeds);

// Process firing action
// Returns true if target was hit
bool TargetGame_fire(float pointingX, float pointingY);

// Update animations and write to LED array
// Returns true if animation is still running
bool TargetGame_updateAnimations(uint32_t* outputColors, int numLeds);

// Get game statistics
void TargetGame_getStats(int* hits, int* misses, unsigned long* runTimeMs);

// Clean up resources used by the target game
void TargetGame_cleanup(void);

#endif // TARGET_GAME_H 