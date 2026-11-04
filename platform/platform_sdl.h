#ifndef PLATFORM_SDL_H
#define PLATFORM_SDL_H

#include <SDL2/SDL.h>
#include <stdint.h>

struct platform_sdl
{
	/* State */
	uint8_t initialized;
	uint8_t running;

	/* SDL */
	SDL_Window* window;
	SDL_GLContext gl_context;

	/* Window */
	uint16_t window_w;
	uint16_t window_h;
	uint8_t vsync;

	/* Frame timing */
	double target_fps;
	double target_frame_time;

	Uint64 perf_freq;
	Uint64 frame_start;
};

int  platform_initialize(
		struct platform_sdl* p,
		uint16_t width,
		uint16_t height,
		const char* title,
		uint8_t vsync,
		double target_fps
		);

void platform_shutdown(struct platform_sdl* p);

void platform_begin_frame(struct platform_sdl* p);
void platform_end_frame(struct platform_sdl* p);

void platform_poll_events(struct platform_sdl* p);

#endif
