
#include "texture.h"

#include <GL/glew.h>
#include <png.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/*
 * Convert RGB888 into the packed format expected by
 * GL_RGB5_A1 with GL_UNSIGNED_SHORT_5_5_5_1.
 *
 * Bit layout:
 * RRRRRGGGGGBBBBBA
 *
 * The lowest bit is the OpenGL alpha bit.
 */
static uint16_t rgb_to_psx(uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t r5 = r >> 3;
    uint16_t g5 = g >> 3;
    uint16_t b5 = b >> 3;

    return (r5 << 11) | (g5 << 6) | (b5 << 1) | 1;
}

/* --------------------------------------------------------- */
/* PNG: extract palette                                      */
/* --------------------------------------------------------- */

void texture_load_clut_png_extract_palette(
    struct texture_clut_palette_atlas *atlas,
    const char *filename)
{
    png_structp png = NULL;
    png_infop info = NULL;

    png_bytep trans_alpha = NULL;
    png_color_16p trans_color = NULL;
    png_colorp png_palette = NULL;

    int palette_size = 0;
    int num_trans = 0;
    int i;

    FILE *fp = NULL;
    uint16_t *new_palette = NULL;

    if (!atlas || !filename) {
        fprintf(stderr, "Invalid palette loader arguments.\n");
        exit(EXIT_FAILURE);
    }

    fp = fopen(filename, "rb");

    if (!fp) {
        perror(filename);
        exit(EXIT_FAILURE);
    }

    png = png_create_read_struct(
        PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png) {
        fclose(fp);
        fprintf(stderr, "Failed to create PNG read structure.\n");
        exit(EXIT_FAILURE);
    }

    info = png_create_info_struct(png);

    if (!info) {
        png_destroy_read_struct(&png, NULL, NULL);
        fclose(fp);
        fprintf(stderr, "Failed to create PNG info structure.\n");
        exit(EXIT_FAILURE);
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        fprintf(stderr, "Error while reading %s\n", filename);
        exit(EXIT_FAILURE);
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    if (png_get_color_type(png, info) != PNG_COLOR_TYPE_PALETTE) {
        fprintf(stderr, "%s is not an indexed PNG.\n", filename);
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    if (!png_get_PLTE(png, info, &png_palette, &palette_size)) {
        fprintf(stderr, "%s contains no palette.\n", filename);
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    if (palette_size > 256) {
        fprintf(stderr, "%s palette exceeds 256 colors.\n", filename);
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    png_get_tRNS(
        png,
        info,
        &trans_alpha,
        &num_trans,
        &trans_color
    );

    /*
     * Allocate space for one additional palette.
     * Each palette contains exactly 256 uint16_t entries.
     */
    if ((size_t)atlas->count + 1 > SIZE_MAX / (256 * sizeof(uint16_t))) {
        fprintf(stderr, "Palette atlas size overflow.\n");
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    new_palette = realloc(
        atlas->palette,
        ((size_t)atlas->count + 1) * 256 * sizeof(uint16_t)
    );

    if (!new_palette) {
        fprintf(stderr, "Failed to allocate palette memory.\n");
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    atlas->palette = new_palette;

    /*
     * Fill the new palette row with black and opaque entries.
     */
    for (i = 0; i < 256; i++)
        atlas->palette[(size_t)atlas->count * 256 + i] = 1;

    /*
     * Convert PNG colors into packed RGB5_A1 values.
     */
    for (i = 0; i < palette_size; i++) {
        uint16_t color = rgb_to_psx(
            png_palette[i].red,
            png_palette[i].green,
            png_palette[i].blue
        );

        /*
         * PNG transparency is mapped to the OpenGL alpha bit.
         * A fully transparent entry gets alpha = 0.
         */
        if (i < num_trans && trans_alpha[i] == 0)
            color &= (uint16_t)~1u;

        atlas->palette[(size_t)atlas->count * 256 + i] = color;
    }

    atlas->count++;

    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);
}

/* --------------------------------------------------------- */
/* PNG: extract indexed pixels                               */
/* --------------------------------------------------------- */

void texture_load_clut_png_extract_indices(
    struct texture_clut_indexed_image *image,
    const char *filename)
{
    png_structp png = NULL;
    png_infop info = NULL;
    png_bytep *rows = NULL;

    FILE *fp = NULL;

    png_uint_32 width;
    png_uint_32 height;
    png_size_t rowbytes;

    int bit_depth;
    int color_type;
    int y;

    if (!image || !filename) {
        fprintf(stderr, "Invalid indexed image loader arguments.\n");
        exit(EXIT_FAILURE);
    }

    image->width = 0;
    image->height = 0;
    image->indices = NULL;

    fp = fopen(filename, "rb");

    if (!fp) {
        perror(filename);
        exit(EXIT_FAILURE);
    }

    png = png_create_read_struct(
        PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png) {
        fclose(fp);
        fprintf(stderr, "Failed to create PNG read structure.\n");
        exit(EXIT_FAILURE);
    }

    info = png_create_info_struct(png);

    if (!info) {
        png_destroy_read_struct(&png, NULL, NULL);
        fclose(fp);
        fprintf(stderr, "Failed to create PNG info structure.\n");
        exit(EXIT_FAILURE);
    }

    if (setjmp(png_jmpbuf(png))) {
        free(rows);
        free(image->indices);

        image->indices = NULL;
        image->width = 0;
        image->height = 0;

        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);

        fprintf(stderr, "Error while reading %s\n", filename);
        exit(EXIT_FAILURE);
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    width = png_get_image_width(png, info);
    height = png_get_image_height(png, info);

    bit_depth = png_get_bit_depth(png, info);
    color_type = png_get_color_type(png, info);

    if (color_type != PNG_COLOR_TYPE_PALETTE) {
        fprintf(stderr, "%s is not an indexed PNG.\n", filename);
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    if (width == 0 || height == 0 ||
        width > UINT_MAX || height > UINT_MAX ||
        (size_t)width > SIZE_MAX / (size_t)height) {
        fprintf(stderr, "Invalid image dimensions in %s.\n", filename);
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    /*
     * PNG palette images may use 1, 2, 4 or 8 bits per index.
     * We need exactly one byte per pixel for GL_R8UI.
     */
    if (bit_depth < 8)
        png_set_packing(png);

    png_read_update_info(png, info);

    rowbytes = png_get_rowbytes(png, info);

    if (rowbytes != width) {
        fprintf(stderr, "Unexpected PNG row size in %s.\n", filename);
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    image->width = (unsigned int)width;
    image->height = (unsigned int)height;

    image->indices = malloc((size_t)width * height);

    if (!image->indices) {
        fprintf(stderr, "Out of memory for indexed image.\n");
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    rows = malloc((size_t)height * sizeof(png_bytep));

    if (!rows) {
        fprintf(stderr, "Out of memory for PNG rows.\n");
        free(image->indices);
        image->indices = NULL;

        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    /*
     * Reverse Y so the image keeps the same orientation
     * as in the original implementation.
     */
    for (y = 0; y < (int)height; y++) {
        rows[y] = image->indices +
            (size_t)(height - 1 - y) * width;
    }

    png_read_image(png, rows);

    free(rows);

    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);
}

/* --------------------------------------------------------- */
/* Palette atlas: upload                                     */
/* --------------------------------------------------------- */

void texture_clut_palette_atlas_upload(
    struct texture_clut_palette_atlas *atlas)
{
    GLint old_unpack_alignment;

    if (!atlas || !atlas->palette || atlas->count == 0) {
        fprintf(stderr, "Palette atlas is empty.\n");
        exit(EXIT_FAILURE);
    }

    if (atlas->id)
        glDeleteTextures(1, &atlas->id);

    glGenTextures(1, &atlas->id);
    glBindTexture(GL_TEXTURE_2D, atlas->id);

    glGetIntegerv(GL_UNPACK_ALIGNMENT, &old_unpack_alignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    /*
     * One row = one 256-color palette.
     */
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB5_A1,
        256,
        (GLsizei)atlas->count,
        0,
        GL_RGBA,
        GL_UNSIGNED_SHORT_5_5_5_1,
        atlas->palette
    );

    glPixelStorei(GL_UNPACK_ALIGNMENT, old_unpack_alignment);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
}

/* --------------------------------------------------------- */
/* Indexed texture atlas: create                             */
/* --------------------------------------------------------- */

void texture_clut_indexed_atlas_create(
    struct texture_clut_indexed_atlas *atlas,
    unsigned int width,
    unsigned int height,
    unsigned int count)
{
    if (!atlas || width == 0 || height == 0 || count == 0) {
        fprintf(stderr, "Invalid indexed atlas dimensions.\n");
        exit(EXIT_FAILURE);
    }

    if (atlas->id)
        glDeleteTextures(1, &atlas->id);

    atlas->id = 0;
    atlas->width = width;
    atlas->height = height;
    atlas->count = count;

    glGenTextures(1, &atlas->id);
    glBindTexture(GL_TEXTURE_2D_ARRAY, atlas->id);

    /*
     * Allocate all layers.
     * Every layer has identical dimensions.
     * Each texel stores one unsigned palette index.
     */
    glTexImage3D(
        GL_TEXTURE_2D_ARRAY,
        0,
        GL_R8UI,
        (GLsizei)width,
        (GLsizei)height,
        (GLsizei)count,
        0,
        GL_RED_INTEGER,
        GL_UNSIGNED_BYTE,
        NULL
    );

    glTexParameteri(
        GL_TEXTURE_2D_ARRAY,
        GL_TEXTURE_MIN_FILTER,
        GL_NEAREST
    );

    glTexParameteri(
        GL_TEXTURE_2D_ARRAY,
        GL_TEXTURE_MAG_FILTER,
        GL_NEAREST
    );

    glTexParameteri(
        GL_TEXTURE_2D_ARRAY,
        GL_TEXTURE_WRAP_S,
        GL_CLAMP_TO_EDGE
    );

    glTexParameteri(
        GL_TEXTURE_2D_ARRAY,
        GL_TEXTURE_WRAP_T,
        GL_CLAMP_TO_EDGE
    );

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

/* --------------------------------------------------------- */
/* Indexed texture atlas: upload one layer                   */
/* --------------------------------------------------------- */

void texture_clut_indexed_atlas_upload_layer(
    struct texture_clut_indexed_atlas *atlas,
    unsigned int layer,
    const struct texture_clut_indexed_image *image)
{
    GLint old_unpack_alignment;

    if (!atlas || !image || !image->indices) {
        fprintf(stderr, "Invalid indexed atlas upload arguments.\n");
        exit(EXIT_FAILURE);
    }

    if (!atlas->id || layer >= atlas->count) {
        fprintf(stderr, "Invalid indexed atlas layer.\n");
        exit(EXIT_FAILURE);
    }

    if (image->width != atlas->width ||
        image->height != atlas->height) {
        fprintf(stderr, "Indexed image dimensions do not match atlas.\n");
        exit(EXIT_FAILURE);
    }

    glBindTexture(GL_TEXTURE_2D_ARRAY, atlas->id);

    glGetIntegerv(GL_UNPACK_ALIGNMENT, &old_unpack_alignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    /*
     * Upload image data into the selected array layer.
     */
    glTexSubImage3D(
        GL_TEXTURE_2D_ARRAY,
        0,
        0,
        0,
        (GLint)layer,
        (GLsizei)image->width,
        (GLsizei)image->height,
        1,
        GL_RED_INTEGER,
        GL_UNSIGNED_BYTE,
        image->indices
    );

    glPixelStorei(GL_UNPACK_ALIGNMENT, old_unpack_alignment);

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

/* --------------------------------------------------------- */
/* Texture binding                                           */
/* --------------------------------------------------------- */

void texture_clut_palette_atlas_bind(
    const struct texture_clut_palette_atlas *atlas)
{
    glBindTextureUnit(0, atlas->id);
}

void texture_clut_indexed_atlas_bind(
    const struct texture_clut_indexed_atlas *atlas)
{
    glBindTextureUnit(1, atlas->id);
}

/* --------------------------------------------------------- */
/* Destruction                                                */
/* --------------------------------------------------------- */

void texture_clut_palette_atlas_destroy(
    struct texture_clut_palette_atlas *atlas)
{
    if (!atlas)
        return;

    if (atlas->id) {
        glDeleteTextures(1, &atlas->id);
        atlas->id = 0;
    }

    free(atlas->palette);
    atlas->palette = NULL;
    atlas->count = 0;
}

void texture_clut_indexed_atlas_destroy(
    struct texture_clut_indexed_atlas *atlas)
{
    if (!atlas)
        return;

    if (atlas->id) {
        glDeleteTextures(1, &atlas->id);
        atlas->id = 0;
    }

    atlas->width = 0;
    atlas->height = 0;
    atlas->count = 0;
}

void texture_clut_indexed_image_destroy(
    struct texture_clut_indexed_image *image)
{
    if (!image)
        return;

    free(image->indices);

    image->indices = NULL;
    image->width = 0;
    image->height = 0;
}

