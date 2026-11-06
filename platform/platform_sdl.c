#include "platform_sdl.h"
#include <assert.h>

#include <GL/glew.h>

/* Modul platformowy SDL2/OpenGL.
 * Odpowiada za:
 * - inicjalizacje i zamykanie SDL oraz OpenGL,
 * - tworzenie okna,
 * - obsluge glownej petli zdarzen, 
 * - synchronizacje klatek,
 * - podstawowa obsluge wejscia uzytkownika.
 *
 * funkcje zakladaja przekazanie poprawnego wskaznika
 * do struktury platform_sdl.
 * poprawnosc argumentow jest weryfikowana za pomoca assert().
 */

int platform_initialize(
		struct platform_sdl* p,
		uint16_t width,
		uint16_t height,
		const char* title,
		uint8_t vsync,
		double target_fps
		)
{
	GLenum err;
	assert(p != NULL);
	p->initialized = 0;

	if (target_fps <= 0.0) {
		fprintf(stderr, "Invalid target_fps: %f\n", target_fps);
		return 0;
	}

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
		return 0;
	}

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

	if (p->window == NULL) {
		fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
		return 0;
	}

	p->gl_context = SDL_GL_CreateContext(p->window);
	if (p->gl_context == NULL) {
		fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
		SDL_DestroyWindow(p->window);
		p->window = NULL;
		SDL_Quit();
		return 0;
	}

	if (SDL_GL_SetSwapInterval(vsync) != 0) {
		fprintf(stderr, "SDL_GL_SetSwapInterval failed: %s\n", 
				SDL_GetError());
	}

	glewExperimental = GL_TRUE;
	err = glewInit();

	if (err != GLEW_OK) {
		fprintf(stderr, "glewInit failed: %s\n", glewGetErrorString(err));
		SDL_GL_DeleteContext(p->gl_context);
		SDL_DestroyWindow(p->window);
		p->gl_context = NULL;
		p->window = NULL;
		SDL_Quit();
		return 0;
	}

	p->perf_freq = SDL_GetPerformanceFrequency();
	if (p->perf_freq == 0) {
		fprintf(stderr, "SDL_GetPerformanceFrequency failed\n");
		SDL_GL_DeleteContext(p->gl_context);
		SDL_DestroyWindow(p->window);
		p->gl_context = NULL;
		p->window = NULL;
		SDL_Quit();
		return 0;
	}
	
	p->initialized = 1;
	return 1;
}

void platform_shutdown(struct platform_sdl* p)
{
	assert(p != NULL);
	
	/* Jeśli inicjalizacja w ogóle się nie udała, nie robimy nic */
	if (!p->initialized)
		return;

	if (p->gl_context != NULL) {
		SDL_GL_DeleteContext(p->gl_context);
		p->gl_context = NULL;
	}

	if (p->window != NULL) {
		SDL_DestroyWindow(p->window);
		p->window = NULL;
	}

	/* Zamknięcie głównego podsystemu SDL */
	SDL_Quit();

	/* IMG_Quit(); */
	/* TTF_Quit(); */

	p->initialized = 0;
	p->running = 0;
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
	assert(p != NULL);
	assert(p->initialized);
	p->frame_start = SDL_GetPerformanceCounter();
}

void platform_end_frame(struct platform_sdl* p)
{
	Uint64 frame_end;
	double elapsed, remaining;

	assert(p != NULL);
	assert(p->initialized);

	SDL_GL_SwapWindow(p->window);

	frame_end = SDL_GetPerformanceCounter();

	elapsed = (double)(frame_end - p->frame_start)
			/ (double)p->perf_freq;

	remaining = p->target_frame_time - elapsed;

	if (remaining > 0.0) {
		SDL_Delay((Uint32)(remaining * 1000.0));
	}
}

void platform_poll_events(struct platform_sdl* p)
{
	SDL_Event e;
	assert(p != NULL);

	while (SDL_PollEvent(&e)) {
		switch (e.type) {
			case SDL_QUIT: {
				p->running = 0;
			} break;

			case SDL_WINDOWEVENT: {
				/* Obsługa zmiany rozmiaru okna 
				 * przez system operacyjny */
				if (e.window.event == 
						SDL_WINDOWEVENT_RESIZED || 
				    e.window.event == 
						SDL_WINDOWEVENT_SIZE_CHANGED) {
					p->window_w = e.window.data1;
					p->window_h = e.window.data2;
					/* Aktualizacja obszaru renderowania */
					glViewport(
							0, 
							0, 
							p->window_w, 
							p->window_h
					);
				}
			} break;

			case SDL_KEYDOWN: {
				/* Ignoruj powtórzenia klawiszy, jeśli 
				 * system operacyjny 
				 * wysyła je przy przytrzymaniu */
				if (e.key.repeat)
					break;

				if (e.key.keysym.sym == SDLK_ESCAPE) {
					p->running = 0;
				}
				
			} break;

			case SDL_KEYUP: {
				/* Tutaj obsługuje puszczenie klawisza */
			} break;
			
			default:
				break;
		}
	}
}

