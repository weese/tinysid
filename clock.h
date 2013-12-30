// File:   clock.h
// Author: David Weese <david.weese@fu-berlin.de>

#ifndef CLOCK_H
#define CLOCK_H

#include "HardwareProfile.h"

#define SID_CLK             LATAbits.LATA0
#define SID_CLK_TRI         TRISAbits.TRISA0

// Let compile time pre-processor calculate the CORE_TICK_PERIOD
#define SID_CLK_TICK_RATE   (GetSystemClock()/2/1000000)

void initClocks();

#endif  // ifndef CLOCK_H


