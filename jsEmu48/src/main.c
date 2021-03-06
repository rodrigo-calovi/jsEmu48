/*
 *     /
 *    /__  ___  ___  ____
 *   /  / /  / /__/ / / / /  /
 *  /  / /__/ /__  /   / /__/
 *      /
 *     /    version 0.9.0
 *
 * Copyright 2002 Daniel Nilsson
 *
 * This file is part of hpemu.
 *
 * Hpemu is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hpemu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with hpemu; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <stdio.h>
#include "types.h"
#include "emulator.h"
#include "gui.h"
#include "color.h"

#include "display.h"
#include "gui.h"
#include "timers.h"
#include "keyboard.h"
#include "pcalc.h"
#include "pfiles.h"

#include <SDL.h>
#include <SDL_image.h>
#ifdef SDL_TTF
#include <SDL_ttf.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

const int SCREEN_WIDTH = 540;
const int SCREEN_HEIGHT = 1100;


SDL_Window* window = NULL;
SDL_Renderer * renderer = NULL;
SDL_Texture *texTarget = NULL;
SDL_Texture *faceplateTexture = NULL;

#ifdef SDL_TTF
TTF_Font * ArialFonte = NULL;
#endif

SDL_TimerID my_timer0_id;
SDL_TimerID my_timer1_id;
SDL_TimerID my_timer2_id;
SDL_TimerID my_timer3_id;
SDL_TimerID my_timer4_id;

boolean SDL_ready = FALSE;



unsigned int framecount = 0;
unsigned int emuframecount = 0;

unsigned int currentTime;
unsigned int currentTime_emu;
unsigned int lastTime_timer_fps = 0;
unsigned int lastTime_timer_emu = 0;


unsigned int lastTime_timer1 = 0;
unsigned int delay_timer1 = 10; // 1000 / X = 20

unsigned int lastTime_timer2 = 0;
unsigned int delay_timer2 = 1000;

unsigned int lastTime_timer3 = 0;
unsigned int delay_timer3 = 62; // 1000 / X = 16

unsigned int lastTime_timer4 = 0;
unsigned int delay_timer4 = 200;// 50; // 1000 / X = 8192

unsigned int lastTime_timer5 = 0;
unsigned int delay_timer5 = 100; // 100; //50;



Uint32 my_callbackfunc0(Uint32 interval, void *param)
{
	SDL_Event event;
	SDL_UserEvent userevent;
	
	userevent.type = SDL_USEREVENT;
	userevent.code = 1;
	userevent.data1 = &gui_update;
	userevent.data2 = NULL;//param;
	
	event.type = SDL_USEREVENT;
	event.user = userevent;
	
	SDL_PushEvent(&event);
	return(interval);
}


Uint32 my_callbackfunc1(Uint32 interval, void *param)
{
	SDL_Event event;
	SDL_UserEvent userevent;
	
	userevent.type = SDL_USEREVENT;
	userevent.code = 1;
	userevent.data1 = NULL;//&display_update;
	userevent.data2 = NULL;//param;
	
	event.type = SDL_USEREVENT;
	event.user = userevent;
	
	SDL_PushEvent(&event);
	return(interval);
}

Uint32 my_callbackfunc2(Uint32 interval, void *param)
{
	SDL_Event event;
	SDL_UserEvent userevent;
	
	userevent.type = SDL_USEREVENT;
	userevent.code = 2;
	userevent.data1 = &true_speed_proc;
	userevent.data2 = NULL;//param;
	
	event.type = SDL_USEREVENT;
	event.user = userevent;
	
	SDL_PushEvent(&event);
	return(interval);
}

Uint32 my_callbackfunc3(Uint32 interval, void *param)
{
	SDL_Event event;
	SDL_UserEvent userevent;
	
	userevent.type = SDL_USEREVENT;
	userevent.code = 3;
	userevent.data1 = &timer1_update;
	userevent.data2 = NULL;//param;
	
	event.type = SDL_USEREVENT;
	event.user = userevent;
	
	SDL_PushEvent(&event);
	return(interval);
}

Uint32 my_callbackfunc4(Uint32 interval, void *param)
{
	SDL_Event event;
	SDL_UserEvent userevent;
	
	userevent.type = SDL_USEREVENT;
	userevent.code = 4;
	userevent.data1 = &display_show; //  timer2_update;
	userevent.data2 = NULL;//param;
	
	event.type = SDL_USEREVENT;
	event.user = userevent;
	
	SDL_PushEvent(&event);
	return(interval);
}



static int fullscreen = FALSE;

static void parse_args(int argc, char *argv[])
{
    while (--argc) {
	argv++;
	if (argv[0][0] == '-') {
	    switch (argv[0][1]) {
	    case 'f':
		fullscreen = TRUE;
		break;
	    case 'w':
		fullscreen = FALSE;
		break;
	    }
	}
    }
}


static void program_init(void)
{
	if( SDL_Init( SDL_INIT_VIDEO | IMG_INIT_PNG | SDL_INIT_TIMER ) < 0 )
	{
		printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
		return;
	}
	
#ifdef SDL_TTF
	if(TTF_Init() == -1)
	{
		fprintf(stderr, "Erreur d'initialisation de TTF_Init : %s\n", TTF_GetError());
		exit(EXIT_FAILURE);
	}
	
	ArialFonte = TTF_OpenFont("FreeSans.ttf", 14);
#endif
	
	
	window = SDL_CreateWindow( "jsEmu48", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
	if( window == NULL )
	{
		printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
		return;
	}
	
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if(renderer == NULL)
	{
		printf("Erreur lors de la creation d'un renderer : %s",SDL_GetError());
		return;
	}
	
	texTarget = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 131, 64);
	
	
	SDL_Surface * faceplate = IMG_Load("48face4.png");
	if(faceplate) {
		//printf("init text2 %s\n", buttons->text);
		
		faceplateTexture = SDL_CreateTextureFromSurface( renderer, faceplate );
	}
	
	
	SDL_SetRenderTarget(renderer, texTarget);
	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	SDL_RenderClear(renderer);
	SDL_SetRenderTarget(renderer, NULL);
	
	SDL_UpdateWindowSurface( window );
	
	
	pcalc_init();
	
	printf("init done\n");
	
    color_init();
	
	SDL_ready = TRUE;
}

void start_timers()
{
	printf("start_timers\n");
	//my_timer0_id = SDL_AddTimer(100, my_callbackfunc0, NULL); // gui_update
//	my_timer1_id = SDL_AddTimer(50, my_callbackfunc1, NULL); // display
//	my_timer2_id = SDL_AddTimer(1000, my_callbackfunc2, NULL); // cpu real speed
//	my_timer3_id = SDL_AddTimer(62, my_callbackfunc3, NULL); // timer1
//	my_timer4_id = SDL_AddTimer(500, my_callbackfunc4, NULL); // timer2
}

static void program_exit(void)
{
	/*
	//SDL_RemoveTimer(my_timer0_id);
	SDL_RemoveTimer(my_timer1_id);
	SDL_RemoveTimer(my_timer2_id);
	SDL_RemoveTimer(my_timer3_id);
	SDL_RemoveTimer(my_timer4_id);
	*/
