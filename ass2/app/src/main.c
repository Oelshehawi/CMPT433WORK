// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include "hal/sampler.h"
#include "hal/udp_server.h"
#include "hal/periodTimer.h"
#include "draw_stuff.h"

static volatile bool keep_running = true;

void handle_signal()
{
    keep_running = false;
}

static void print_sample_line(void)
{
    // Get timing statistics
    Period_statistics_t stats;
    Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, &stats);

    // Get other statistics
    int samples = Sampler_getHistorySize();
    double avg = Sampler_getAverageReading();
    int dips = Sampler_getDips();

    // Print first line with fixed width fields
    printf("#Smpl/s = %3d Flash @ %2dHz avg = %.3fV dips = %2d Smpl ms[%6.3f,%6.3f] avg %.3f/%d\n",
           samples,
           0, // Flash rate placeholder
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

int main()
{
    printf("Starting Light Sensor Sampling...\n");

    // Setup signal handling
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Initialize all modules; HAL modules first
    Period_init();
    Sampler_init();
    UdpServer_init();
    DrawStuff_init();

    // Main loop: Update display every second
    while (keep_running && !UdpServer_shouldStop())
    {
        // Print statistics and samples
        print_sample_line();

        // Move current samples to history every second
        Sampler_moveCurrentDataToHistory();

        sleep(1); // Wait for next second
    }

    // Cleanup all modules (HAL modules last)
    DrawStuff_cleanup();
    UdpServer_cleanup();
    Sampler_cleanup();
    Period_cleanup();

    printf("Program terminated.\n");
    return 0;
}