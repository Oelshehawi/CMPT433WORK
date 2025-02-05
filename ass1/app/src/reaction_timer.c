#include "hal/joystick.h"
#include "hal/LEDStart.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// Timer helper
static long long get_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

// Flash LED helper
static void flash_led(const char *led_path, int times, int duration_ms)
{
    for (int i = 0; i < times; i++)
    {
        set_led_state(led_path, 1);
        usleep(duration_ms * 1000);
        set_led_state(led_path, 0);
        usleep(duration_ms * 1000);
    }
}

// Global best time
static long long best_time = -1;

void reaction_timer(void)
{
    // Initialize
    joystick_init();
    LED_init();
    srand(time(NULL));

    // Display welcome message
    printf("Welcome to the Reaction Timer Game!\n");
    printf("Instructions:\n");
    printf("- Wait for the LED prompt (red=down, green=up)\n");
    printf("- Move the joystick in the indicated direction as fast as you can\n");
    printf("- Try to beat your best time!\n");
    printf("- Press left or right to quit\n\n");

    while (1)
    {
        // Step 1: Get ready sequence
        printf("\nGet ready!\n");
        LED_start();

        // Step 2: Wait for joystick to return to neutral
        JoystickDirection dir = read_joystick_direction();
        if (dir != JOYSTICK_NONE)
        {
            printf("Please let go of the joystick...\n");
        }

        long long center_start_time = get_time_ms();
        while (dir != JOYSTICK_NONE)
        {
            if (get_time_ms() - center_start_time > 5000)
            {
                printf("\nNo input detected for 5 seconds. Game ended.\n");
                LED_cleanup();
                return;
            }
            usleep(100000); // 100ms
            dir = read_joystick_direction();
        }

        // Step 3: Random delay (0.5 to 3.0 seconds)
        int delay_ms = 500 + (rand() % 2500);
        usleep(delay_ms * 1000);

        // Step 4: Check for early movement
        dir = read_joystick_direction();
        if (dir != JOYSTICK_NONE)
        {
            printf("Too early! Wait for the prompt. (Detected: %s)\n",
                   dir == JOYSTICK_UP ? "UP" : dir == JOYSTICK_DOWN ? "DOWN"
                                           : dir == JOYSTICK_LEFT   ? "LEFT"
                                           : dir == JOYSTICK_RIGHT  ? "RIGHT"
                                                                    : "UNKNOWN");
            continue;
        }

        // Step 5: Choose random direction (UP or DOWN only)
        JoystickDirection target = (rand() % 2) ? JOYSTICK_UP : JOYSTICK_DOWN;
        printf("Move joystick %s!\n", (target == JOYSTICK_UP) ? "UP" : "DOWN");

        // Light corresponding LED
        if (target == JOYSTICK_UP)
        {
            set_led_state(GREEN_LED_BRIGHTNESS, 1);
            set_led_state(RED_LED_BRIGHTNESS, 0);
        }
        else
        {
            set_led_state(GREEN_LED_BRIGHTNESS, 0);
            set_led_state(RED_LED_BRIGHTNESS, 1);
        }

        // Step 6: Time the response
        long long start_time = get_time_ms();
        JoystickDirection user_dir;

        while ((user_dir = read_joystick_direction()) == JOYSTICK_NONE)
        {
            if (get_time_ms() - start_time > 5000)
            {
                printf("Time out! Too slow (over 5 seconds)\n");
                set_led_state(GREEN_LED_BRIGHTNESS, 0);
                set_led_state(RED_LED_BRIGHTNESS, 0);
                break;
            }
        }

        // Calculate reaction time
        long long reaction_time = get_time_ms() - start_time;

        // Turn off LEDs
        set_led_state(GREEN_LED_BRIGHTNESS, 0);
        set_led_state(RED_LED_BRIGHTNESS, 0);

        // Step 7: Process response
        if (user_dir == JOYSTICK_LEFT || user_dir == JOYSTICK_RIGHT)
        {
            printf("\nGame ended by user\n");
            if (best_time != -1)
            {
                printf("Best time achieved: %lld ms\n", best_time);
            }
            break;
        }
        else if (user_dir == target)
        {
            printf("Correct direction!\n");

            if (best_time == -1 || reaction_time < best_time)
            {
                best_time = reaction_time;
                printf("New best time!\n");
            }
            printf("Response time: %lld ms (Best: %lld ms)\n",
                   reaction_time, best_time);
            flash_led(GREEN_LED_BRIGHTNESS, 3, 100); // Success feedback
        }
        else
        {
            printf("Wrong direction!\n");
            flash_led(RED_LED_BRIGHTNESS, 3, 100); // Error feedback
        }
    }

    // Cleanup
    LED_cleanup();
}