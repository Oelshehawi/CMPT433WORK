// UDP Server for BeatBox application
// Provides remote control functionality via UDP sockets
#ifndef _UDP_SERVER_H_
#define _UDP_SERVER_H_

#include <stdbool.h>

// Default UDP port for the server to listen on
#define UDP_SERVER_PORT 12345

// Starts a listener thread on the specified port
void UdpServer_init(void);

void UdpServer_cleanup(void);

// Check if the application should stop (based on UDP command)
// Returns true if a shutdown command was received
bool UdpServer_shouldStop(void);

#endif 