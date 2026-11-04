#include "renderer.h"
#include "../gpu/shader.h"

#include "GL/glew.h"
#include <assert.h>

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
	glEnable(GL_BLEND);
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
	glDrawArrays(GL_TRIANGLES, 0, data->gpu_primitive_count);
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

