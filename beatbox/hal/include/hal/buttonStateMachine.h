// This file is used to handle the state machine 
// for the rotary encoder button press
#ifndef _BTN_STATEMACHINE_H_
#define _BTN_STATEMACHINE_H_

#include <stdbool.h>

// Button action callback type
typedef void (*ButtonAction)(void);

void BtnStateMachine_init(void);
void BtnStateMachine_cleanup(void);

int BtnStateMachine_getValue(void);

// New function to set custom button action
void BtnStateMachine_setAction(ButtonAction action);

// TODO: This should be on a background thread (internal?)
void BtnStateMachine_doState();

#endif