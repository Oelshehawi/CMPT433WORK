#ifndef TARGET_GAME_H
#define TARGET_GAME_H

#include <stdint.h>
#include <stdbool.h>

// Initialize the target game
void TargetGame_init(void);

// Generate a new random target
void TargetGame_generateNewTarget(void);

// Process pointing coordinates and update LED colors
// Returns true if the target is hit
bool TargetGame_processPointing(float pointingX, float pointingY, uint32_t* outputColors, int numLeds);

// Clean up resources used by the target game
void TargetGame_cleanup(void);

#endif // TARGET_GAME_H 