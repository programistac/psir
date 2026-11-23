#include "gte.h"

#include <assert.h>
#include <stddef.h> /* NULL */

int16_t gte_project_fullscreen(
        int16_t *out_x,
        int16_t *out_y,
        const int16_t *local_v,
        const struct gte_transform *t)
{
	int16_t screen_v[3];

	assert(out_x != NULL);
	assert(out_y != NULL);
	assert(local_v != NULL);
	assert(t != NULL);

	translate_v3(screen_v, local_v, t->v);

	*out_x = FIX_CLAMP(screen_v[0]);
	*out_y = FIX_CLAMP(screen_v[1]);

	return 1;
}

int16_t gte_project_vertex_2d(
        int16_t *out_x,
        int16_t *out_y,
        const int16_t *local_v,
        const struct gte_transform *t, 
	int16_t internal_w, int16_t internal_h)
{
	int16_t screen_v[3];
	int32_t x_proj, y_proj;

	assert(out_x != NULL);
	assert(out_y != NULL);
	assert(local_v != NULL);
	assert(t != NULL);

	translate_v3(screen_v, local_v, t->v);

	x_proj = (((int32_t)screen_v[0] * internal_h) / internal_w);                
	y_proj = screen_v[1];

	*out_x = FIX_CLAMP(x_proj);
	*out_y = FIX_CLAMP(y_proj);

	return 1;
}

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

void gte_apply_vertex_light(struct gpu_vertex *v, uint8_t light)
{
	uint16_t r = (v->r * light) >> 7;
	uint16_t g = (v->g * light) >> 7;
	uint16_t b = (v->b * light) >> 7;

	v->r = (r > 255) ? 255 : (uint8_t)r;
	v->g = (g > 255) ? 255 : (uint8_t)g;
	v->b = (b > 255) ? 255 : (uint8_t)b;
}

/* -------------------------------------------------------------------------- */
/* Oblicza jasność Lamberta w skali PSX (128 = 1.0, <128 = cień, >128 = overbright) */
/* normal: znormalizowany wektor wierzchołka (Q3.12)                          */
/* light_dir: wektor DO źródła światła w przestrzeni obiektu (Q3.12)          */
/* ambient: bazowe oświetlenie otoczenia (np. 32 = 25% jasności)              */
/* intensity: moc światła kierunkowego (np. 128 = 100%, 192 = 150% overbright)*/
/* -------------------------------------------------------------------------- */
uint8_t gte_calc_lambert(const int16_t *normal, const int16_t *light_dir,
                         uint8_t ambient, uint8_t intensity)
{
	int16_t dot;
	int32_t light;

	assert(normal != NULL);
	assert(light_dir != NULL);

	/* 1. Iloczyn skalarny w Q3.12 (4096 = 1.0) */
	dot = dot_product(normal, light_dir);

	/* 2. Jeśli światło pada od tyłu, zostaje tylko ambient */
	if (dot <= 0) {
		return ambient;
	}

	/* 3. Skalujemy moc światła przez cosinus kąta (Q12 -> skala 0..255) */
	light = ambient + (((int32_t)dot * intensity) >> FIX_SHIFT);

	/* 4. Przycinamy do uint8_t (255 = prawie 2x overbright przy >> 7) */
	return (light > 255) ? 255 : (uint8_t)light;
}

