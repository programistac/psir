#include "texture.h" 

#include "GL/glew.h"
#include <GL/gl.h>
#include <SDL2/SDL_image.h> 

#include <png.h> 

static uint16_t rgb_to_psx(uint8_t r, uint8_t g, uint8_t b)
{
	uint16_t r5 = r >> 3;
	uint16_t g5 = g >> 3;
	uint16_t b5 = b >> 3; 

	/*

	 * PSX layout:
	 * 00000BBBBBGGGGGRRRRR
	 * 
	 * But OpenGL RGB5_A1 expects:
	 * RRRRRGGGGGBBBBBA
	 */
	return (r5 << 11) | (g5 << 6) | (b5 << 1) | 1;

} 

/* CLUT WORK VERSION */
void texture_load_clut_png_extract_palette(struct texture_clut_palette_atlas *t,
		const char *filename)
{
	png_structp png = NULL;
	png_infop info = NULL;
	png_bytep trans_alpha = NULL;
	png_color_16p trans_color = NULL;
	png_colorp png_palette = NULL;
	int palette_size = 0;
	int num_trans = 0;
	FILE *fp = NULL;
	int i; 

	/* Make sure there is room for another palette. */
	if (t->count >= MAX_PALETTES) {
		fprintf(stderr, "Palette atlas is full.\n");
		exit(EXIT_FAILURE);
	}

	/* Open the PNG file. */
	fp = fopen(filename, "rb");
	if (!fp) {
		perror(filename);
		exit(EXIT_FAILURE);
	}

	/*

	 * Create the main libpng read structure.
	 * This object stores all internal libpng state.
	 */
	png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png) {
		fclose(fp);
		fprintf(stderr, "Failed to create PNG read structure.\n");
		exit(EXIT_FAILURE);
	}

	/*

	 * Create the PNG info structure.
	 * It stores image metadata such as width, height,
	 * color type, palette, etc.
	 */
	info = png_create_info_struct(png);
	if (!info) {
		png_destroy_read_struct(&png, NULL, NULL);
		fclose(fp);
		fprintf(stderr, "Failed to create PNG info structure.\n");
		exit(EXIT_FAILURE);
	}

	/*

	 * libpng uses setjmp/longjmp for error handling.
	 * If libpng encounters an error, execution jumps here.
	 */
	if (setjmp(png_jmpbuf(png))) {
		png_destroy_read_struct(&png, &info, NULL);
		fclose(fp);
		fprintf(stderr, "Error while reading %s\n", filename);
		exit(EXIT_FAILURE);
	}

	/* Tell libpng to read from our opened file. */
	png_init_io(png, fp);

	/*

	 * Read only the PNG header.
	 * We do NOT read pixel data because we only need the palette.
	 */
	png_read_info(png, info);

	/*

	 * Verify that this PNG is indexed (palettized).
	 * PS1 CLUT textures require indexed PNGs.
	 */
	if (png_get_color_type(png, info) != PNG_COLOR_TYPE_PALETTE) {
		fprintf(stderr, "%s is not an indexed PNG.\n", filename);
		png_destroy_read_struct(&png, &info, NULL);
		fclose(fp);
		exit(EXIT_FAILURE);
	}

	/*

	 * Obtain the PLTE chunk.
	 * png_palette now points to palette_size RGB colors.
	 */
	if (!png_get_PLTE(png, info, &png_palette, &palette_size)) {
		fprintf(stderr, "%s contains no palette.\n", filename);
		png_destroy_read_struct(&png, &info, NULL);
		fclose(fp);
		exit(EXIT_FAILURE);
	}

	/*

	 * Sanity check.
	 * Indexed PNGs should never exceed 256 colors.
	 */
	if (palette_size > 256) {
		fprintf(stderr, "%s palette exceeds 256 colors.\n", filename);
		png_destroy_read_struct(&png, &info, NULL);
		fclose(fp);
		exit(EXIT_FAILURE);
	}

	/*

	 * Check whether a transparency (tRNS) chunk exists.
	 * If present, trans_alpha[i] contains the alpha value
	 * for palette entry i.
	 */
	png_get_tRNS(png, info, &trans_alpha, &num_trans, &trans_color);

	/* Convert every palette color into PSX 15-bit format. */
	for (i = 0; i < palette_size; i++) {
		/* Convert RGB888 -> PSX RGB555. */
		uint16_t color = rgb_to_psx(png_palette[i].red,
				png_palette[i].green,
				png_palette[i].blue);
		/*
		 * If this palette entry is completely transparent,
		 * set the PSX STP bit.
		 * (Assuming bit 15 represents transparency.)
		 */
		if (i < num_trans && trans_alpha[i] == 0)
			color |= 0x8000;

		/* Store into the next row of the palette atlas. */
		t->palette[t->count][i] = color;

	}

	/*

	 * Fill unused palette entries with black.
	 * Every palette in the atlas always contains exactly
	 * 256 entries.
	 */
	for (i = palette_size; i < 256; i++)
		t->palette[t->count][i] = 0;

	/* One more palette has been added to the atlas. */
	t->count++;

	/* Free libpng resources. */
	png_destroy_read_struct(&png, &info, NULL);

	/* Close the PNG file. */
	fclose(fp);

}


