// Playback sounds in real time, allowing multiple simultaneous wave files
// to be mixed together and played without jitter.
// Note: Generates low latency audio on BeagleBone Black; higher latency found on host.
#include "hal/audioMixer.h"
#include <alsa/asoundlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <limits.h>
#include <alloca.h> // needed for mixer
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Forward declaration of Terminal display function
void TerminalDisplay_markAudioEvent(void);

static snd_pcm_t *handle;

#define DEFAULT_VOLUME 80

#define SAMPLE_RATE 44100
#define NUM_CHANNELS 1
#define SAMPLE_SIZE (sizeof(short)) // bytes per sample
// Sample size note: This works for mono files because each sample ("frame') is 1 value.
// If using stereo files then a frame would be two samples.

static unsigned long playbackBufferSize = 0;
static short *playbackBuffer = NULL;

// Currently active (waiting to be played) sound bites
#define MAX_SOUND_BITES 30
typedef struct
{
    // A pointer to a previously allocated sound bite (wavedata_t struct).
    // Note that many different sound-bite slots could share the same pointer
    // (overlapping cymbal crashes, for example)
    wavedata_t *pSound;

    // The offset into the pData of pSound. Indicates how much of the
    // sound has already been played (and hence where to start playing next).
    int location;
} playbackSound_t;
static playbackSound_t soundBites[MAX_SOUND_BITES];

// Playback threading
void *playbackThread(void *arg);
static _Bool stopping = false;
static pthread_t playbackThreadId;
static pthread_mutex_t audioMutex = PTHREAD_MUTEX_INITIALIZER;

static int volume = 0;

