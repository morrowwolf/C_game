#pragma once
#ifndef EVENT_H_
#define EVENT_H_

#include <windows.h>

#pragma comment(lib, "Synchronization.lib")

#define SIGNAL_OFF 0
#define SIGNAL_ON 1

#define MILLISECONDS_TO_HUNDREDNANOSECONDS(milliseconds) (milliseconds * 10000LL)
#define HUNDREDNANOSECONDS_TO_MILLISECONDS(nanoseconds) (nanoseconds / 10000LL)

typedef struct Signal
{
    volatile long signal;
} Signal;

void Signal_Init(Signal *signal, long initialSignal);

void Signal_SetSignal(Signal *signal, long settingSignal);

void Signal_WaitForSignal(Signal *signal, long waitForSignal);
_int8 Signal_TrySignal(Signal *signal, long waitForSignal);

#endif
