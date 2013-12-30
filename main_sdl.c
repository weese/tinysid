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

#include "sys.h"

#ifdef USE_SDL
#include <SDL.h>
#include <SDL_endian.h>
#else
//#include <pthread.h>
#include <plib.h>
#include "HardwareProfile.h"
#include "delay.h"
uint32_t cia_period_usec();
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

//#include <unistd.h>
#include <sys/time.h>
#include <signal.h>

#include "main.h"
#include "prefs.h"
#include "cpu.h"
#include "sid.h"


/*
 *  Get current value of microsecond timer
 */

uint64_t GetTicks_usec()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec * 1000000 + t.tv_usec;
}


/*
 *  Delay by specified number of microseconds (<1 second)
 *  (adapted from SDL_Delay() source)
 */

void Delay_usec(uint32_t usec)
{
    int was_error;
#ifndef __linux__    /* Non-Linux implementations need to calculate time left */
    uint64_t then, now, elapsed;
#endif
    struct timeval tv;

    /* Set the timeout interval - Linux only needs to do this once */
#ifdef __linux__
    tv.tv_sec = 0;
    tv.tv_usec = usec;
#else
    then = GetTicks_usec();
#endif
    do {
        errno = 0;
#ifndef __linux__
        /* Calculate the time interval left (in case of interrupt) */
        now = GetTicks_usec();
        elapsed = (now-then);
        then = now;
        if ( elapsed >= usec ) {
            break;
        }
        usec -= elapsed;
        tv.tv_sec = 0;
        tv.tv_usec = usec;
#endif
        was_error = select(0, NULL, NULL, NULL, &tv);
    } while (was_error && (errno == EINTR));
}


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
    keepRunning = false;
    printf("Exiting...\n");
} 

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
//    struct timespec delay;
    while (keepRunning)
    {
        // Delay to maintain proper replay frequency
//        int64_t nextIRQ = GetTicks_usec() + cia_period_usec();
        usTimer = GetTicks_usec();

        // execute IRQ handler (that sends commands to the HW SID directly)
        UpdatePlayAdr();
        CPUExecute(play_adr, 0, 0, 0, 1000000);

//        int64_t delay_usec = nextIRQ - GetTicks_usec();
//	if (delay_usec > 0)
//        {
////            Delay_usec(delay_usec);
//	    delay.tv_sec = delay_usec / 1000000;
//            delay.tv_nsec = (delay_usec % 1000000) * 1000;
//            nanosleep(&delay, NULL);
////	    printf("%llu\n",delay_usec);
//	}
//	else
    if (usTimer != 0)
    {
        while (usTimer);
    }
    else
	{
		printf("too slow.\n");
	}
    }
    SIDReset(0);
#endif
    ExitAll();
    return 0;
}
