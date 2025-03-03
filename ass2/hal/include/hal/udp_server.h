#ifndef _UDP_SERVER_H_
#define _UDP_SERVER_H_

#include <stdbool.h>

// Module that implements a UDP server for remote command control of the application.
// Runs a background thread that listens on a UDP socket (port 12345) for incoming commands.
// Supports the following commands:
// - help/?: Show help message with available commands
// - count: Get total number of samples taken
// - length: Get number of samples taken in the previous second
// - dips: Get number of light dips detected in the previous second
// - history: Get all voltage samples from the previous second
// - <enter>: Repeat the last command
// - stop: Exit the program
// Provides error responses for unknown commands.

// Begin/end the UDP server thread which listens for commands
void UdpServer_init(void);
void UdpServer_cleanup(void);

// Check if server received stop command
bool UdpServer_shouldStop(void);

#endif