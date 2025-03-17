#ifndef _UDP_SERVER_H_
#define _UDP_SERVER_H_

#include <stdbool.h>

// UDP Server for BeatBox application
// Provides remote control functionality via UDP sockets

// Default UDP port for the server to listen on
#define UDP_SERVER_PORT 12345

// Initialize the UDP server
// Starts a listener thread on the specified port
void UdpServer_init(void);

// Clean up the UDP server resources
void UdpServer_cleanup(void);

// Check if the application should stop (based on UDP command)
// Returns true if a shutdown command was received
bool UdpServer_shouldStop(void);

#endif // _UDP_SERVER_H_ 