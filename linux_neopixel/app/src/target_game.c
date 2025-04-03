#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "target_game.h"

// Target coordinates
static float targetX = 0.0f;
static float targetY = 0.0f;

// LED color constants (GRB format)
static const uint32_t COLOR_RED_BRIGHT = 0x00FF0000;   // Red (G=00, R=FF, B=00)
static const uint32_t COLOR_GREEN_BRIGHT = 0xFF000000; // Green (G=FF, R=00, B=00)
static const uint32_t COLOR_BLUE_BRIGHT = 0x0000FF00;  // Blue (G=00, R=00, B=FF)
static const uint32_t COLOR_OFF = 0x00000000;          // Off
static const uint32_t COLOR_RED_DIM = 0x00400000;      // Dimmer Red
static const uint32_t COLOR_GREEN_DIM = 0x40000000;    // Dimmer Green
static const uint32_t COLOR_BLUE_DIM = 0x00004000;     // Dimmer Blue

// Thresholds for vertical zones (Adjust these based on testing!)
static const float T1 = 0.1f;  // On target threshold
static const float T2 = 0.2f;  // Threshold between state 1 & 2 (closest)
static const float T3 = 0.35f; // Threshold between state 2 & 3
static const float T4 = 0.55f; // Threshold between state 3 & 4
static const float T5 = 0.75f; // Threshold between state 4 & 5 (furthest)

// Initialize the target game
void TargetGame_init(void)
{
    // Seed random number generator
    srand(time(NULL));

    // Generate initial target
    TargetGame_generateNewTarget();
}

// Generate a new random target
void TargetGame_generateNewTarget(void)
{
    // Generate random float between -0.5 and +0.5
    targetX = ((float)rand() / (float)RAND_MAX) - 0.5f;
    targetY = ((float)rand() / (float)RAND_MAX) - 0.5f;
    printf("New Target: X=%.2f, Y=%.2f\n", targetX, targetY);
}

// Process pointing coordinates and update LED colors
// Returns true if the target is hit
bool TargetGame_processPointing(float pointingX, float pointingY, uint32_t *outputColors, int numLeds)
{
    // Calculate difference between pointing and target
    float deltaX = pointingX - targetX;
    float deltaY = pointingY - targetY;

    // Check if on target using T1 threshold
    bool onTargetX = fabsf(deltaX) < T1;
    bool onTargetY = fabsf(deltaY) < T1;

    // Initialize all LEDs to off
    for (int i = 0; i < numLeds; i++)
    {
        outputColors[i] = COLOR_OFF;
    }

    // Determine base colors based on horizontal offset
    uint32_t baseColorBright = COLOR_BLUE_BRIGHT;
    uint32_t baseColorDim = COLOR_BLUE_DIM;
    if (deltaX < -T1)
    { // Target is Left
        baseColorBright = COLOR_RED_BRIGHT;
        baseColorDim = COLOR_RED_DIM;
    }
    else if (deltaX > T1)
    { // Target is Right
        baseColorBright = COLOR_GREEN_BRIGHT;
        baseColorDim = COLOR_GREEN_DIM;
    }

    // Set LED patterns based on vertical offset (deltaY)
    if (onTargetY)
    {
        // Vertically On Target
        uint32_t colorToUse = (onTargetX) ? COLOR_BLUE_BRIGHT : baseColorBright;
        for (int i = 0; i < numLeds; i++)
        {
            outputColors[i] = colorToUse;
        }
    }
    else if (deltaY < -T5)
    {
        // State Below 1: Way Below (o at index 7)
        if (numLeds > 0)
            outputColors[numLeds - 1] = baseColorDim;
    }
    else if (deltaY < -T4)
    {
        // State Below 2: Far Below (X at 7, o at 6)
        if (numLeds >= 2)
        {
            outputColors[numLeds - 1] = baseColorBright;
            outputColors[numLeds - 2] = baseColorDim;
        }
        else if (numLeds == 1)
            outputColors[0] = baseColorBright;
    }
    else if (deltaY < -T3)
    {
        // State Below 3: Mid-Far Below (o at 5, X at 6, o at 7)
        if (numLeds >= 3)
        {
            outputColors[numLeds - 2] = baseColorBright; // Index 6
            outputColors[numLeds - 3] = baseColorDim;    // Index 5
            outputColors[numLeds - 1] = baseColorDim;    // Index 7
        } // Add simpler cases if needed
    }
    else if (deltaY < -T2)
    {
        // State Below 4: Mid Below (o at 4, X at 5, o at 6)
        if (numLeds >= 3)
        {
            int centerLed = numLeds - 3; // Index 5 for 8 LEDs
            outputColors[centerLed] = baseColorBright;
            if (centerLed > 0)
                outputColors[centerLed - 1] = baseColorDim;
            if (centerLed < numLeds - 1)
                outputColors[centerLed + 1] = baseColorDim;
        } // Add simpler cases if needed
    }
    else if (deltaY < -T1)
    {
        // State Below 5: Close Below (o at 3, X at 4, o at 5)
        if (numLeds >= 3)
        {
            int centerLed = numLeds - 4; // Index 4 for 8 LEDs
            outputColors[centerLed] = baseColorBright;
            if (centerLed > 0)
                outputColors[centerLed - 1] = baseColorDim;
            if (centerLed < numLeds - 1)
                outputColors[centerLed + 1] = baseColorDim;
        } // Add simpler cases if needed
    }
    else if (deltaY > T5)
    {
        // State Above 5: Way Above (o at index 0)
        if (numLeds > 0)
            outputColors[0] = baseColorDim;
    }
    else if (deltaY > T4)
    {
        // State Above 4: Far Above (X at 0, o at 1)
        if (numLeds >= 2)
        {
            outputColors[0] = baseColorBright;
            outputColors[1] = baseColorDim;
        }
        else if (numLeds == 1)
            outputColors[0] = baseColorBright;
    }
    else if (deltaY > T3)
    {
        // State Above 3: Mid-Far Above (o at 1, X at 2, o at 3)
        if (numLeds >= 3)
        {
            outputColors[2] = baseColorBright; // Index 2
            outputColors[1] = baseColorDim;    // Index 1
            outputColors[3] = baseColorDim;    // Index 3
        } // Add simpler cases if needed
    }
    else if (deltaY > T2)
    {
        // State Above 2: Mid Above (o at 2, X at 3, o at 4)
        if (numLeds >= 3)
        {
            int centerLed = 3; // Index 3 for 8 LEDs
            outputColors[centerLed] = baseColorBright;
            if (centerLed > 0)
                outputColors[centerLed - 1] = baseColorDim;
            if (centerLed < numLeds - 1)
                outputColors[centerLed + 1] = baseColorDim;
        } // Add simpler cases if needed
    }
    else if (deltaY > T1)
    {
        // State Above 1: Close Above (o at 2, X at 3, o at 4)
        if (numLeds >= 3)
        {
            int centerLed = 2; // Index 2 for 8 LEDs
            outputColors[centerLed] = baseColorBright;
            if (centerLed > 0)
                outputColors[centerLed - 1] = baseColorDim;
            if (centerLed < numLeds - 1)
                outputColors[centerLed + 1] = baseColorDim;
        } // Add simpler cases if needed
    }
    // If none of the above, LEDs remain off

    // Return true if we're on target in both X and Y
    return (onTargetX && onTargetY);
}

// Clean up resources used by the target game
void TargetGame_cleanup(void)
{
    // Currently nothing to clean up
}