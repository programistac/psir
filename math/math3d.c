#include "math3d.h"
#include <math.h>

#include <stdio.h>

void identity(int16_t *m)
{
	int i = 0;

	for (i = 0; i < 9; i++) {
		m[i] = 0;
	}

	m[0] = FIX_ONE; /* 1.0 = 4096 */
	m[4] = FIX_ONE;
	m[8] = FIX_ONE;
}

/* wywolywanie multiply_mxm(A, A, B); wynik trafia do A */

void multiply_mxm(int16_t *out, const int16_t *a, const int16_t *b)
{
	int16_t result[9];
	int c = 0;
	int r = 0;
	int i = 0;

	for (r = 0; r < 3; r++) {
		for (c = 0; c < 3; c++) {
			int64_t sum;
			int32_t fixed_result;

			sum = a[r * 3 + 0] * b[0 * 3 + c] +
				a[r * 3 + 1] * b[1 * 3 + c] +
				a[r * 3 + 2] * b[2 * 3 + c];

			fixed_result = (int32_t)(sum >> FIX_SHIFT);
			result[r * 3 + c] = FIX_CLAMP(fixed_result);

		}
	}

	for (i = 0; i < 9; i++) {
		out[i] = result[i];
	}
}

/* -------------------------------------------------------------------------- */
/* 3. Mnożenie Wektora przez Macierz (Wektor 1x3 = Wektor 1x3 * Macierz 3x3)  */
/*    Umożliwia bezpieczne wywołanie typu: multiply_vxm(v, v, M);             */
/* -------------------------------------------------------------------------- */
void multiply_vxm(int16_t *out, const int16_t *a, const int16_t *b)
{
	int16_t result[3];
	int c = 0;
	int i = 0;

	/* Pętla przechodzi po kolumnach macierzy b (odpowiadających składowym X, Y, Z wyniku) */
	for (c = 0; c < 3; c++) {
		int32_t sum;

		/* Mnożymy wiersz wektora przez kolumnę c macierzy b */
		sum = ((int32_t)a[0] * b[0 * 3 + c]) +
			((int32_t)a[1] * b[1 * 3 + c]) +
			((int32_t)a[2] * b[2 * 3 + c]);

		/* Powrót z formatu Q24 do formatu Q12 */
		sum >>= FIX_SHIFT;

		/* Nasycenie za pomocą Twojego makra */
		result[c] = FIX_CLAMP(sum);
	}

	/* Bezpieczne przepisanie do tablicy wyjściowej */
	for (i = 0; i < 3; i++) {
		out[i] = result[i];
	}
}

/* -------------------------------------------------------------------------- */
/* 4. Translacja (Dodawanie dwóch wektorów 1x3 w pętli)                      */
/*    Umożliwia bezpieczne wywołanie typu: translate_v3(v, v, offset);        */
/* -------------------------------------------------------------------------- */
void translate_v3(int16_t *out, const int16_t *a, const int16_t *offset)
{
	int16_t result[3];
	int i = 0;

	for (i = 0; i < 3; i++) {
		int32_t sum;

		/* Dodawanie w 32 bitach chroniące przed przepełnieniem */
		sum = (int32_t)a[i] + offset[i];

		result[i] = FIX_CLAMP(sum);
	}

	for (i = 0; i < 3; i++) {
		out[i] = result[i];
	}
}


/* -------------------------------------------------------------------------- */
/* Generowanie macierzy rotacji wokół osi Y (Q3.12)   */
/* -------------------------------------------------------------------------- */
void rotate_y(int16_t *m, float angle_radians)
{
	int16_t cos_a;
	int16_t sin_a;

	/* 1. Ustawiamy macierz jednostkową */
	identity(m);

	/* 2. Liczymy wartości trygonometryczne */
	cos_a = (int16_t)(cos(angle_radians) * 4096.0f);
	sin_a = (int16_t)(sin(angle_radians) * 4096.0f);

	/* 3. Nadpisujemy konkretne komórki bezpośrednio ich numerami w pamięci */
	m[0] = cos_a;   /* Wiersz 0, Kolumna 0 */
	m[2] = sin_a;   /* Wiersz 0, Kolumna 2 */

	/* m[4] to środek (Y) - zostaje jako 4096 z identity() */

	m[6] = -sin_a;  /* Wiersz 2, Kolumna 0 */
	m[8] = cos_a;   /* Wiersz 2, Kolumna 2 */
}

void rotate_x(int16_t *m, float angle_radians)
{
	int16_t cos_a = (int16_t)(cos(angle_radians) * 4096.0f);
	int16_t sin_a = (int16_t)(sin(angle_radians) * 4096.0f);

	identity(m);

	/* m[0] zostaje jako 4096 */
	m[4] = cos_a;   /* Wiersz 1, Kolumna 1 */
	m[5] = -sin_a;  /* Wiersz 1, Kolumna 2 */
	m[7] = sin_a;   /* Wiersz 2, Kolumna 1 */
	m[8] = cos_a;   /* Wiersz 2, Kolumna 2 */
}

void rotate_z(int16_t *m, float angle_radians)
{
	int16_t cos_a = (int16_t)(cos(angle_radians) * 4096.0f);
	int16_t sin_a = (int16_t)(sin(angle_radians) * 4096.0f);

	identity(m);

	m[0] = cos_a;   /* Wiersz 0, Kolumna 0 */
	m[1] = -sin_a;  /* Wiersz 0, Kolumna 1 */
	m[3] = sin_a;   /* Wiersz 1, Kolumna 0 */
	m[4] = cos_a;   /* Wiersz 1, Kolumna 1 */
	/* m[8] zostaje jako 4096 */
}

void scale(int16_t *m, int16_t sx, int16_t sy, int16_t sz)
{
	identity(m);

	m[0] = sx;
	m[4] = sy;
	m[8] = sz;
}
