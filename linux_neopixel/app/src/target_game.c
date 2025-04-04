#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include "target_game.h"

// Target coordinates
static float targetX = 0.0f;
static float targetY = 0.0f;

// Game statistics
static int hitCount = 0;
static int missCount = 0;
static unsigned long gameStartTime = 0;

// Animation state
static AnimationState_t currentAnimation = ANIM_NONE;
static unsigned long animationStartTime = 0;
static unsigned int animationFrame = 0;

// Animation durations (3x longer than before)
#define HIT_ANIM_DURATION_MS 2400  // Was 800ms
#define MISS_ANIM_DURATION_MS 1200 // Was 400ms
#define ANIMATION_FRAME_INTERVAL_MS 50

// LED color constants (GRB format)
static const uint32_t COLOR_RED_BRIGHT = 0x00FF0000;    // Red (G=00, R=FF, B=00)
static const uint32_t COLOR_GREEN_BRIGHT = 0xFF000000;  // Green (G=FF, R=00, B=00)
static const uint32_t COLOR_BLUE_BRIGHT = 0x0000FF00;   // Blue (G=00, R=00, B=FF)
static const uint32_t COLOR_ORANGE_BRIGHT = 0x60FF0000; // Orange (G=60, R=FF, B=00)
static const uint32_t COLOR_YELLOW_BRIGHT = 0xFFFF0000; // Yellow (G=FF, R=FF, B=00)
static const uint32_t COLOR_WHITE_BRIGHT = 0xFFFFFF00;  // White (G=FF, R=FF, B=FF)
static const uint32_t COLOR_OFF = 0x00000000;           // Off
static const uint32_t COLOR_RED_DIM = 0x00400000;       // Dimmer Red
static const uint32_t COLOR_GREEN_DIM = 0x40000000;     // Dimmer Green
static const uint32_t COLOR_BLUE_DIM = 0x00004000;      // Dimmer Blue

// Thresholds for vertical zones (Adjust these based on testing!)
static const float T1 = 0.1f;  // On target threshold
static const float T2 = 0.2f;  // Threshold between state 1 & 2 (closest)
static const float T3 = 0.35f; // Threshold between state 2 & 3
static const float T4 = 0.55f; // Threshold between state 3 & 4
static const float T5 = 0.75f; // Threshold between state 4 & 5 (furthest)