void AudioMixer_init(void)
{
    AudioMixer_setVolume(DEFAULT_VOLUME);

    // Initialize the currently active sound-bites being played
    for (int i = 0; i < MAX_SOUND_BITES; i++)
    {
        soundBites[i].pSound = NULL;
        soundBites[i].location = 0;
    }

    // Open the PCM output
    int err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0)
    {
        printf("Playback open error: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    // Configure parameters of PCM output
    err = snd_pcm_set_params(handle,
                             SND_PCM_FORMAT_S16_LE,
                             SND_PCM_ACCESS_RW_INTERLEAVED,
                             NUM_CHANNELS,
                             SAMPLE_RATE,
                             1,      // Allow software resampling
                             50000); // 0.05 seconds per buffer
    if (err < 0)
    {
        printf("Playback open error: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    // Allocate this software's playback buffer to be the same size as the
    // the hardware's playback buffers for efficient data transfers.
    // ..get info on the hardware buffers:
    unsigned long unusedBufferSize = 0;
    snd_pcm_get_params(handle, &unusedBufferSize, &playbackBufferSize);
    // ..allocate playback buffer:
    playbackBuffer = malloc(playbackBufferSize * sizeof(*playbackBuffer));

    // Launch playback thread:
    pthread_create(&playbackThreadId, NULL, playbackThread, NULL);
}

// Client code must call AudioMixer_freeWaveFileData to free dynamically allocated data.
void AudioMixer_readWaveFileIntoMemory(char *fileName, wavedata_t *pSound)
{
    assert(pSound);

    // The PCM data in a wave file starts after the header:
    const int PCM_DATA_OFFSET = 44;

    // Open the wave file
    FILE *file = fopen(fileName, "r");
    if (file == NULL)
    {
        fprintf(stderr, "ERROR: Unable to open file %s.\n", fileName);
        exit(EXIT_FAILURE);
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    int sizeInBytes = ftell(file) - PCM_DATA_OFFSET;
    pSound->numSamples = sizeInBytes / SAMPLE_SIZE;

    // Search to the start of the data in the file
    fseek(file, PCM_DATA_OFFSET, SEEK_SET);

    // Allocate space to hold all PCM data
    pSound->pData = malloc(sizeInBytes);
    if (pSound->pData == 0)
    {
        fprintf(stderr, "ERROR: Unable to allocate %d bytes for file %s.\n",
                sizeInBytes, fileName);
        exit(EXIT_FAILURE);
    }

    // Read PCM data from wave file into memory
    int samplesRead = fread(pSound->pData, SAMPLE_SIZE, pSound->numSamples, file);
    if (samplesRead != pSound->numSamples)
    {
        fprintf(stderr, "ERROR: Unable to read %d samples from file %s (read %d).\n",
                pSound->numSamples, fileName, samplesRead);
        exit(EXIT_FAILURE);
    }

    fclose(file);
}

void AudioMixer_freeWaveFileData(wavedata_t *pSound)
{
    pSound->numSamples = 0;
    free(pSound->pData);
    pSound->pData = NULL;
}

void AudioMixer_queueSound(wavedata_t *pSound)
{
    // Ensure we are only being asked to play "good" sounds:
    assert(pSound->numSamples > 0);
    assert(pSound->pData);

    // Insert the sound by searching for an empty sound bite spot

    // Lock the mutex to ensure thread safety
    pthread_mutex_lock(&audioMutex);

    // Search for an empty slot
    int i;
    for (i = 0; i < MAX_SOUND_BITES; i++)
    {
        if (soundBites[i].pSound == NULL)
        {
            // Found an empty slot, use it
            soundBites[i].pSound = pSound;
            soundBites[i].location = 0;
            break;
        }
    }

    // Unlock the mutex
    pthread_mutex_unlock(&audioMutex);

    // If we didn't find an empty slot, print an error
    if (i == MAX_SOUND_BITES)
    {
        fprintf(stderr, "ERROR: No free slots available to queue sound.\n");
        return;
    }
}

void AudioMixer_cleanup(void)
{
    printf("Stopping audio...\n");

    // Stop the PCM generation thread
    stopping = true;
    pthread_join(playbackThreadId, NULL);

    // Shutdown the PCM output, allowing any pending sound to play out (drain)
    snd_pcm_drain(handle);
    snd_pcm_close(handle);

    // Free playback buffer
    // (note that any wave files read into wavedata_t records must be freed
    //  in addition to this by calling AudioMixer_freeWaveFileData() on that struct.)
    free(playbackBuffer);
    playbackBuffer = NULL;

    printf("Done stopping audio...\n");
    fflush(stdout);
}

int AudioMixer_getVolume()
{
    // Return the cached volume; good enough unless someone is changing
    // the volume through other means and the cached value is out of date.
    return volume;
}

// Function copied from:
// http://stackoverflow.com/questions/6787318/set-alsa-master-volume-from-c-code
// Written by user "trenki".
void AudioMixer_setVolume(int newVolume)
{
    // Ensure volume is reasonable; If so, cache it for later getVolume() calls.
    if (newVolume < 0 || newVolume > AUDIOMIXER_MAX_VOLUME)
    {
        printf("ERROR: Volume must be between 0 and 100.\n");
        return;
    }
    volume = newVolume;

    long min, max;
    snd_mixer_t *mixerHandle;
    snd_mixer_selem_id_t *sid;
    const char *card = "default";
    // const char *selem_name = "PCM";	// For ZEN cape
    const char *selem_name = "PCM"; // For USB Audio

    snd_mixer_open(&mixerHandle, 0);
    snd_mixer_attach(mixerHandle, card);
    snd_mixer_selem_register(mixerHandle, NULL, NULL);
    snd_mixer_load(mixerHandle);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);
    snd_mixer_elem_t *elem = snd_mixer_find_selem(mixerHandle, sid);

    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    snd_mixer_selem_set_playback_volume_all(elem, volume * max / 100);

    snd_mixer_close(mixerHandle);
}

// Fill the buff array with new PCM values to output.
//    buff: buffer to fill with new PCM data from sound bites.
//    size: the number of *values* to store into buff
static void fillPlaybackBuffer(short *buff, int size)
{

    // Wipe the buffer to all 0's to clear any previous PCM data
    memset(buff, 0, size * SAMPLE_SIZE);

    // Lock the mutex for thread safety
    pthread_mutex_lock(&audioMutex);

    // Loop through each slot in soundBites[]
    for (int i = 0; i < MAX_SOUND_BITES; i++)
    {
        // Check if this sound bite slot is used
        if (soundBites[i].pSound != NULL)
        {
            // Grab the sound and current location for efficiency
            wavedata_t *sound = soundBites[i].pSound;
            int location = soundBites[i].location;

            // Calculate how many samples we can add from this sound bite
            int samplesRemaining = sound->numSamples - location;
            int samplesToAdd = (samplesRemaining < size) ? samplesRemaining : size;

            // Add samples to the playback buffer, mixing with what's already there
            for (int j = 0; j < samplesToAdd; j++)
            {
                // Mix the sample with the existing buffer
                int mixed_sample = buff[j] + sound->pData[location + j];

                // Clip to prevent overflow
                if (mixed_sample > SHRT_MAX)
                {
                    mixed_sample = SHRT_MAX;
                }
                else if (mixed_sample < SHRT_MIN)
                {
                    mixed_sample = SHRT_MIN;
                }

                // Store back into buffer
                buff[j] = (short)mixed_sample;
            }

            // Update the location in the sound bite
            location += samplesToAdd;

            // If we've played the entire sound, mark the slot as free
            if (location >= sound->numSamples)
            {
                soundBites[i].pSound = NULL;
            }
            else
            {
                // Otherwise, update the location for next time
                soundBites[i].location = location;
            }
        }
    }

    // Unlock the mutex
    pthread_mutex_unlock(&audioMutex);
}

void *playbackThread(void *arg)
{
    // Suppress unused parameter warning
    (void)arg;

    while (!stopping)
    {
        // Generate next block of audio
        fillPlaybackBuffer(playbackBuffer, playbackBufferSize);

        // Mark audio event for timing statistics
        TerminalDisplay_markAudioEvent();

        // Output the audio
        snd_pcm_sframes_t frames = snd_pcm_writei(handle,
                                                  playbackBuffer, playbackBufferSize);

        // Check for (and handle) possible error conditions on output
        if (frames < 0)
        {
            fprintf(stderr, "AudioMixer: writei() returned %li\n", frames);
            frames = snd_pcm_recover(handle, frames, 1);
        }
        if (frames < 0)
        {
            fprintf(stderr, "ERROR: Failed writing audio with snd_pcm_writei(): %li\n",
                    frames);
            exit(EXIT_FAILURE);
        }
        if (frames > 0 && frames < (snd_pcm_sframes_t)playbackBufferSize)
        {
            printf("Short write (expected %lu, wrote %li)\n",
                   playbackBufferSize, frames);
        }
    }

    return NULL;
}