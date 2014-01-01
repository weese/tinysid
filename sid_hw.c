#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <time.h>
#include <fcntl.h>
//#include <unistd.h>
//#include <sys/mman.h>

#include "stm32f4_discovery.h"

#include "sid.h"
#include "prefs.h"

#define DATA_OUT_REG    0x13C
#define DATA_IN_REG     0x138
#define GPIO_OE_REG     0x134
//spru73h pg 4093
#define DATA_CLEAR_REG  0x190
//spru73h pg 4094
#define DATA_SET_REG    0x194

volatile uint8_t pending_IRQs;

const int GpioMemBlockLength = 0xfff;

/**
 * Base addresses for GPIO blocks in memory
 */

// Phi2 clock frequency
static uint32_t cycles_per_second;
const uint32_t PAL_CLOCK = 985248;
const uint32_t NTSC_OLD_CLOCK = 1000000;
const uint32_t NTSC_CLOCK = 1022727;

// Replay counter variables
static uint16_t cia_timer;      // CIA timer A latch
static uint32_t speed_adjust;   // Speed adjustment in percent

// Clock frequency changed
void SIDClockFreqChanged() {}

#define SID_DELAY   50

#define SID_PORT    GPIOB       // PB0-PB7 data, PB8-PB12 addr
#define SID_RW      GPIO_Pin_13 // low=write, HIGH=read, PB13
#define SID_CS      GPIO_Pin_14 // PB14
#define SID_RES     GPIO_Pin_15 // PB15

//uint32_t cia_period_usec()
//{
//    return ((uint32_t)(cia_timer + 1) * 50000ul) / ((cycles_per_second * speed_adjust) / 2000);
//}

void startGPIO()
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_StructInit(&GPIO_InitStructure);

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void stopGPIO()
{
}

void TIM4_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
        pending_IRQs |= IRQ_CIA_A;
    }
}

/**
  * @brief  Configure the TIM3 Ouput Channels to output the 1MHz SID clock. TIM4 is used for CIA interrupts.
  * @param  None
  * @retval None
  */
void startTimers(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBase_InitStructure;
    TIM_OCInitTypeDef TIM_OC_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    uint16_t Prescaler1MHz = ((SystemCoreClock / 2) / 1000000) - 1;
    uint16_t Prescaler2MHz = ((SystemCoreClock / 2) / 2000000) - 1;

    // TIM3 and TIM4 clock enable
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3 | RCC_APB1Periph_TIM4, ENABLE);

    // GPIOC clock enable
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

    // GPIOC Configuration: TIM3 CH1 (PC6)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_TIM3);

    // Timer 3: 2 cycles at 2MHz

    TIM_TimeBase_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBase_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBase_InitStructure.TIM_Period = 1;
    TIM_TimeBase_InitStructure.TIM_Prescaler = Prescaler2MHz;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBase_InitStructure);

    // PWM1 Mode configuration: Channel1
    TIM_OC_InitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OC_InitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OC_InitStructure.TIM_Pulse = 0;
    TIM_OC_InitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM3, &TIM_OC_InitStructure);
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM3->CCR1 = 1;

    // Timer 4: 16bit counter at 1MHz

    TIM_TimeBase_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBase_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBase_InitStructure.TIM_Period = 65535;
    TIM_TimeBase_InitStructure.TIM_Prescaler = Prescaler1MHz;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBase_InitStructure);

    // Enable timer 4 interrupt
    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
    NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
    NVIC_Init(&NVIC_InitStructure);

    // Enable timers
    TIM_ARRPreloadConfig(TIM3, ENABLE);
    TIM_Cmd(TIM3, ENABLE);
    TIM_Cmd(TIM4, ENABLE);
}

void delay(uint32_t nanoSec)
{
    volatile uint32_t x, y = 0;
    for (x = 0; x < nanoSec; x += 1)
        ++y;
//    struct timespec delay;
//    delay.tv_sec = 0;
//    delay.tv_nsec = nanoSec;
//    nanosleep(&delay, NULL);  
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
    (void) name;
    (void) from;
    set_cycles_per_second(to);
    SIDClockFreqChanged();
}

static void prefs_speed_changed(const char *name, int32_t from, int32_t to)
{
    (void) name;
    (void) from;

    speed_adjust = to;
}

/*
 *  Start hardware SID connection
 */

void SIDInit()
{
	startGPIO();
    startTimers();
    pending_IRQs = 0;

    set_cycles_per_second(PrefsFindString("victype", 0));
    speed_adjust = PrefsFindInt32("speed");
    PrefsSetCallbackString("victype", prefs_victype_changed);
    PrefsSetCallbackInt32("speed", prefs_speed_changed);
}

void SIDReset(uint32_t now)
{
    (void) now;

    GPIO_WriteBit(SID_PORT, SID_RES, Bit_RESET);
    delay(10000);
    GPIO_WriteBit(SID_PORT, SID_RES, Bit_SET);
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
    TIM_SetAutoreload(TIM4, (uint32_t)cia_timer * 100 / speed_adjust);
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
    TIM_SetAutoreload(TIM4, (uint32_t)cia_timer * 100 / speed_adjust);
}

void cia_th_write(uint8_t byte)
{
    cia_timer = (cia_timer & 0x00ff) | (byte << 8);
    TIM_SetAutoreload(TIM4, (uint32_t)cia_timer * 100 / speed_adjust);
}

// Read from SID register
uint32_t sid_read(uint32_t adr, uint32_t now)
{
    (void) adr;
    (void) now;

    return 0;
}

// Write to SID register
void sid_write(uint32_t adr, uint32_t byte, uint32_t now, bool rmw)
{
    (void) now;
    (void) rmw;

    //printf("sid_write %02x to %04x at cycle %d\n", byte, adr, now);
    uint16_t val = byte | (adr << 8) | SID_RES;
    GPIO_Write(SID_PORT, val | SID_CS);
    GPIO_Write(SID_PORT, val);
    delay(SID_DELAY);
    GPIO_Write(SID_PORT, val | SID_CS);
}
