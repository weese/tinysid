#include "sys.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <time.h>
#include <fcntl.h>
//#include <unistd.h>
//#include <sys/mman.h>

#include "sid.h"
#include "prefs.h"


#define LATCH_CP    23
#define LATCH_ADDR  26
#define LATCH_DATA  27
#define LATCH_DELAY 200

#define SID_RW      2   // low=write, HIGH=read
#define SID_BIT_A0  3   // first bit of address
#define SID_DELAY   5000

#define SID_RES     30
#define SID_CS      22


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
static uint32_t volatile *gpios[4];
static int gpioFd;

// Phi2 clock frequency
static uint32_t cycles_per_second;
const uint32_t PAL_CLOCK = 985248;
const uint32_t NTSC_OLD_CLOCK = 1000000;
const uint32_t NTSC_CLOCK = 1022727;

// Replay counter variables
static uint16_t cia_timer;      // CIA timer A latch
static int speed_adjust;        // Speed adjustment in percent

// Clock frequency changed
void SIDClockFreqChanged() {}

int startGPIO()
{
	gpioFd = open("/dev/mem", O_RDWR | O_SYNC);
	if (gpioFd < 0)
	{
		printf("Can't open /dev/mem\n");
		return 1;
	}
    int i;
	for (i = 0; i < 4; ++i)
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
	for (i = 0; i < 4; ++i)
		munmap((void*)gpios[i], GpioMemBlockLength);
	close(gpioFd);
}

inline
void pinSetDirection(int pin, bool output)
{
	if (output)
		gpios[pin >> 5][GPIO_OE_REG / 4] &= ~(1 << (pin & 31));
	else
		gpios[pin >> 5][GPIO_OE_REG / 4] |= 1 << (pin & 31);

}

inline
void pinSetValue(int pin, bool enable)
{
	if (enable)
		gpios[pin >> 5][DATA_SET_REG / 4] = 1 << (pin & 31);
	else
		gpios[pin >> 5][DATA_CLEAR_REG / 4] = 1 << (pin & 31);
}

void delay(uint32_t nanoSec)
{
    volatile uint32_t x, y = 0;
    for (x = 0; x < nanoSec; x += 2)
        ++y;
//    struct timespec delay;
//    delay.tv_sec = 0;
//    delay.tv_nsec = nanoSec;
//    nanosleep(&delay, NULL);  
}

inline
void latchWrite(uint8_t addr, uint8_t data)
{
    uint8_t i;
    for (i = 0; i < 8; ++i)
    {
        pinSetValue(LATCH_CP, false);
        pinSetValue(LATCH_ADDR, addr >> 7);
        pinSetValue(LATCH_DATA, data >> 7);
        delay(LATCH_DELAY);
        pinSetValue(LATCH_CP, true);
        delay(LATCH_DELAY);
	addr <<= 1;
	data <<= 1;
    }
}

static void set_cycles_per_second(const char *to)
{
    if (strncmp(to, "6569", 4) == 0)
        cycles_per_second = PAL_CLOCK;
    else if (strcmp(to, "6567R5") == 0)
        cycles_per_second = NTSC_OLD_CLOCK;
    else
        cycles_per_second = NTSC_CLOCK;
}

static void prefs_victype_changed(const char *name, const char *from, const char *to)
{
    set_cycles_per_second(to);
    SIDClockFreqChanged();
}

static void prefs_speed_changed(const char *name, int32_t from, int32_t to)
{
    speed_adjust = to;
}

/*
 *  Start hardware SID connection
 */

void SIDInit()
{
	startGPIO();
	pinSetDirection(LATCH_CP, true);
	pinSetDirection(LATCH_ADDR, true);
	pinSetDirection(LATCH_DATA, true);
	pinSetDirection(SID_RES, true);
	pinSetDirection(SID_CS, true);

    set_cycles_per_second(PrefsFindString("victype", 0));
    speed_adjust = PrefsFindInt32("speed");
    PrefsSetCallbackString("victype", prefs_victype_changed);
    PrefsSetCallbackInt32("speed", prefs_speed_changed);
}

void SIDReset(uint32_t now)
{
    pinSetValue(SID_RES, false);
    delay(10000);
    pinSetValue(SID_RES, true);
}

/*
 *  Stop hardware SID connection
 */

void SIDExit()
{
    stopGPIO();
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
    PrefsReplaceInt32("speed", percent);
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
    //printf("sid_write %02x to %04x at cycle %d\n", byte, adr, now);
    pinSetValue(SID_CS, true);
    latchWrite(adr << SID_BIT_A0, byte);
    pinSetValue(SID_CS, false);
    delay(SID_DELAY);
    pinSetValue(SID_CS, true);
}

uint32_t cia_period_usec()
{
    return ((uint32_t)(cia_timer + 1) * 50000ul) / ((cycles_per_second * speed_adjust) / 2000);
}
