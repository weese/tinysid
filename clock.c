/* File:   clock.c
 * Author: David Weese <david.weese@fu-berlin.de>
 */

#include <plib.h>
#include "clock.h"

/*void __ISR(_CORE_TIMER_VECTOR, ipl2) CoreTimerHandler(void)
{
    // update the period
    UpdateCoreTimer(CORE_TICK_RATE);

    

    // clear the interrupt flag
    mCTClearIntFlag();
}*/

void initClocks()
{
    SID_CLK_TRI = 0;
    OpenTimer2(T2_ON | T2_GATE_OFF | T2_PS_1_1 | T2_SOURCE_INT, SID_CLK_TICK_RATE); 
    ConfigIntTimer2(T2_INT_OFF); 
    OpenOC2(OC_ON | OC_TIMER2_SRC | OC_TOGGLE_PULSE, 0x00, 0x00);

}

void stopClocks()
{
    CloseOC2();
}

