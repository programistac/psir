#ifndef POSTPROCESS_H
#define POSTPROCESS_H

#include "framebuffer.h"

struct postprocess {
	unsigned int program;
	unsigned int vao;
};

int postprocess_initialize(struct postprocess *pp);

void postprocess_draw(
	struct postprocess *pp,
	const struct framebuffer *fb,
	int x,
	int y,
	int width,
	int height
);

void postprocess_destroy(struct postprocess *pp);

#endif
