#include "sys.h"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "sid.h"

#define DATA_OUT_REG    0x13C
#define DATA_IN_REG     0x138
#define GPIO_OE_REG     0x134
//spru73h pg 4093
#define DATA_CLEAR_REG  0x190
//spru73h pg 4094
#define DATA_SET_REG    0x194

const int GpioMemBlockLength = 0xfff;

/**
 * Base addresses for GPIO blocks in memory
 */
const uint32_t gpioAddrs[] = { 0x44E07000, 0x4804C000, 0x481AC000, 0x481AE000 };
static uint32_t *gpios[4];
static int gpioFd;

// Phi2 clock frequency
static uint32_t cycles_per_second;
//const fp24p8_t PAL_CLOCK = ftofp24p8(985248.444);
//const fp24p8_t NTSC_OLD_CLOCK = ftofp24p8(1000000.0);
//const fp24p8_t NTSC_CLOCK = ftofp24p8(1022727.143);

// Replay counter variables
static uint16_t cia_timer;        // CIA timer A latch

/*
struct BeagleGoo::GPIOInfo BeagleGoo::gpioInfos[] =
	{
		{ (char*) "P8_3", 1, 6, 0, 0 },
		{ (char*) "P8_4", 1, 7, 0, 0 },
		{ (char*) "P8_5", 1, 2, 0, 0 },
		{ (char*) "P8_6", 1, 3, 0, 0 },
		{ (char*) "P8_7", 2, 2, 0, 0 },
		{ (char*) "P8_8", 2, 3, 0, 0 },
		{ (char*) "P8_9", 2, 5, 0, 0 },
		{ (char*) "P8_10", 2, 4, 0, 0 },
		{ (char*) "P8_11", 1, 13, 0, 0 },
		{ (char*) "P8_12", 1, 12, 0, 0 },
		{ (char*) "P8_13", 0, 23, 0, 0 },
		{ (char*) "P8_14", 0, 26, 0, 0 },
		{ (char*) "P8_15", 1, 15, 0, 0 },
		{ (char*) "P8_16", 1, 14, 0, 0 },
		{ (char*) "P8_17", 0, 27, 0, 0 },
		{ (char*) "P8_18", 2, 1, 0, 0 },
		{ (char*) "P8_19", 0, 22, 0, 0 },
		{ (char*) "P8_20", 1, 31, 0, 0 },
		{ (char*) "P8_21", 1, 30, 0, 0 },
		{ (char*) "P8_22", 1, 5, 0, 0 },
		{ (char*) "P8_23", 1, 4, 0, 0 },
		{ (char*) "P8_24", 1, 1, 0, 0 },
		{ (char*) "P8_25", 1, 0, 0, 0 },
		{ (char*) "P8_26", 1, 29, 0, 0 },
		{ (char*) "P8_27", 2, 22, 0, 0 },
		{ (char*) "P8_28", 2, 24, 0, 0 },
		{ (char*) "P8_29", 2, 23, 0, 0 },
		{ (char*) "P8_30", 2, 25, 0, 0 },
		{ (char*) "P8_31", 0, 10, 0, 0 },
		{ (char*) "P8_32", 0, 11, 0, 0 },
		{ (char*) "P8_33", 0, 9, 0, 0 },
		{ (char*) "P8_34", 2, 17, 0, 0 },
		{ (char*) "P8_35", 0, 8, 0, 0 },
		{ (char*) "P8_36", 2, 16, 0, 0 },
		{ (char*) "P8_37", 2, 14, 0, 0 },
		{ (char*) "P8_38", 2, 15, 0, 0 },
		{ (char*) "P8_39", 2, 12, 0, 0 },
		{ (char*) "P8_40", 2, 13, 0, 0 },
		{ (char*) "P8_41", 2, 10, 0, 0 },
		{ (char*) "P8_42", 2, 11, 0, 0 },
		{ (char*) "P8_43", 2, 8, 0, 0 },
		{ (char*) "P8_44", 2, 9, 0, 0 },
		{ (char*) "P8_45", 2, 6, 0, 0 },
		{ (char*) "P8_46", 2, 7, 0, 0 },

		{ (char*) "P9_11", 0, 30, 0, 0 },
		{ (char*) "P9_12", 1, 28, 0, 0 },
		{ (char*) "P9_13", 0, 31, 0, 0 },
		{ (char*) "P9_14", 1, 18, 0, 0 },
		{ (char*) "P9_15", 1, 16, 0, 0 },
		{ (char*) "P9_16", 1, 19, 0, 0 },
		{ (char*) "P9_17", 0, 5, 0, 0 },
		{ (char*) "P9_18", 0, 4, 0, 0 },
		{ (char*) "P9_19", 0, 13, 0, 0 },
		{ (char*) "P9_20", 0, 12, 0, 0 },
		{ (char*) "P9_21", 0, 3, 0, 0 },
		{ (char*) "P9_22", 0, 2, 0, 0 },
		{ (char*) "P9_23", 1, 17, 0, 0 },
		{ (char*) "P9_24", 0, 15, 0, 0 },
		{ (char*) "P9_25", 3, 21, 0, 0 },
		{ (char*) "P9_26", 0, 14, 0, 0 },
		{ (char*) "P9_27", 3, 19, 0, 0 },
		{ (char*) "P9_28", 3, 17, 0, 0 },
		{ (char*) "P9_29", 3, 15, 0, 0 },
		{ (char*) "P9_30", 3, 21, 0, 0 },
		{ (char*) "P9_31", 3, 14, 0, 0 },
		{ (char*) "P9_41", 0, 20, 0, 0 },
		{ (char*) "P9_42", 0, 7, 0, 0 } };
*/

