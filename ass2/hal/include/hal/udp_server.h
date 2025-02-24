#ifndef _UDP_SERVER_H_
#define _UDP_SERVER_H_

#include <stdbool.h>

// Begin/end the UDP server thread which listens for commands
void UdpServer_init(void);
void UdpServer_cleanup(void);

// Check if server received stop command
bool UdpServer_shouldStop(void);

#endif