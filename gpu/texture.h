#ifndef TEXTURE_H
#define TEXTURE_H

#include <stdint.h> /* for uint16_t */

#define MAX_PALETTES 4

/* CLUT WORK VERSION */
struct texture_clut_palette_atlas {
	unsigned int id;
	uint16_t palette[MAX_PALETTES][256];

	int count;
};

struct texture_clut_indexed_image {
	unsigned int id;
	uint8_t *indices;

	int width;
	int height;

	int palette_id;
};

/* load png image extract palette and add it to atlas */
void texture_load_clut_png_extract_palette(
		struct texture_clut_palette_atlas *t, 
		const char *filename);

/* load png image extract indices and upload to opengl */
void texture_load_clut_png_extract_indices(
		struct texture_clut_indexed_image *t, 
		const char *filename);

void texture_clut_palette_atlas_upload(
		struct texture_clut_palette_atlas *atlas);
void texture_clut_indexed_image_upload(
		struct texture_clut_indexed_image *image);


void texture_clut_palette_atlas_bind(
		const struct texture_clut_palette_atlas *atlas);

void texture_clut_indexed_image_bind(
		const struct texture_clut_indexed_image *image);

/* Resource cleanup and destruction */
void texture_clut_palette_atlas_destroy(
		struct texture_clut_palette_atlas *atlas);
void texture_clut_indexed_image_destroy(
		struct texture_clut_indexed_image *image);

#endif
