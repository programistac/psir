#ifndef GTE_H
#define GTE_H

#include <stdint.h>

/* Parametry renderera (wartości przykładowe, idealne dla stylu PSX) */

/* Granica obcinania Z (Wierzchołek za blisko = clipping) */
#define GTE_NEAR_PLANE 4
/* Granica obcinania Z (Wierzchołek za daleko = clipping) */
#define GTE_FAR_PLANE 30000
/* Ogniskowa / Odległość ekranu (odpowiednik rejestru H w GTE) */
#define GTE_SCREEN_H 256

/* =============================================================
   Odpowiednik rejestru MATRIX z PSX (3x3 Rotacja + 3D Translacja)
   ============================================================= */

struct gte_transform {
	int16_t m[9]; /* Macierz 3x3 */
	int16_t v[3]; /* Vector3D */
	int8_t is_identity;
};

int16_t gte_project_vertex_3d(
		int16_t *out_x, int16_t *out_y, int16_t *out_z, 
		const int16_t *local_v, const struct gte_transform *t, 
		int16_t internal_w, int16_t internal_h);
#endif
