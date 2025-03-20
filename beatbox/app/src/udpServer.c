#include "udpServer.h"
#include "beatPlayer.h"
#include "hal/audioMixer.h"
#include "hal/rotaryEncoder.h"
#include "drumSounds.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <signal.h>
#include <ctype.h>

#define MAX_BUFFER_SIZE 1024

// Internal variables
static pthread_t serverThreadId;
static bool isRunning = false;
static bool shouldStopApplication = false;
static int socketFd = -1;

// For signal handling
static struct sigaction old_sa;

// Command formats supported:
// 1. command,param (comma-separated for test client): volume,80
// 2. command param (space-separated for web UI): volume 80
//
// Supported commands:
// mode <0-2>: Change the drum-beat mode (0=none, 1=rock, 2=custom)
// volume <0-100>: Change the volume
// tempo <40-300>: Change the tempo (BPM)
// play <0|1|2>: Play the specified drum sound once (0=base, 1=hihat, 2=snare)
// stop: Shut down the entire application

static void *serverThread(void *arg);
static void processCommand(const char *command, struct sockaddr_in *clientAddr, socklen_t addrLen);

// Forward SIGINT to the parent process
static void udpSignalHandler(int sig)
{
    if (sig == SIGINT)
    {
        // Restore the original signal handler
        if (sigaction(SIGINT, &old_sa, NULL) == -1)
        {
            perror("Error restoring original signal handler");
        }

        // Now pass the signal to the original handler
        if (old_sa.sa_handler != SIG_IGN && old_sa.sa_handler != SIG_DFL)
        {
            old_sa.sa_handler(sig);
        }
        else if (old_sa.sa_handler == SIG_DFL)
        {
            // Default action is to terminate process
            signal(sig, SIG_DFL);
            raise(sig);
        }
    }
}

void UdpServer_init(void)
{
    // Prevent double initialization
    if (isRunning)
    {
        return;
    }

    // Get the original signal handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = udpSignalHandler;

    if (sigaction(SIGINT, NULL, &old_sa) == -1)
    {
        perror("Error getting original signal handler");
    }

    // Don't install a new handler - keep using the one in main.c

    // Create thread to handle UDP server
    int result = pthread_create(&serverThreadId, NULL, serverThread, NULL);
    if (result != 0)
    {
        printf("Error: Unable to create UDP server thread\n");
        return;
    }

    isRunning = true;
    printf("UDP Server initialized on port %d\n", UDP_SERVER_PORT);
}

void UdpServer_cleanup(void)
{
    if (!isRunning)
    {
        return;
    }

    isRunning = false;

    // Unblock the recvfrom call in the server thread
    if (socketFd != -1)
    {
        // Send a signal to ourselves to wake up the recvfrom
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        addr.sin_port = htons(UDP_SERVER_PORT);

        int tempSock = socket(AF_INET, SOCK_DGRAM, 0);
        if (tempSock >= 0)
        {
            char *msg = "STOP";
            sendto(tempSock, msg, strlen(msg), 0, (struct sockaddr *)&addr, sizeof(addr));
            close(tempSock);
        }

        // Close the socket
        close(socketFd);
        socketFd = -1;
    }

    // Wait for thread to exit
    pthread_join(serverThreadId, NULL);
    printf("UDP Server cleaned up\n");
}

