// Module to sample light levels in the background (uses a thread).
//
// It continuously samples the light level, and stores it internally.
// It provides access to the samples it recorded during the _previous_
// complete second.
//
// The application will do a number of actions each second which must
// be synchronized (such as computing dips and printing to the screen).
// To make easy to work with the data, the app must call
// Sampler_moveCurrentDataToHistory() each second to trigger this
// module to move the current samples into the history.
#ifndef _SAMPLER_H_
#define _SAMPLER_H_

// Begin/end the background thread which samples light levels.
// Thread will sample every 1ms and maintain history/statistics
void Sampler_init(void);
void Sampler_cleanup(void);

// Read the current light sensor value
void Sampler_readLightSensor(void);

// Must be called once every 1s.
// Moves the samples that it has been collecting this second into
// the history, which makes the samples available for reads (below).
void Sampler_moveCurrentDataToHistory(void);

// Get the number of samples collected during the previous complete second.
int Sampler_getHistorySize(void);

// Get a copy of the samples in the sample history.
// Returns a newly allocated array and sets `size` to be the
// number of elements in the returned array (output-only parameter).
// The calling code must call free() on the returned pointer.
// Note: It provides both data and size to ensure consistency.
double *Sampler_getHistory(int *size);

// Get the average light level using exponential smoothing (not tied to the history).
// Uses 99.9% weight for previous average.
double Sampler_getAverageReading(void);

// Get the total number of light level samples taken so far.
long long Sampler_getNumSamplesTaken(void);

// Get the number of light dips detected in the previous second's data.
// A dip is when the light level drops 0.1V below the average and must rise
// back to within 0.07V of the average before another dip can be detected.
int Sampler_getDips(void);

// Get timing statistics for samples in the previous second
// All values are in milliseconds
void Sampler_getTimingStats(double *min_ms, double *max_ms, double *avg_ms);

#endif