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

	/* Stan klawiatury dla obecnej i poprzedniej klatki */
	unsigned char keys_current[SDL_NUM_SCANCODES];
	unsigned char keys_previous[SDL_NUM_SCANCODES];
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

/* Funkcje do odczytu stanu z dowolnego miejsca, gdzie masz wskaznik p */
int platform_key_down(const struct platform_sdl *p, SDL_Scancode key);
int platform_key_pressed(const struct platform_sdl *p, SDL_Scancode key);
int platform_key_released(const struct platform_sdl *p, SDL_Scancode key);

#endif
