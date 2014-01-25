/*
 *  main_sdl.cpp - SIDPlayer SDL main program
 *
 *  SIDPlayer (C) Copyright 1996-2004 Christian Bauer
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <pthread.h>
#include "stm32f4xx_it.h"
#include <stm32f4xx_usart.h>
#include <stm32f4_discovery.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/time.h>
#include <signal.h>

#include "main.h"
#include "prefs.h"
#include "cpu.h"
#include "sid.h"

/*
 *  Main program
 */

static void usage(const char *prg_name)
{
    printf("Usage: %s [OPTION...] FILE [song_number]\n", prg_name);
    PrefsPrintUsage();
    exit(0);
}

static void quit()
{
    ExitAll();
#ifdef USE_SDL
    SDL_Quit();
#endif
}

static bool keepRunning = true;

void intHandler(int dummy) 
{
    (void) dummy;
    keepRunning = false;
    printf("Exiting...\n");
} 

void usart2_init()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	/* enable peripheral clock for USART2 */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);


	/* GPIOA clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	/* GPIOA Configuration:  USART2 TX on PA2 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Connect USART2 pins to AF2 */
	// TX = PA2
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);

	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);

	USART_Cmd(USART2, ENABLE); // enable USART2


//    USART_InitTypeDef USART_InitStructure;
//    USART_InitStructure.USART_BaudRate = 115200;
//    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
//    USART_InitStructure.USART_StopBits = USART_StopBits_1;
//    USART_InitStructure.USART_Parity = USART_Parity_No;
//    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
//    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
//
//    GPIO_InitTypeDef GPIO_InitStructure;
//    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
//    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
//
//    /* Configure USART Tx as push-pull */
//    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
//    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
//    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_Init(GPIOA, &GPIO_InitStructure);
//
//    /* Configure USART Rx as input floating */
//    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
//    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
//    GPIO_Init(GPIOA, &GPIO_InitStructure);
//
//    /* USART configuration */
//    USART_Init(USART2, &USART_InitStructure);
//
//    /* Enable USART */
//    USART_Cmd(USART2, ENABLE);
}

//int main(int argc, char **argv)
int main(void)
{
    int argc = 2;
    char *argv[] = { "./tinysid", "calabash.sid" };
    STM_EVAL_LEDInit(LED3);
    STM_EVAL_LEDInit(LED4);
    STM_EVAL_LEDInit(LED5);
    STM_EVAL_LEDInit(LED6);

    STM_EVAL_LEDOn(LED3);

    //by default stdin/stdout are on usart2
    usart2_init();
    // turn off buffers, so IO occurs immediately
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    // Print banner
    printf(
        "SIDPlayer Version 4.4\n\n"
        "Copyright (C) 1996-2004 Christian Bauer\n"
        "E-mail: Christian.Bauer@uni-mainz.de\n"
        "http://www.uni-mainz.de/~bauec002/\n\n"
        "This is free software with ABSOLUTELY NO WARRANTY.\n"
        "You are welcome to redistribute it under certain conditions.\n"
        "For details, see the file COPYING.\n\n"
    );

#ifdef USE_SDL
    // Initialize everything
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "Couldn't initialize SDL (%s)\n", SDL_GetError());
        exit(1);
    }
#endif

    atexit(quit);
    InitAll(argc, argv);
    signal(SIGINT, intHandler);

    int32_t speed = PrefsFindInt32("speed");

    // Parse non-option arguments
    const char *file_name = NULL;
    int song = 0;
        int i;
    for (i=1; i<argc; i++) {
		if (argv[i] == NULL)
			break;
        if (strcmp(argv[i], "--help") == 0)
            usage(argv[0]);
        else if (argv[i][0] == '-') {
            fprintf(stderr, "Unrecognized option '%s'\n", argv[i]);
            usage(argv[0]);
        } else {
            if (file_name == NULL)
                file_name = argv[i];  // First non-option argument is file name
            else
                song = atoi(argv[i]); // Second non-option argument is song number
        }
    }
    if (file_name == NULL)
        usage(argv[0]);

    // Load given PSID file
    if (!LoadPSIDFile(file_name)) {
        fprintf(stderr, "Couldn't load '%s' (not a PSID file?)\n", file_name);
        exit(1);
    }

    // Select song
    if (song > 0) {
        if (song > number_of_songs)
            song = number_of_songs;
        SelectSong(song - 1);
    }

    SIDAdjustSpeed(speed); // SelectSong and LoadPSIDFile() reset this to 100%

    // Print file information
    printf("Module Name: %s\n", module_name);
    printf("Author     : %s\n", author_name);
    printf("Copyright  : %s\n\n", copyright_info);
    printf("Playing song %d/%d\n", current_song + 1, number_of_songs);

#ifdef USE_SDL
    // Start replay and enter main loop
    SDL_PauseAudio(false);
    while (true) {
        SDL_Event e;
        if (SDL_WaitEvent(&e)) {
            if (e.type == SDL_QUIT)
                break;
        }
    }
#else
    STM_EVAL_LEDOn(LED3);
    while (keepRunning)
    {
        // execute IRQ handler (that sends commands to the HW SID directly)
        STM_EVAL_LEDOn(LED4);
        UpdatePlayAdr();
        CPUExecute(play_adr, 0, 0, 0, 1000000);
        STM_EVAL_LEDOff(LED4);

        while ((pending_IRQs & IRQ_CIA_A) == 0);
        pending_IRQs &= ~IRQ_CIA_A;
    }
    SIDReset(0);
#endif
    ExitAll();
    return 0;
}