static void processCommand(const char *command, struct sockaddr_in *clientAddr, socklen_t addrLen)
{
    char response[MAX_BUFFER_SIZE];
    char cmdCopy[MAX_BUFFER_SIZE];
    strncpy(cmdCopy, command, MAX_BUFFER_SIZE - 1);
    cmdCopy[MAX_BUFFER_SIZE - 1] = '\0';

    // Make a copy for debugging
    char originalCommand[MAX_BUFFER_SIZE];
    strncpy(originalCommand, command, MAX_BUFFER_SIZE - 1);
    originalCommand[MAX_BUFFER_SIZE - 1] = '\0';

    // Determine command format (comma or space separator)
    char *separator = strchr(cmdCopy, ',');
    char *cmd, *param;

    if (separator)
    {
        // Comma-separated format (command,param)
        *separator = '\0';
        cmd = cmdCopy;
        param = separator + 1;
    }
    else
    {
        // Space-separated format (command param)
        cmd = strtok(cmdCopy, " \t");
        param = strtok(NULL, " \t");
    }

    if (cmd == NULL)
    {
        return;
    }

    // Convert command to lowercase for case-insensitive comparison
    for (char *p = cmd; *p; p++)
    {
        *p = tolower(*p);
    }

    if (strcmp(cmd, "mode") == 0)
    {
        // Handle mode with or without parameter
        if (param != NULL && strcmp(param, "null") != 0)
        {
            int mode = atoi(param);
            if (mode >= 0 && mode <= 2)
            {
                BeatPlayer_setMode(mode);
                // Just sync the beatPlayer mode; the rotary encoder will get updated via main.c
            }
        }
        // Always respond with current mode for both queries and settings
        BeatMode_t currentMode = BeatPlayer_getMode();
        snprintf(response, MAX_BUFFER_SIZE, "%d", currentMode);
    }
    else if (strcmp(cmd, "volume") == 0)
    {
        // Handle volume with or without parameter
        if (param != NULL && strcmp(param, "null") != 0)
        {
            int volume = atoi(param);
            if (volume >= 0 && volume <= 100)
            {
                AudioMixer_setVolume(volume);
            }
        }
        // Always respond with current volume for both queries and settings
        int currentVolume = AudioMixer_getVolume();
        snprintf(response, MAX_BUFFER_SIZE, "%d", currentVolume);
    }
    else if (strcmp(cmd, "tempo") == 0)
    {
        // Handle tempo with or without parameter
        if (param != NULL && strcmp(param, "null") != 0)
        {
            int tempo = atoi(param);
            if (tempo >= 40 && tempo <= 300)
            {
                BeatPlayer_setTempo(tempo);
                // Also update the rotary encoder to maintain consistency
                RotaryEncoder_setBPM(tempo);
            }
        }
        // Always respond with current tempo for both queries and settings
        int currentTempo = BeatPlayer_getTempo();
        snprintf(response, MAX_BUFFER_SIZE, "%d", currentTempo);
    }
    else if (strcmp(cmd, "play") == 0 && param != NULL)
    {
        // Handle both string and numeric parameters for play
        if (strcmp(param, "base") == 0 || strcmp(param, "0") == 0)
        {
            AudioMixer_queueSound(DrumSounds_getSound(DRUM_SOUND_BASE));
            snprintf(response, MAX_BUFFER_SIZE, "OK");
        }
        else if (strcmp(param, "hihat") == 0 || strcmp(param, "1") == 0)
        {
            AudioMixer_queueSound(DrumSounds_getSound(DRUM_SOUND_HIHAT));
            snprintf(response, MAX_BUFFER_SIZE, "OK");
        }
        else if (strcmp(param, "snare") == 0 || strcmp(param, "2") == 0)
        {
            AudioMixer_queueSound(DrumSounds_getSound(DRUM_SOUND_SNARE));
            snprintf(response, MAX_BUFFER_SIZE, "OK");
        }
        else
        {
            snprintf(response, MAX_BUFFER_SIZE, "ERROR: Unknown drum sound");
        }
    }
    else if (strcmp(cmd, "shutdown") == 0 || strcmp(cmd, "stop") == 0)
    {
        shouldStopApplication = true;
        snprintf(response, MAX_BUFFER_SIZE, "OK");
    }
    else if (strcmp(cmd, "STOP") == 0)
    {
        // Internal command to unblock recvfrom
        return;
    }
    else
    {
        // Limit the command length in the error message to avoid buffer overflow
        char cmdShort[20];
        strncpy(cmdShort, cmd, sizeof(cmdShort) - 1);
        cmdShort[sizeof(cmdShort) - 1] = '\0';
        snprintf(response, MAX_BUFFER_SIZE, "ERROR: Unknown command '%s'", cmdShort);
    }

    // Send response
    if (clientAddr && addrLen > 0)
    {
        sendto(socketFd, response, strlen(response), 0,
               (struct sockaddr *)clientAddr, addrLen);
    }
}

static void *serverThread(void *arg)
{
    // Suppress unused parameter warning
    (void)arg;

    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    char buffer[MAX_BUFFER_SIZE];

    // Create UDP socket
    socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd < 0)
    {
        printf("Error: Unable to create socket for UDP server\n");
        return NULL;
    }

    // Set socket to reuse address
    int enable = 1;
    if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        printf("Error: setsockopt(SO_REUSEADDR) failed\n");
    }

    // Set up server address
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(UDP_SERVER_PORT);

    // Bind socket to address
    if (bind(socketFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        printf("Error: Unable to bind UDP server socket\n");
        close(socketFd);
        socketFd = -1;
        return NULL;
    }

    // Main receive loop
    while (isRunning)
    {
        // Receive data
        memset(&clientAddr, 0, sizeof(clientAddr));
        addrLen = sizeof(clientAddr);

        int numBytes = recvfrom(socketFd, buffer, MAX_BUFFER_SIZE - 1, 0,
                                (struct sockaddr *)&clientAddr, &addrLen);

        if (!isRunning)
        {
            break; // Exit if we've been asked to stop
        }

        if (numBytes < 0)
        {
            if (isRunning)
            {
                perror("Error receiving UDP packet");
            }
            continue;
        }

        // Null-terminate the received data
        buffer[numBytes] = '\0';

        // Process the command
        processCommand(buffer, &clientAddr, addrLen);
    }

    return NULL;
}

bool UdpServer_shouldStop(void)
{
    return shouldStopApplication;
}