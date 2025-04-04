#ifndef SHUTDOWN_INPUT_H
#define SHUTDOWN_INPUT_H

#include <stdbool.h>


bool ShutdownInput_init(void);


void ShutdownInput_cleanup(void);

bool ShutdownInput_isShutdownRequested(void);

#endif // SHUTDOWN_INPUT_H 