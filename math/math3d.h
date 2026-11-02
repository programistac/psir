#ifndef MATH3D_H
#define MATH3D_H

#include <stdint.h>

/* Fixed-point math constraints (Q3.12 format for PSX) */
#define FIX_SHIFT	12
#define FIX_ONE		4096 /* 1 << FIX_SHIFT */

/* Trigonometry helper */
#define DEG_TO_RAD(deg)	((deg) * 0.01745329251f)

/* Macros for handling fixed-point limits and overflows */
#define FIX_CLAMP(val) (val) > 32767 ? 32767 : (val) < -32768 ? -32768 : (int16_t)(val)

/* Matrix and vector math operations */
void identity(int16_t *m);
void multiply_mxm(int16_t *out, const int16_t *a, const int16_t *b);
void multiply_vxm(int16_t *out, const int16_t *a, const int16_t *b);
void translate_v3(int16_t *out, const int16_t *a, const int16_t *offset);

/* Transformation matrix generation functions */
void rotate_x(int16_t *m, float angle_radians);
void rotate_y(int16_t *m, float angle_radians);
void rotate_z(int16_t *m, float angle_radians);
void scale(int16_t *m, int16_t sx, int16_t sy, int16_t sz);

#endif /* MATH3D_H */
