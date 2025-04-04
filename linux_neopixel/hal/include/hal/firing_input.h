#ifndef FIRING_INPUT_H
#define FIRING_INPUT_H

#include <stdbool.h>


bool FiringInput_init(void);


void FiringInput_cleanup(void);

bool FiringInput_wasButtonPressed(void);

#endif // FIRING_INPUT_H 