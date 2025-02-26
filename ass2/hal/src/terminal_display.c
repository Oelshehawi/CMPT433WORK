#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>

#include "hal/terminal_display.h"
#include "hal/sampler.h"
#include "hal/periodTimer.h"
#include "hal/pwm_led.h"

static pthread_t display_thread;
static volatile bool should_stop = false;
static bool is_initialized = false;

static void print_display_line(void)
{
    // Get timing statistics
    Period_statistics_t stats;
    Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, &stats);

    // Get other statistics
    int samples = Sampler_getHistorySize();
    double avg = Sampler_getAverageReading();
    int dips = Sampler_getDips();
    double flash_freq = PwmLed_getFrequency();

    // Print first line with fixed width fields
    printf("#Smpl/s = %3d Flash @ %2.0fHz avg = %.3fV dips = %2d Smpl ms[%6.3f,%6.3f] avg %.3f/%d\n",
           samples,
           flash_freq,
           avg,
           dips,
           stats.minPeriodInMs,
           stats.maxPeriodInMs,
           stats.avgPeriodInMs,
           stats.numSamples);

    // Get and print 10 evenly spaced samples
    int size;
    double *history = Sampler_getHistory(&size);
    if (history && size > 0)
    {
        int step = (size > 10) ? size / 10 : 1;
        for (int i = 0; i < size && i / step < 10; i += step)
        {
            printf("%d:%.3f%s", i, history[i],
                   (i / step == 9 || i + step >= size) ? "\n" : " ");
        }
    }
    free(history);
}

static void *display_thread_function()
{
    while (!should_stop)
    {
        // Print display lines
        print_display_line();

        // Move current samples to history
        Sampler_moveCurrentDataToHistory();

        // Sleep for exactly 1 second
        sleep(1);
    }
    return NULL;
}

void TerminalDisplay_init(void)
{
    printf("Terminal Display - Initializing\n");
    assert(!is_initialized);

    // Reset flags
    should_stop = false;

    // Create display thread
    pthread_create(&display_thread, NULL, display_thread_function, NULL);

    is_initialized = true;
}

void TerminalDisplay_cleanup(void)
{
    printf("Terminal Display - Cleanup\n");
    assert(is_initialized);

    // Stop thread
    should_stop = true;
    pthread_join(display_thread, NULL);

    is_initialized = false;
}