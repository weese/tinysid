/* File:   test.c
 * Author: David Weese <david.weese@fu-berlin.de>
 */

#include <stdio.h>
#include "HardwareProfile.h"
#include "clock.h"

/* Function int main(int argc, char **argv) {{{ */
int main(int argc, char **argv)
{
        // Set all analog pins to be digital I/O
    AD1PCFG = 0xFFFF;
    
    // Configure the proper PB frequency and the number of wait states
        SYSTEMConfigPerformance(80000000L);
        
        // Open up the core timer at our 1ms rate
        OpenCoreTimer(CORE_TICK_RATE);
        
    // set up the core timer interrupt with a prioirty of 2 and zero sub-priority
        mConfigIntCoreTimer((CT_INT_ON | CT_INT_PRIOR_2 | CT_INT_SUB_PRIOR_0));
        
    // enable multi-vector interrupts
        INTEnableSystemMultiVectoredInt();
        
        // Turn off JTAG so we get the pins back
        mJTAGPortEnable(0);
        
//    //Initialize all of the LED pins
        mInitAllLEDs();

    initClocks();
    while(1);

	return 0;
} /* }}} */


/* Modeline for ViM {{{
 * vim:set ts=4:
 * vim600:fdm=marker fdl=0 fdc=3:
 * }}} */


