#include "platform_sdl.h"

#include <GL/glew.h>

int platform_init(
		struct platform_sdl* p,
		int width,
		int height,
		const char* title,
		int vsync,
		double target_fps
		)
{
	SDL_Init(SDL_INIT_VIDEO);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);

	SDL_GL_SetAttribute(
			SDL_GL_CONTEXT_PROFILE_MASK,
			SDL_GL_CONTEXT_PROFILE_CORE
			);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 1);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	p->window_w = width;
	p->window_h = height;
	p->vsync = vsync;

	p->target_fps = target_fps;
	p->target_frame_time = 1.0 / target_fps;

	p->running = 1;

	p->window = SDL_CreateWindow(
			title,
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			width,
			height,
			SDL_WINDOW_OPENGL |
			SDL_WINDOW_FULLSCREEN_DESKTOP
			);

	p->gl_context = SDL_GL_CreateContext(p->window);

	SDL_GL_SetSwapInterval(vsync);

	glewExperimental = GL_TRUE;
	glewInit();

	p->perf_freq = SDL_GetPerformanceFrequency();

	return 1;
}

void platform_shutdown(struct platform_sdl* p)
{
	SDL_GL_DeleteContext(p->gl_context);
	SDL_DestroyWindow(p->window);

	SDL_Quit();
}

/* Wysokorozdzielczy licznik czasu służy do precyzyjnego pomiaru czasu 
   trwania klatki.
   Częstotliwość licznika pozwala przeliczyć różnicę odczytów na sekundy. 

   - pobieram stan licznika na poczatku i na koncu klatki
   - przeliczam roznice i zamieniam to na sekundy
   - sprawdzam czy czas jest mniejszy niz czas na klatke i robie odpowiedni delay

*/

void platform_begin_frame(struct platform_sdl* p)
{
	p->frame_start = SDL_GetPerformanceCounter();
}

void platform_end_frame(struct platform_sdl* p)
{
	SDL_GL_SwapWindow(p->window);

	Uint64 frame_end =
		SDL_GetPerformanceCounter();

	double elapsed =
		(double)(frame_end - p->frame_start)
		/ (double)p->perf_freq;

	double remaining =
		p->target_frame_time - elapsed;

	if (remaining > 0.0) {
		SDL_Delay((Uint32)(remaining * 1000.0));
	}
}

void platform_poll_events(struct platform_sdl* p)
{
	SDL_Event e;

	while (SDL_PollEvent(&e)) {

		if (e.type == SDL_QUIT)
			p->running = 0;

		if (e.type == SDL_KEYDOWN) {
			if (e.key.keysym.sym == SDLK_ESCAPE)
				p->running = 0;
		}
	}
}
