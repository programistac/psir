#ifndef GPU_H
#define GPU_H

#include <stdint.h>

/* =======================================================
   PSX Render Modes
   ======================================================= */

enum gpu_render_modes {
	GPU_RENDER_FLAT = 0,
	GPU_RENDER_FLAT_GOURAUD,
	GPU_RENDER_TEXTURED_FLAT,
	GPU_RENDER_TEXTURED_GOURAUD,
	GPU_RENDER_MODE_NUM
};

/* =======================================================
   PSX Vertex (wierzcholek)
   ======================================================= */

struct gpu_vertex {
	int16_t x, y, z; /* Pozycja */

	/* Oszczędność pamięci:
	   Zamiast 12 bajtów (3x float) albo 6 bajtów (3x int16_t),
	   cały kolor dla jednego wierzchołka zajmuje teraz dokładnie 4 bajty. */
	uint8_t r, g, b, a; /* Kolor */
	uint8_t u, v; /* Pozycja Textury */

	uint16_t texture_layer;
	uint16_t palette_id;

	enum gpu_render_modes render_mode;

	/* wyrownanie 6 4 2 4 = 16 jest ok!  */
};


/* =======================================================
   GPU Maksymalna ilsoc wierzcholkow
   ======================================================= */
#define GPU_MAX_VERTICES 1024


/* =======================================================
   GPU VAO IDs
   ======================================================= */
enum vao_ids {
	VAO_DATA = 0,
	VAO_NUM
};

/* =======================================================
   GPU Buffer IDs
   ======================================================= */

enum buffer_ids {
	BUFFER_VERTEX = 0,
	BUFFER_NUM
};

/* =======================================================
   GPU Attrib IDs
   ======================================================= */

enum attrib_ids {
	ATTRIB_POSITION = 0,
	ATTRIB_COLOR,
	ATTRIB_TEXTURE,
	ATTRIB_RENDER_MODE,
	ATTRIB_TEXTURE_LAYER,
	ATTRIB_PALETTE_ID,
	ATTRIB_NUM
};

struct gpu_data {
	unsigned int vaos[VAO_NUM];	/* Przechowuje konfiguracje sposobu pobierania 
								   danych wierzcholkow */
	unsigned int buffers[BUFFER_NUM]; /* przechowuje dane wierzcholkow w pamieci GPU */
	struct gpu_vertex gpu_primitive_stream[GPU_MAX_VERTICES];
	unsigned int gpu_vertex_count;
};

/* ==========================================================
   GPU Funkcje
   ========================================================== */
void gpu_create(struct gpu_data *data);
void gpu_destroy(struct gpu_data *data);

/* wysyla dane do vbo buffer */
void gpu_receive_data(struct gpu_data *data);
void gpu_reset_stream(struct gpu_data *data);

/* dodaje trojkat do potoku */
void gpu_submit_triangle(
	struct gpu_data *gpu,
	struct gpu_vertex *a,
	struct gpu_vertex *b,
	struct gpu_vertex *c
);

#endif
