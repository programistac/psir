#ifndef GTE_H
#define GTE_H

#include "../math/math3d.h"
#include "../gpu/gpu.h"

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

int16_t gte_project_fullscreen(
        int16_t *out_x,
        int16_t *out_y,
        const int16_t *local_v,
        const struct gte_transform *t);

int16_t gte_project_vertex_2d(
        int16_t *out_x,
        int16_t *out_y,
        const int16_t *local_v,
        const struct gte_transform *t, 
	int16_t internam_w, int16_t internal_h);

void gte_apply_vertex_light(struct gpu_vertex *v, uint8_t light);

/* TODO: lambert 
Jak to dokładnie działało na PSX:

1. W pliku modelu (format TMD / HMD):
Geometria składała się z osobnych tablic:
	• Tablica wierzchołków (SVECTOR – pozycje X, Y, Z)
	• Tablica wektorów normalnych (SVECTOR – znormalizowane kierunki NX, NY, NZ w formacie 1.12, gdzie 4096 = 1.0)
	• Tablica prymitywów (trójkątów/quadów), która trzymała osobne indeksy do pozycji i osobne do normalnych (dzięki temu można było łatwo robić zarówno flat shading – 1 normalna na trójkąt, jak i Gouraud shading – 3 różne normalne na wierzchołki).
2. Co działo się w locie (w GTE):
Kiedy obiekt się obracał, nie obracałeś wektorów normalnych obiektu! Zamiast tego stosowano sprytny trik matematyczny:
	• Zamiast mnożyć setki wektorów normalnych przez macierz obrotu obiektu, brało się odwrotność (transpozycję) macierzy obrotu obiektu i mnożyło przez nią tylko 3 wektory kierunkowe świateł (w przestrzeni świata).
	• Dzięki temu światła lądowały w lokalnej przestrzeni obiektu (Object Space), a wektory normalne z pliku modelu można było wrzucać bezpośrednio do GTE bez żadnej transformacji!

Jak zaimplementować Lamberta w Twoim stylu (PSX-like):

Jeśli chcesz dodać model Lamberta (światło kierunkowe), zrób to dokładnie tak, jak robiło to GTE w rozkazach typu NCDS / NCCS:
1. Dodaj normal (wektor 3D) do struktury wierzchołka. Upewnij się, że ma długość 1.0 (lub 4096, jeśli piszesz na liczbach stałoprzecinkowych).
2. Przelicz kierunek światła do przestrzeni lokalnej obiektu (przemnóż wektor światła przez odwrotność macierzy rotacji obiektu). Jeśli obiekt się nie obraca (jest statyczny), ten krok pomijasz.
3. Policz iloczyn skalarny (Dot Product):
intensity = max(0.0, dot(vertex.normal, light_dir))
(To jest właśnie prawo Lamberta – cosinus kąta między normalną a kierunkiem do źródła światła).
4. Pomnóż przez kolor światła i dodaj światło otoczenia (Ambient):
final_light_rgb = ambient_rgb + (light_color_rgb * intensity)
5. Na koniec zrób to, co masz teraz: pomnóż kolor tekstury (texel) przez final_light_rgb (pamiętając, że na PSX bazową jasnością tekstury było 0x80 / 128, czyli 1.0, więc wartości powyżej 128 pozwalały na tzw. overbright – rozjaśnianie tekstury ponad jej oryginalne kolory). */

uint8_t gte_calc_lambert(const int16_t *normal, const int16_t *light_dir,
                         uint8_t ambient, uint8_t intensity);

#endif
