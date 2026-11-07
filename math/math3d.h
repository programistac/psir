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


/* LUT dla sin i cos FIXED_POINT 
 * pelny obrot 360 stopni dziele na LUT_SIZE
 * np. 360 / 1024 = 1024 = 0.25 FIXED_ONE
 * */

#define LUT_SIZE 512
#define LUT_MASK (LUT_SIZE -1)
#define PI 3.141592653589323846

/* SIN/COS LUT */
extern int16_t sin_lut[LUT_SIZE];
extern int16_t cos_lut[LUT_SIZE];

#define SIN_LUT(angle_id) (sin_lut[(angle_id) & LUT_MASK])
#define COS_LUT(angle_id) (cos_lut[(angle_id) & LUT_MASK])

/* Matrix and vector math operations */
void identity(int16_t *m);
void multiply_mxm(int16_t *out, const int16_t *a, const int16_t *b);
void multiply_vxm(int16_t *out, const int16_t *a, const int16_t *b);
void translate_v3(int16_t *out, const int16_t *a, const int16_t *offset);

/* Transformation matrix generation functions */
void rotate_x(int16_t *m, float angle_radians);
void rotate_y(int16_t *m, int angle_id);
void rotate_z(int16_t *m, float angle_radians);
void scale(int16_t *m, int16_t sx, int16_t sy, int16_t sz);

/* wypelnij tablice sin_lut */
void math3d_initialize_sin_lut(void);

#endif /* MATH3D_H */
