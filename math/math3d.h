#ifndef MATH3D_H
#define MATH3D_H

#include <stdint.h>

/* Arytmetyka staloprzecinkowa (Q3.12 format) 
 * 12 bitow przeznaczonych jest na czesc ulamkowa.
 */
#define FIX_SHIFT	12
#define FIX_ONE		4096 /* 1 << FIX_SHIFT */

/* Ograniczenie wartosci do zakresu int16_t(16bit). */
#define FIX_CLAMP(val) (val) > 32767 ? 32767 : (val) < -32768 ? -32768 : (int16_t)(val)


/* Tablice wartosci funkcji trygonometrycznych.
 *
 * Pelny obrot 360 stopni dziele na LUT_SIZE
 * przedzialow katowych.
 */

#define LUT_SIZE 1024
#define LUT_MASK (LUT_SIZE -1)
#define PI 3.141592653589323846

/* Tablice obliczonych wartosci sinusa i cosinusa. */
extern int16_t sin_lut[LUT_SIZE];
extern int16_t cos_lut[LUT_SIZE];

/* Pobieranie wartosci funkcji trygonometrycznych na podstawie
 * indeksu kata.
 *
 * Operacja AND z maska LUT_MASK ograniczy indeks do zakresu
 * [0, LUT_SIZE -1], umozliwiajac cykliczne indeksowanie tablicy.
 * 1000000000000 = 4096
 * AND
 * 011111111111 = 4095
 * =
 * 000000000000 
 */
#define SIN_LUT(angle_id) (sin_lut[(angle_id) & LUT_MASK])
#define COS_LUT(angle_id) (cos_lut[(angle_id) & LUT_MASK])

/* Operacje na macierzach i wektorach. */
void identity(int16_t *m);
void multiply_mxm(int16_t *out, const int16_t *a, const int16_t *b);
void multiply_vxm(int16_t *out, const int16_t *a, const int16_t *b);

/* Transformacje geometryczne.*/
void translate_v3(int16_t *out, const int16_t *a, const int16_t *offset);
void rotate_x(int16_t *m, int angle_id);
void rotate_y(int16_t *m, int angle_id);
void rotate_z(int16_t *m, int angle_id);
void scale(int16_t *m, int16_t sx, int16_t sy, int16_t sz);

/* Inicializacja tablic wartosci funkcji trygonometrycznych. */
void math3d_initialize_sin_lut(void);

#endif /* MATH3D_H */
