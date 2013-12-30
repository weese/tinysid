#include "HardwareProfile.h"

// Decrements every 1 ms.
volatile uint32_t usTimer;

void __ISR(_CORE_TIMER_VECTOR, ipl2) CoreTimerHandler(void)
{
    mPORTAToggleBits(BIT_0);

    // clear the interrupt flag
    mCTClearIntFlag();
	
	if (usTimer)
		--usTimer;

    // update the period
    UpdateCoreTimer(CORE_TICK_RATE);
}