void texture_load_clut_png_extract_indices(struct texture_clut_indexed_image *image,
		const char *filename)
{
	png_structp png = NULL;
	png_infop info = NULL;
	png_bytep *rows = NULL;
	FILE *fp = NULL;
	int y; 

	/* Open the PNG file. */
	fp = fopen(filename, "rb");
	if (!fp) {
		perror(filename);
		exit(EXIT_FAILURE);
	}

	/* Create libpng structures. */
	png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png) {
		fclose(fp);
		exit(EXIT_FAILURE);
	}

	info = png_create_info_struct(png);
	if (!info) {
		png_destroy_read_struct(&png, NULL, NULL);
		fclose(fp);
		exit(EXIT_FAILURE);
	}

	/* libpng error handling. */
	if (setjmp(png_jmpbuf(png))) {
		png_destroy_read_struct(&png, &info, NULL);
		fclose(fp);
		exit(EXIT_FAILURE);
	}

	/* Read PNG header. */
	png_init_io(png, fp);
	png_read_info(png, info);

	image->width = png_get_image_width(png, info);
	image->height = png_get_image_height(png, info);

	/* Verify that the PNG is indexed. */
	if (png_get_color_type(png, info) != PNG_COLOR_TYPE_PALETTE) {
		fprintf(stderr, "%s is not an indexed PNG.\n", filename);
		png_destroy_read_struct(&png, &info, NULL);
		fclose(fp);
		exit(EXIT_FAILURE);
	}

	/*

	 * Allocate storage for the palette indices.
	 * Every pixel occupies exactly one byte.
	 */
	image->indices = malloc(image->width * image->height);
	if (!image->indices) {
		fprintf(stderr, "Out of memory.\n");
		png_destroy_read_struct(&png, &info, NULL);
		fclose(fp);
		exit(EXIT_FAILURE);
	}

	/*

	 * Build an array of scanline pointers.
	 * Linux kernel style avoids Variable Length Arrays (VLA) on stack,
	 * so we dynamically allocate rows array.
	 */
	rows = malloc(image->height * sizeof(png_bytep));
	if (!rows) {
		fprintf(stderr, "Out of memory for rows.\n");
		free(image->indices);
		png_destroy_read_struct(&png, &info, NULL);
		fclose(fp);
		exit(EXIT_FAILURE);
	}

	for (y = 0; y < image->height; y++)
		rows[y] = image->indices + (image->height - 1 - y) * image->width;

	/*

	 * Read the indexed image.
	 * Each byte written into image->indices is already a palette
	 * index (0-255).
	 */
	png_read_image(png, rows);

	/* Cleanup. */
	free(rows);
	png_destroy_read_struct(&png, &info, NULL);
	fclose(fp);

}

void texture_clut_palette_atlas_upload(struct texture_clut_palette_atlas *atlas)
{
	/* Create the OpenGL texture. */
	glGenTextures(1, &atlas->id);
	glBindTexture(GL_TEXTURE_2D, atlas->id); 

	/*

	 * Upload every palette as one row.
	 * width  = 256 colors
	 * height = number of palettes
	 */
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5_A1, 256, atlas->count, 0,
			GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, atlas->palette);

	/* No filtering. */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_2D, 0);

} 

void texture_clut_indexed_image_upload(struct texture_clut_indexed_image *image)
{
	/* Create the OpenGL texture. */
	glGenTextures(1, &image->id);
	glBindTexture(GL_TEXTURE_2D, image->id); 

	/*

	 * Upload palette indices.
	 * Every texel stores one unsigned byte.
	 */
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, image->width, image->height, 0,
			GL_RED_INTEGER, GL_UNSIGNED_BYTE, image->indices);

	/* No filtering. */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_2D, 0);

} 

void texture_clut_palette_atlas_bind(const struct texture_clut_palette_atlas *atlas)
{
	glBindTextureUnit(0, atlas->id);
} 

void texture_clut_indexed_image_bind(const struct texture_clut_indexed_image *image)
{
	glBindTextureUnit(1, image->id);
}

void texture_clut_palette_atlas_destroy(struct texture_clut_palette_atlas *atlas)
{
	if (atlas->id) {
		glDeleteTextures(1, &atlas->id);
		atlas->id = 0;
	}
	atlas->count = 0;
}

void texture_clut_indexed_image_destroy(struct texture_clut_indexed_image *image)
{
	if (image->id) {
		glDeleteTextures(1, &image->id);
		image->id = 0;
	}
	if (image->indices) {
		free(image->indices);
		image->indices = NULL;
	}
	image->width = 0;
	image->height = 0;
}