// Helper function to get current time in milliseconds
static unsigned long getCurrentTimeMs(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

// Initialize the target game
void TargetGame_init(void)
{
    // Seed random number generator
    srand(time(NULL));

    // Initialize game statistics
    hitCount = 0;
    missCount = 0;
    gameStartTime = getCurrentTimeMs();

    // Initialize animation state
    currentAnimation = ANIM_NONE;
    animationStartTime = 0;
    animationFrame = 0;

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

// Get game statistics
void TargetGame_getStats(int *hits, int *misses, unsigned long *runTimeMs)
{
    if (hits)
        *hits = hitCount;
    if (misses)
        *misses = missCount;
    if (runTimeMs)
        *runTimeMs = getCurrentTimeMs() - gameStartTime;
}

// Process pointing coordinates and update LED colors
// Returns true if the target is hit
bool TargetGame_processPointing(float pointingX, float pointingY, uint32_t *outputColors, int numLeds)
{
    // If an animation is running, don't update targeting display
    if (currentAnimation != ANIM_NONE)
    {
        return false;
    }

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

// Process firing action
// Returns true if target was hit
bool TargetGame_fire(float pointingX, float pointingY)
{
    // Calculate if we're on target
    float deltaX = pointingX - targetX;
    float deltaY = pointingY - targetY;
    bool onTargetX = fabsf(deltaX) < T1;
    bool onTargetY = fabsf(deltaY) < T1;
    bool hit = onTargetX && onTargetY;

    // Increment hit or miss counter
    if (hit)
    {
        hitCount++;
    }
    else
    {
        missCount++;
    }

    // Start the appropriate animation
    currentAnimation = hit ? ANIM_HIT : ANIM_MISS;
    animationStartTime = getCurrentTimeMs();
    animationFrame = 0;

    printf("FIRE! %s (Hits: %d, Misses: %d)\n", hit ? "HIT!" : "Miss", hitCount, missCount);

    return hit;
}

// Helper for hit explosion animation
static void renderHitAnimation(uint32_t *outputColors, int numLeds, unsigned int frame)
{
    int center = numLeds / 2;
    int radius = frame % (numLeds / 2 + 2);

    // Reset LEDs
    for (int i = 0; i < numLeds; i++)
    {
        outputColors[i] = COLOR_OFF;
    }

    // Explosion animation with alternating colors expanding outward
    for (int i = 0; i < numLeds; i++)
    {
        int distance = abs(i - center);
        if (distance == radius % (numLeds / 2 + 1))
        {
            // Cycle through colors as the explosion progresses
            switch (frame % 4)
            {
            case 0:
                outputColors[i] = COLOR_ORANGE_BRIGHT;
                break;
            case 1:
                outputColors[i] = COLOR_RED_BRIGHT;
                break;
            case 2:
                outputColors[i] = COLOR_YELLOW_BRIGHT;
                break;
            case 3:
                outputColors[i] = COLOR_WHITE_BRIGHT;
                break;
            }
        }
        // For frames 4-7, add a second explosion wave
        if (frame >= 4 && distance == (radius + 2) % (numLeds / 2 + 1))
        {
            outputColors[i] = COLOR_RED_BRIGHT;
        }
    }

    // For later frames, add some flashing at the center
    if (frame > 6)
    {
        int centerLeds = (frame % 2) + 1;
        for (int i = center - centerLeds; i <= center + centerLeds; i++)
        {
            if (i >= 0 && i < numLeds)
            {
                outputColors[i] = (frame % 2 == 0) ? COLOR_YELLOW_BRIGHT : COLOR_WHITE_BRIGHT;
            }
        }
    }
}

// Helper for miss animation - all LEDs turn on red and flash
static void renderMissAnimation(uint32_t *outputColors, int numLeds, unsigned int frame)
{
    // Reset LEDs
    for (int i = 0; i < numLeds; i++)
    {
        outputColors[i] = COLOR_OFF;
    }

    // For the miss animation, simply flash all LEDs red at different intensities
    // Alternate between bright red, dim red, and off
    if (frame % 3 == 0)
    {
        // Bright red phase
        for (int i = 0; i < numLeds; i++)
        {
            outputColors[i] = COLOR_RED_BRIGHT;
        }
    }
    else if (frame % 3 == 1)
    {
        // Dim red phase
        for (int i = 0; i < numLeds; i++)
        {
            outputColors[i] = COLOR_RED_DIM;
        }
    }
    // Third phase (frame % 3 == 2) is all LEDs off, which is already done
}

// Update animations and write to LED array
// Returns true if animation is still running
bool TargetGame_updateAnimations(uint32_t *outputColors, int numLeds)
{
    // If no animation is running, return false
    if (currentAnimation == ANIM_NONE)
    {
        return false;
    }

    // Calculate elapsed time
    unsigned long currentTime = getCurrentTimeMs();
    unsigned long elapsedTime = currentTime - animationStartTime;

    // Calculate current frame based on time
    unsigned int newFrame = elapsedTime / ANIMATION_FRAME_INTERVAL_MS;

    // Check if animation is complete
    bool animationComplete = false;

    switch (currentAnimation)
    {
    case ANIM_HIT:
        animationComplete = elapsedTime >= HIT_ANIM_DURATION_MS;
        if (!animationComplete)
        {
            renderHitAnimation(outputColors, numLeds, newFrame);
        }
        break;
    case ANIM_MISS:
        animationComplete = elapsedTime >= MISS_ANIM_DURATION_MS;
        if (!animationComplete)
        {
            renderMissAnimation(outputColors, numLeds, newFrame);
        }
        break;
    default:
        break;
    }

    // If animation is complete, reset animation state
    if (animationComplete)
    {
        AnimationState_t completedAnimation = currentAnimation;
        currentAnimation = ANIM_NONE;

        // If it was a hit animation, generate a new target
        if (completedAnimation == ANIM_HIT)
        {
            TargetGame_generateNewTarget();
        }

        return false;
    }

    // If frame changed, update frame counter
    if (newFrame != animationFrame)
    {
        animationFrame = newFrame;
    }

    return true;
}

// Clean up resources used by the target game
void TargetGame_cleanup(void)
{
    // Currently nothing to clean up
}