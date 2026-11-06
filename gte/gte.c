#include "gte.h"
#include "../math/math3d.h"

#include <assert.h>
#include <stddef.h> /* NULL */

int16_t gte_project_vertex_3d(
		int16_t *out_x, int16_t *out_y, int16_t *out_z,
                const int16_t *local_v, const struct gte_transform *t, 
		int16_t internal_w, int16_t internal_h)
{
	int16_t cam_v[3];
	int32_t x_proj, y_proj, z_proj;

	assert(out_x != NULL);
	assert(out_y != NULL);
	assert(out_z != NULL);
	assert(local_v != NULL);
	assert(t != NULL);

	(void)z_proj; /* swiadomie nieuzywam  */

	/* 1. Transformacja wierzchołka do przestrzeni kamery */
	if (t->is_identity) {
		translate_v3(cam_v, local_v, t->v);
	} else {
		multiply_vxm(cam_v, local_v, t->m);
		translate_v3(cam_v, cam_v, t->v);
	}

	/* 2. Near Clipping */ 
	if (cam_v[2] <= GTE_NEAR_PLANE) {
		return 0;
	}

	/* 3. Projekcja perspektywiczna (Podział przez Z) */
	x_proj = /*cam_v[0];*/((int32_t)cam_v[0] * GTE_SCREEN_H) / cam_v[2];
	y_proj = /*cam_v[1];*/((int32_t)cam_v[1] * GTE_SCREEN_H) / cam_v[2];

        x_proj = (x_proj * ((1 << 15) - 1)) / (internal_w / 2);
        y_proj = (y_proj * ((1 << 15) - 1)) / (internal_h / 2);

        *out_x = FIX_CLAMP(x_proj);
        *out_y = FIX_CLAMP(y_proj);
        *out_z = FIX_CLAMP(cam_v[2]); /* zwracam z - TODO: brak sprawdzenia czy lezy w 0..32767 */

        return 1;

}
