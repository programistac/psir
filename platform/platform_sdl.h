#ifndef PLATFORM_SDL_H
#define PLATFORM_SDL_H

#include <SDL2/SDL.h>

struct platform_sdl
{
	SDL_Window* window;
	SDL_GLContext gl_context;

	int window_w;
	int window_h;

	int running;
	int vsync;

	double target_fps;
	double target_frame_time;

	Uint64 perf_freq;
	Uint64 frame_start;
};

int  platform_init(
		struct platform_sdl* p,
		int width,
		int height,
		const char* title,
		int vsync,
		double target_fps
		);

void platform_shutdown(struct platform_sdl* p);

void platform_begin_frame(struct platform_sdl* p);
void platform_end_frame(struct platform_sdl* p);

void platform_poll_events(struct platform_sdl* p);

#endif
