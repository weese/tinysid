#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>

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

int startGPIO()
{
	gpioFd = open("/dev/mem", O_RDWR | O_SYNC);
	if (gpioFd < 0)
	{
		printf("Can't open /dev/mem\n");
		return 1;
	}
	for (int i = 0; i < 4; i++)
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
	for (int i = 0; i < 4; i++)
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
int main()
{
	startGPIO();
	setDirection(gpios[0], 23, true);
	for(int i=0;i<10;++i)
	{
		setBit(gpios[0], 23, true);
		sleep(1);
		setBit(gpios[0], 23, false);	
		sleep(1);
	}
	stopGPIO();
}


