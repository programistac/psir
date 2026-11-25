#include "math3d.h"
#include <math.h>
#include <stddef.h> /* NULL */
#include <assert.h>

/* Wewnetrzna biblioteka matematyczna 
 * Wszystkie wskazniki przekazywane do funkcji musza byc poprawne!
 * Poprawnosc jest weryfikowana za pomoca assert
 */

/* Celowo uzywam int dla licznikow petli.  
 * Typy o stalej szerokosci int*_t
 * sa zarezerwowane dla danych renderera i obliczen
 * gdzie istotny jest dokladny rozmiar 
 */


int16_t sin_lut[LUT_SIZE];
int16_t cos_lut[LUT_SIZE];

void identity(int16_t *m)
{
	int i = 0;

	assert(m != NULL);

	/* memset(m, 0, sizeof(int16_t) * 9);  */

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

	assert(out != NULL);
	assert(a != NULL);
	assert(b != NULL);

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

	assert(out != NULL);
	assert(a != NULL);
	assert(b != NULL);

	/* Pętla przechodzi po kolumnach macierzy b (odpowiadających składowym X, Y, Z wyniku) */
	for (c = 0; c < 3; c++) {
		/* przepelnienie? przy skrajnych wartosciach...  */
		int32_t sum;

		/* Mnożymy wiersz wektora przez kolumnę c macierzy b */
		sum = ((int32_t)a[0] * b[0 * 3 + c]) +
			((int32_t)a[1] * b[1 * 3 + c]) +
			((int32_t)a[2] * b[2 * 3 + c]);

		/* Powrót z formatu Q24 do formatu Q12 */
		sum >>= FIX_SHIFT;

		/* Nasycenie za pomocą makra */
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

	assert(out != NULL);
	assert(a != NULL);
	assert(offset != NULL);

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
/* Generowanie macierzy rotacji wokół osi Y (Q3.12)                           */
/* dla układu LEWOSKRĘTNEGO (+Z w głąb)					      */
/* -------------------------------------------------------------------------- */
void rotate_y(int16_t *m, int angle_id)
{
	int16_t cos_a;
	int16_t sin_a;

	assert(m != NULL);

	/* 1. Ustawiamy macierz jednostkową */
	identity(m);

	/* Pobieram wartosci sin/cos indeksem z tablicy */

	sin_a = SIN_LUT(angle_id);
	cos_a = COS_LUT(angle_id);

	/* 3. Nadpisujemy konkretne komórki bezpośrednio ich numerami w pamięci */
	m[0] = cos_a;   /* Wiersz 0, Kolumna 0 */
	m[2] = -sin_a;   /* Wiersz 0, Kolumna 2 */

	/* m[4] to środek (Y) - zostaje jako 4096 z identity() */

	m[6] = sin_a;  /* Wiersz 2, Kolumna 0 */
	m[8] = cos_a;   /* Wiersz 2, Kolumna 2 */
}

void rotate_x(int16_t *m, int angle_id)
{
	int16_t cos_a = COS_LUT(angle_id);
	int16_t sin_a = SIN_LUT(angle_id);

	assert(m != NULL);

	identity(m);

	/* m[0] zostaje jako 4096 */
	m[4] = cos_a;   /* Wiersz 1, Kolumna 1 */
	m[5] = sin_a;  /* Wiersz 1, Kolumna 2 */
	m[7] = -sin_a;   /* Wiersz 2, Kolumna 1 */
	m[8] = cos_a;   /* Wiersz 2, Kolumna 2 */
}

void rotate_z(int16_t *m, int angle_id)
{
	int16_t cos_a = COS_LUT(angle_id);
	int16_t sin_a = SIN_LUT(angle_id);

	assert(m != NULL);

	identity(m);

	m[0] = cos_a;   /* Wiersz 0, Kolumna 0 */
	m[1] = sin_a;  /* Wiersz 0, Kolumna 1 */
	m[3] = -sin_a;   /* Wiersz 1, Kolumna 0 */
	m[4] = cos_a;   /* Wiersz 1, Kolumna 1 */
	/* m[8] zostaje jako 4096 */
}

void scale(int16_t *m, int16_t sx, int16_t sy, int16_t sz)
{
	assert(m != NULL);

	identity(m);

	m[0] = sx;
	m[4] = sy;
	m[8] = sz;
}

void math3d_initialize_sin_lut(void)
{
	int i;
	double angle;

	/* wypelnij wartosciami Q3.12 */
	for (i = 0; i < LUT_SIZE; i++) {
		/* rozdziel pelny obrot(360stopni = 2*PI radianow)  
		 * na LUT_SIZE rownych krokow
		 *
		 * wyznaczam jaka czesc pelnego obrotu reprezentuje indeks i
		 */
		angle = (2 * PI * i) / LUT_SIZE;

		sin_lut[i] = (int16_t)(sin(angle) * FIX_ONE);
		cos_lut[i] = (int16_t)(cos(angle) * FIX_ONE);
	}
}

/* -------------------------------------------------------------------------- */
/* Transpozycja macierzy 3x3 (zamiana wierszy z kolumnami)                    */
/* Umożliwia bezpieczne wywołanie typu: transpose_m(M, M);                    */
/* -------------------------------------------------------------------------- */
void transpose_m(int16_t *out, const int16_t *m)
{
	int16_t tmp;

	assert(out != NULL);
	assert(m != NULL);

	/* Przekątna główna zostaje bez zmian */
	out[0] = m[0];
	out[4] = m[4];
	out[8] = m[8];

	/* Zamiana elementów symetrycznych (używam tmp, by obsłużyć out == m) */
	tmp = m[1];
	out[1] = m[3];
	out[3] = tmp;

	tmp = m[2];
	out[2] = m[6];
	out[6] = tmp;

	tmp = m[5];
	out[5] = m[7];
	out[7] = tmp;
}

/* -------------------------------------------------------------------------- */
/* Iloczyn skalarny dwóch wektorów 1x3 w formacie Q3.12                       */
/* Zwraca wartość w formacie Q3.12 (4096 = 1.0)                               */
/* -------------------------------------------------------------------------- */
int16_t dot_product(const int16_t *a, const int16_t *b)
{
	int32_t sum;

	assert(a != NULL);
	assert(b != NULL);

	/* Rzutowanie na int32_t chroni przed przepełnieniem przy mnożeniu */
	sum = ((int32_t)a[0] * b[0]) +
	      ((int32_t)a[1] * b[1]) +
	      ((int32_t)a[2] * b[2]);

	/* Powrót z formatu Q24 (12 + 12 bitów po przecinku) do Q12 */
	sum >>= FIX_SHIFT;

	return FIX_CLAMP(sum);
}

/* -------------------------------------------------------------------------- */
/* Pomocniczy całkowitoliczbowy pierwiastek kwadratowy (32-bit -> 16-bit)      */
/* -------------------------------------------------------------------------- */
uint16_t isqrt32(uint32_t n)
{
	uint32_t root = 0;
	uint32_t bit = (uint32_t)1 << 30; /* Najwyższa potęga 4 mieszcząca się w uint32_t */

	while (bit > n) {
		bit >>= 2;
	}

	while (bit != 0) {
		if (n >= root + bit) {
			n -= root + bit;
			root = (root >> 1) + bit;
		} else {
			root >>= 1;
		}
		bit >>= 2;
	}

	return (uint16_t)root;
}

/* -------------------------------------------------------------------------- */
/* Długość wektora 1x3 w formacie Q3.12                                       */
/* Zwraca długość w formacie Q3.12 (np. 4096 = 1.0)                           */
/* -------------------------------------------------------------------------- */
int16_t length_v3(const int16_t *v)
{
	uint64_t sq_sum;
	uint16_t len;

	assert(v != NULL);

	/* Suma kwadratów w Q24 (bezpieczna do składowych ok. +-26000, czyli +-6.3 w Q3.12) */
	sq_sum = (uint64_t)((int64_t)v[0] * v[0]) +
	         (uint64_t)((int64_t)v[1] * v[1]) +
	         (uint64_t)((int64_t)v[2] * v[2]);

	/* Pierwiastek z liczby Q24 automatycznie daje wynik w Q12! */
	len = (uint32_t)isqrt32(sq_sum);

	/* return FIX_CLAMP((int32_t)len); -- ERROR? */
	return (len > 32767) ? 32767 : (int16_t)len;
}

/* -------------------------------------------------------------------------- */
/* Normalizacja wektora 1x3 (zamiana na wektor jednostkowy o długości 4096)   */
/* Umożliwia bezpieczne wywołanie typu: normalize_v3(v, v);                   */
/* -------------------------------------------------------------------------- */
void normalize_v3(int16_t *out, const int16_t *v)
{
	int16_t len;
	int i;

	assert(out != NULL);
	assert(v != NULL);

	len = length_v3(v);

	/* Zabezpieczenie przed dzieleniem przez zero dla wektora zerowego */
	if (len == 0) {
		out[0] = 0;
		out[1] = 0;
		out[2] = 0;
		return;
	}

	/* Dzielenie w Q3.12: przesuwamy licznik o FIX_SHIFT (12 bitów) w lewo */
	for (i = 0; i < 3; i++) {
		int32_t scaled = ((int32_t)v[i] << FIX_SHIFT) / len;
		out[i] = FIX_CLAMP(scaled);
	}
}

/* -------------------------------------------------------------------------- */
/* Odejmowanie dwóch wektorów 1x3 (out = a - b)                               */
/* Umożliwia bezpieczne wywołanie typu: sub_v3(v, v, target);                 */
/* -------------------------------------------------------------------------- */
void sub_v3(int16_t *out, const int16_t *a, const int16_t *b)
{
	int16_t result[3];
	int i = 0;

	assert(out != NULL);
	assert(a != NULL);
	assert(b != NULL);

	for (i = 0; i < 3; i++) {
		int32_t diff = (int32_t)a[i] - b[i];
		result[i] = FIX_CLAMP(diff);
	}

	for (i = 0; i < 3; i++) {
		out[i] = result[i];
	}
}
