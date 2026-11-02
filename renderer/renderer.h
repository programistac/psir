#ifndef RENDERER_H
#define RENDERER_H

#include "../gpu/framebuffer.h" /* framebuffer struct */
#include "../gpu/gpu.h" /* gpu geometry data */

enum renderer_clear_flags {
	RENDERER_CLEAR_COLOR = 1 << 0,
	RENDERER_CLEAR_DEPTH = 1 << 1,
	RENDERER_CLEAR_STENCIL = 1 << 2

};

struct renderer {
	struct framebuffer fb;
	int resolution_width;
	int resolution_height;
	float clear_color[4]; /* background rgba */
};

void renderer_initialize(struct renderer *r, int w, int h);
void renderer_begin(struct renderer *r);
void renderer_end(struct renderer *r);

void renderer_draw_frame(struct gpu_data *data);

void renderer_draw_static_geometry(struct gpu_data *geometry, int num_vertices);
void renderer_present(struct renderer *r, int w, int h);
void renderer_destroy(struct renderer *r);

void renderer_set_clear_color(struct renderer *r, float re, float g, float b, float a);
void renderer_clear(struct renderer *r, int f);

void renderer_set_full_transparent_mode();
void renderer_set_semi_transparent_mode();


#endif
