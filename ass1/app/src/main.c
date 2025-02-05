#include <stdio.h>
#include "hal/LEDStart.h"
#include "hal/joystick.h"
#include "reaction_timer.h"

int main() {
    // Run the reaction timer game
    reaction_timer();

    return 0;
}