#ifdef SDL_TTF
	TTF_CloseFont(ArialFonte);
	TTF_Quit();
#endif
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow( window );
	SDL_Quit();
}

boolean refreshSDL()
{
	SDL_Event event;
	//SDL_WaitEvent(&event);
	while(SDL_PollEvent(&event))
	//if(SDL_PollEvent(&event))
	{
		switch(event.type)
		{
			case SDL_MOUSEBUTTONUP:
			{
				//printf("mouse up %d %d\n", event.button.x, event.button.y);
				
				pcalc_up(event.button.x, event.button.y, 1);
			}
			break;
				
			case SDL_MOUSEBUTTONDOWN:
			{
				//printf("mouse down %d %d\n", event.button.x, event.button.y);
				
				pcalc_down(event.button.x, event.button.y, 1);
			}
			break;
				
			case SDL_USEREVENT:
			{
				printf("SDL_USEREVENT\n");
				//if(event.user.code == 1)
				
				//void (*p) (void*) = event.user.data1;
				//p(event.user.data2);
			}
			break;
				
			case SDL_QUIT:
			{
				please_exit = TRUE;
				//emulator_state = EMULATOR_STOP;
				return FALSE;
			}
		}
	}
	return TRUE;
}

void mainloop()
{
	if(please_exit == TRUE)
	{
		printf("please exit\n");
		return;
	}
	if(SDL_ready == TRUE)
	{

		currentTime = SDL_GetTicks();
		
#ifdef EMSCRIPTEN
		
		currentTime_emu = currentTime;
		emuframecount = 0;
		
		do {
			emuframecount ++;
			emulator_run();
			
			currentTime_emu = SDL_GetTicks() - currentTime;
		}
		while (currentTime_emu < 2);
		
		//printf("EMU emuframecount = %d | time = %d\n", emuframecount, currentTime_emu);
		
#else
		
		emulator_run();
		
#endif
		
		/*
		framecount++;
		
		if (currentTime >= lastTime_timer_fps + 1000) {
			//printf("Report(2) %dmsec: %d\n", delay_timer2, currentTime - lastTime_timer2);
			lastTime_timer_fps = currentTime;
			printf("FPS = %d\n", framecount);
			framecount = 0;
		}
		*/
		
		//printf("mainloop() currentTime = %d\n", currentTime);
		
#if 1
		// true_speed_proc
		if (currentTime > lastTime_timer2 + delay_timer2) {
			//printf("Report(2) %dmsec: %d\n", delay_timer2, currentTime - lastTime_timer2);
			lastTime_timer2 = currentTime;
			true_speed_proc();
		}
	
		// display_update
		if (currentTime > lastTime_timer1 + delay_timer1) {
			//printf("Report(1) %dmsec: %d\n", delay_timer1, currentTime - lastTime_timer1);
			lastTime_timer1 = currentTime;
			display_update();
		}
		
		// timer1
		if (currentTime > lastTime_timer3 + delay_timer3) {
			//printf("Report(3) %dmsec: %d\n", delay_timer3, currentTime - lastTime_timer3);
			lastTime_timer3 = currentTime;
			timer1_update();
		}
/*
		// timer2
		if (currentTime > lastTime_timer4 + delay_timer4) {
			//printf("Report(4) %dmsec: %d\n", delay_timer4, currentTime - lastTime_timer4);
			lastTime_timer4 = currentTime;
			timer2_update();
		}
*/
		// display show
		if (currentTime > lastTime_timer5 + delay_timer5) {
			lastTime_timer5 = currentTime;
			display_show();
		}
#endif
		
	
		if(refreshSDL() == FALSE)
		{
	#ifdef EMSCRIPTEN
			printf("emscripten_cancel_main_loop\n");
			emscripten_cancel_main_loop();
	#endif
			return;
		}
	}
}

int main (int argc, char *argv[])
{
    parse_args (argc, argv);
	
	program_init();
	emulator_init();
	//gui_init();
	
	//start_timers();
	
#ifdef EMSCRIPTEN
	printf("emscripten_set_main_loop\n");
	emscripten_set_main_loop(mainloop, 0, 1);
	
	//emscripten_set_main_loop_timing(EM_TIMING_SETTIMEOUT, 0);
	
	//while(please_exit == FALSE) mainloop();
#else
	printf("NO emscripten_set_main_loop\n");
	while(please_exit == FALSE) mainloop();
#endif
	
	/*
#ifdef EMSCRIPTEN
	printf("emscripten_set_main_loop\n");
	emscripten_set_main_loop(mainloop, 1000, 1);
#else
	printf("NO emscripten_set_main_loop\n");
	while(please_exit == FALSE) mainloop();
#endif
*/
    gui_exit();
    emulator_exit();
    program_exit();

    return 0;
}