int startGPIO()
{
	gpioFd = open("/dev/mem", O_RDWR | O_SYNC);
	if (gpioFd < 0)
	{
		printf("Can't open /dev/mem\n");
		return 1;
	}
    int i;
	for (i = 0; i < 4; i++)
	{
		gpios[i] = (uint32_t *) mmap(NULL, GpioMemBlockLength,
				PROT_READ | PROT_WRITE, MAP_SHARED, gpioFd, gpioAddrs[i]);
		if (gpios[i] == MAP_FAILED )
		{
			printf("GPIO Mapping failed for GPIO Module %i\n", i);
			return 1;
		}
	}
	return 0;
}

void stopGPIO()
{
    int i;
	for (i = 0; i < 4; i++)
		munmap(gpios[i], GpioMemBlockLength);
	close(gpioFd);
}

inline
void setDirection(uint32_t *base, int pin, bool output)
{
	if (output)
		base[GPIO_OE_REG / 4] &= ~(1 << pin);
	else
		base[GPIO_OE_REG / 4] |= 1 << pin;

}

inline
void setBit(uint32_t *base, int pin, bool enable)
{
	if (enable)
		base[DATA_SET_REG / 4] = 1 << pin;
	else
		base[DATA_CLEAR_REG / 4] = 1 << pin;
}

/*
 *  Start hardware SID connection
 */

void SIDInit()
{
	startGPIO();
	setDirection(gpios[0], 22, true);
	setDirection(gpios[0], 23, true);
}

/*
 *  Stop hardware SID connection
 */

void SIDExit()
{
	stopGPIO();
}

void SIDReset(uint32_t now)
{
}

/*
 *  Set replay frequency
 */

void SIDSetReplayFreq(int freq)
{
    cia_timer = cycles_per_second / freq - 1;
}

/*
 *  Set speed adjustment
 */

void SIDAdjustSpeed(int percent)
{
}

/*
 *  Write to CIA timer A (changes replay frequency)
 */

void cia_tl_write(uint8_t byte)
{
    cia_timer = (cia_timer & 0xff00) | byte;
}

void cia_th_write(uint8_t byte)
{
    cia_timer = (cia_timer & 0x00ff) | (byte << 8);
}

// Read from SID register
uint32_t sid_read(uint32_t adr, uint32_t now)
{
    return 0;
}

// Write to SID register
void sid_write(uint32_t adr, uint32_t byte, uint32_t now, bool rmw)
{
    // shift value to the latch
}

/*
int main()
{
    SIDInit();
	for(int i=0;i<10;++i)
	{
		setBit(gpios[0], 23, true);
		sleep(1);
		setBit(gpios[0], 23, false);	
		sleep(1);
	}
    SIDExit();
    return 0;
}
*/
