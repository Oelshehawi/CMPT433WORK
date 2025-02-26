#ifndef _BTN_STATEMACHINE_H_
#define _BTN_STATEMACHINE_H_

// Initialize the button state machine.
// This starts a background thread that monitors the button.
void BtnStateMachine_init(void);

// Clean up the button state machine.
// This stops the background thread and releases resources.
void BtnStateMachine_cleanup(void);

// Get the current value of the button counter.
// This is thread-safe and can be called from any thread.
int BtnStateMachine_getValue(void);

#endif 