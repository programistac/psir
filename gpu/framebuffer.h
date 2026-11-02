#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

struct framebuffer {
	unsigned int fbo;
	unsigned int color;
	unsigned int depth;

	int width;
	int height;
};

void framebuffer_create(
	struct framebuffer *fb,
	int width,
	int height
);

void framebuffer_bind(struct framebuffer *fb);
void framebuffer_unbind(void);
void framebuffer_destroy(struct framebuffer *fb);
void framebuffer_blit_to_screen(struct framebuffer *fb, int dst_x, int dst_y, int dst_w, int dst_h);

#endif
