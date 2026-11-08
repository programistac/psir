#include "renderer.h"
#include "../gpu/shader.h"

#include "GL/glew.h"
#include <assert.h>

/* Definicje dla VRAM jako makra dla kompatybilnosci ze starymi naglowkami */
#define GL_GPU_MEM_TOTAL_NVX 0x9048
#define GL_GPU_MEM_FREE_NVX  0x9049
#define GL_VRAM_TOTAL_AMDX   0x87FB
#define GL_VRAM_FREE_AMDX    0x87FC

void renderer_log_info(const struct renderer *r)
{
	/* Wszystkie deklaracje zmiennych na poczatku bloku (wymog C89) */
	const GLubyte* vendor;
	const GLubyte* renderer_str;
	const GLubyte* version;
	const GLubyte* glsl_ver;
	const GLubyte* glew_ver;

	GLint major = 0;
	GLint minor = 0;
	GLint current_fbo = 0;
	GLenum status;

	/* Zmienne do sprawdzania liczby bitow we Framebufferze */
	GLint red_bits = 0;
	GLint green_bits = 0;
	GLint blue_bits = 0;
	GLint alpha_bits = 0;
	GLint depth_bits = 0;
	GLint stencil_bits = 0;
	GLint color_type = 0;

	GLint max_renderbuffer_size = 0;
	GLint max_color_attachments = 0;
	GLint max_samples = 0;
	GLint max_texture_size = 0;

	GLint total_mem_kb = 0;
	GLint free_mem_kb = 0;

	printf("==================================================\n");
	printf("          INFORMACJE O SYSTEMIE RENDEROWANIA       \n");
	printf("==================================================\n");

	/* 1. Identyfikacja sprzetu i sterownika */
	vendor       = glGetString(GL_VENDOR);
	renderer_str = glGetString(GL_RENDERER);
	version      = glGetString(GL_VERSION);
	glsl_ver     = glGetString(GL_SHADING_LANGUAGE_VERSION);
	glew_ver     = glewGetString(GLEW_VERSION);

	printf("[GPU] Producent:          %s\n", vendor       ? (const char*)vendor       : "Nieznany");
	printf("[GPU] Karta Graficzna:     %s\n", renderer_str ? (const char*)renderer_str : "Nieznana");
	printf("[GL]  Wersja OpenGL:      %s\n", version      ? (const char*)version      : "Nieznana");
	printf("[GL]  Wersja GLSL:        %s\n", glsl_ver     ? (const char*)glsl_ver     : "Nieznana");
	printf("[GL]  Wersja GLEW:        %s\n", glew_ver     ? (const char*)glew_ver     : "Nieznana");

	/* 2. Pobieranie kontekstu numerycznego */
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	if (major > 0) {
		printf("[GL]  Wersja (numerycznie): %d.%d\n", major, minor);
	}


	/* Rzutujemy ze wzgledu na const w sygnaturze funkcji logujacej */
	framebuffer_bind((struct framebuffer*)&r->fb);

	/* 3. Sprawdzenie aktywnego Framebuffera w kontekscie Twojego obiektu */
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &current_fbo);
	printf("[FBO] Rezolucja wewnetrzna: %dx%d\n", r->resolution_width, r->resolution_height);
	printf("[FBO] Aktywny Framebuffer ID: %d", current_fbo);

	if (current_fbo == 0) {
		printf(" (Domyslny ekranowy FBO)\n");
	} else {
		printf(" (Customowy FBO uzytkownika)\n");

		status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		printf("[FBO] Status kompletnosci: ");
		switch (status) {
			case GL_FRAMEBUFFER_COMPLETE:                      printf("KOMPLETNY (OK)\n"); break;
			case GL_FRAMEBUFFER_UNDEFINED:                     printf("BLAD: Niezdefiniowany\n"); break;
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:         printf("BLAD: Niekompletne zalaczniki\n"); break;
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: printf("BLAD: Brak zalacznikow\n"); break;
			case GL_FRAMEBUFFER_UNSUPPORTED:                   printf("BLAD: Format nieobslugiwany\n"); break;
			default:                                           printf("BLAD: Nieznany status (%d)\n", status); break;
		}
	}

	/* 3b. Szczegolowe informacje o bitach i strukturze aktualnego bufora */
	if (current_fbo == 0) {
		/* Dla domyslnego FBO pobieramy bity bezposrednio z globalnego systemu okien */
		glGetIntegerv(GL_RED_BITS, &red_bits);
		glGetIntegerv(GL_GREEN_BITS, &green_bits);
		glGetIntegerv(GL_BLUE_BITS, &blue_bits);
		glGetIntegerv(GL_ALPHA_BITS, &alpha_bits);
		glGetIntegerv(GL_DEPTH_BITS, &depth_bits);
		glGetIntegerv(GL_STENCIL_BITS, &stencil_bits);
	} else {
		/* Dla FBO uzytkownika odpytujemy parametry konkretnych punktow zakotwiczenia */
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE, &red_bits);
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE, &green_bits);
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE, &blue_bits);
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE, &alpha_bits);
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &depth_bits);
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, &stencil_bits);
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE, &color_type);
	}

	printf("[FBO] Format kolorow:     RGBA %d:%d:%d:%d\n", red_bits, green_bits, blue_bits, alpha_bits);
	printf("[FBO] Bufor glebi (Depth): %d bit\n", depth_bits);
	printf("[FBO] Bufor szablonu (St.): %d bit\n", stencil_bits);

	if (current_fbo != 0) {
		printf("[FBO] Typ komponentu:     ");
		switch (color_type) {
			case GL_INT:                printf("Znakowany Integer (int)\n"); break;
			case GL_UNSIGNED_INT:       printf("Bezznakowany Integer (uint)\n"); break;
			case GL_FLOAT:              printf("Zmiennoprzecinkowy (Float / HDR)\n"); break;
			case GL_UNSIGNED_NORMALIZED: printf("Znormalizowany (np. UNORM 8-bit)\n"); break;
			case GL_SIGNED_NORMALIZED:   printf("Znormalizowany ze znakiem\n"); break;
			default:                     printf("Nieznany (%d)\n", color_type); break;
		}
	}

	/* 4. Maksymalne mozliwosci sprzetowe */
	glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &max_renderbuffer_size);
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);
	glGetIntegerv(GL_MAX_SAMPLES, &max_samples);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);

	printf("[LIMIT] Maks. rozmiar Renderbuffera: %dx%d\n", max_renderbuffer_size, max_renderbuffer_size);
	printf("[LIMIT] Maks. rozmiar Tekstury:      %dx%d\n", max_texture_size, max_texture_size);
	printf("[LIMIT] Maks. punkty Color Attach.:  %d\n", max_color_attachments);
	printf("[LIMIT] Maks. probki MSAA:           %dx\n", max_samples);

	/* 5. Pamiec VRAM */
	glGetIntegerv(GL_GPU_MEM_TOTAL_NVX, &total_mem_kb);
	glGetIntegerv(GL_GPU_MEM_FREE_NVX, &free_mem_kb);

	if (total_mem_kb == 0) {
		glGetIntegerv(GL_VRAM_TOTAL_AMDX, &total_mem_kb);
		glGetIntegerv(GL_VRAM_FREE_AMDX, &free_mem_kb);
	}

	if (total_mem_kb > 0) {
		printf("[VRAM] Calkowita pamiec: %d MB\n", total_mem_kb / 1024);
		printf("[VRAM] Wolna pamiec:     %d MB\n", free_mem_kb / 1024);
	} else {
		printf("[VRAM] Informacje o pamieci niedostepne dla tego sterownika.\n");
	}

	printf("==================================================\n\n");
}


