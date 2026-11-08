#ifndef TEXTURE_H
#define TEXTURE_H

#include <stdint.h> /* for uint16_t */
#include <GL/glew.h>
#include <stddef.h>

/*
 * CLUT palette atlas.
 *
 * Every row contains one 256-color palette.
 */
struct texture_clut_palette_atlas
{
    unsigned int id;
    unsigned int count;

    uint16_t *palette;
};

/*
 * Indexed texture array.
 *
 * Every layer contains one indexed texture.
 * All layers must have identical dimensions.
 */
struct texture_clut_indexed_atlas
{
    unsigned int id;

    unsigned int width;
    unsigned int height;
    unsigned int count;
};

/*
 * CPU-side indexed image.
 *
 * Used as a temporary container before uploading
 * image data into the texture array.
 */
struct texture_clut_indexed_image
{
    unsigned int width;
    unsigned int height;

    unsigned char *indices;
};

/* CLUT palette atlas */

void texture_load_clut_png_extract_palette(
    struct texture_clut_palette_atlas *atlas,
    const char *filename
);

void texture_load_clut_png_extract_indices(
    struct texture_clut_indexed_image *image,
    const char *filename
);

void texture_clut_palette_atlas_upload(
    struct texture_clut_palette_atlas *atlas
);

void texture_clut_palette_atlas_bind(
    const struct texture_clut_palette_atlas *atlas
);

void texture_clut_palette_atlas_destroy(
    struct texture_clut_palette_atlas *atlas
);

/* Indexed texture array */

void texture_clut_indexed_atlas_create(
    struct texture_clut_indexed_atlas *atlas,
    unsigned int width,
    unsigned int height,
    unsigned int count
);

void texture_clut_indexed_atlas_upload_layer(
    struct texture_clut_indexed_atlas *atlas,
    unsigned int layer,
    const struct texture_clut_indexed_image *image
);

void texture_clut_indexed_atlas_bind(
    const struct texture_clut_indexed_atlas *atlas
);

void texture_clut_indexed_atlas_destroy(
    struct texture_clut_indexed_atlas *atlas
);

/* CPU-side indexed image */

void texture_clut_indexed_image_destroy(
    struct texture_clut_indexed_image *image
);

#endif
