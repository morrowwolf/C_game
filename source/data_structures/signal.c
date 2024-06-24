
#include "signal.h"

void Signal_Init(Signal *signal, long initialSignal)
{
    ZeroMemory(signal, sizeof(Signal));
    InterlockedExchange(&signal->signal, initialSignal);
}

void Signal_SetSignal(Signal *signal, long settingSignal)
{
    InterlockedExchange(&signal->signal, settingSignal);
    WakeByAddressAll((void *)&signal->signal);
}

void Signal_WaitForSignal(Signal *signal, long waitForSignal)
{
    while (signal->signal != waitForSignal)
    {
        WaitOnAddress(&signal->signal, &waitForSignal, sizeof(signal->signal), INFINITE);
    }
}

__int8 Signal_TrySignal(Signal *signal, long waitForSignal)
{
    if (signal->signal == waitForSignal)
    {
        return TRUE;
    }

    return FALSE;
}