void renderer_initialize(struct renderer *r, int w, int h)
{
	assert(r != NULL);

	r->resolution_width = w;
	r->resolution_height = h;

	r->clear_color[0] = 0.0f;
	r->clear_color[1] = 0.0f;
	r->clear_color[2] = 0.0f;
	r->clear_color[3] = 1.0f;

	framebuffer_create(&r->fb, r->resolution_width, r->resolution_height);

	/* Ustawienie domyślnego koloru czyszczenia raz na początku */
	glClearColor(
		r->clear_color[0],
		r->clear_color[1],
		r->clear_color[2],
		r->clear_color[3]
	);

	/* Włączenie blendowania globalnie, skoro projekt z niego korzysta */
	/*glEnable(GL_BLEND);*/
}

void renderer_set_clear_color(struct renderer *r, float re, float g, float b, float a)
{
	r->clear_color[0] = re;
	r->clear_color[1] = g;
	r->clear_color[2] = b;
	r->clear_color[3] = a;

	/* Aktualizacja stanu OpenGL tylko przy zmianie wartości */
	glClearColor(re, g, b, a);
}

void renderer_clear(struct renderer *r, int f)
{
	GLbitfield mask = 0;
	(void)r; /* Uniknięcie ostrzeżenia o nieużywanym argumencie w C89 */

	if (f & RENDERER_CLEAR_COLOR)
		mask |= GL_COLOR_BUFFER_BIT;

	if (f & RENDERER_CLEAR_DEPTH)
		mask |= GL_DEPTH_BUFFER_BIT;

	if (f & RENDERER_CLEAR_STENCIL)
		mask |= GL_STENCIL_BUFFER_BIT;

	/* Usunięto stąd glClearColor dla lepszej wydajności */
	glClear(mask);
}

void renderer_begin(struct renderer *r)
{
	framebuffer_bind(&r->fb);
	glViewport(0, 0, r->resolution_width, r->resolution_height);
}

void renderer_end(struct renderer *r)
{
	(void)r;
	framebuffer_unbind();
}

void renderer_draw_frame(struct gpu_data *data)
{
	glBindVertexArray(data->vaos[VAO_DATA]);
	glDrawArrays(GL_TRIANGLES, 0, data->gpu_vertex_count);
}

void renderer_present(struct renderer *r, int w, int h)
{
	/* Wszystkie deklaracje zmiennych lądują na samej górze bloku dla C89 */
	float targetAspect;
	float screenAspect;
	int dstW, dstH, dstX, dstY;

	targetAspect = 4.0f / 3.0f;
	screenAspect = (float)w / (float)h;

	if (screenAspect > targetAspect) {
		dstH = h;
		dstW = (int)(dstH * targetAspect);
		dstX = (w - dstW) / 2;
		dstY = 0;
	} else {
		dstW = w;
		dstH = (int)(dstW / targetAspect);
		dstX = 0;
		dstY = (h - dstH) / 2;
	}

	framebuffer_blit_to_screen(&r->fb, dstX, dstY, dstW, dstH);
}

void renderer_set_full_transparent_mode(void)
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void renderer_set_semi_transparent_mode(void)
{
	glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
	glBlendColor(0.0f, 0.0f, 0.0f, 0.5f);
}

