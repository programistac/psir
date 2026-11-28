#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <GL/glew.h>

#include <stdio.h>

#include "platform/platform_sdl.h"
#include "math/math3d.h"
#include "gpu/gpu.h"
#include "gte/gte.h"
#include "gpu/shader.h"
#include "gpu/framebuffer.h"
#include "renderer/renderer.h"
#include "gpu/texture.h"

#define WIN_W 1280
#define WIN_H 720

#define INTERNAL_W 320 /* 512x256 w Tomb Raider3 */
#define INTERNAL_H 240 /* 240 = NTSC 256 = PAL*/


int main(int argc,char *argv[])
{       
	struct platform_sdl platform = { 0 };
	struct gpu_data gpu = { 0 };
	struct renderer renderer = { 0 };
	struct gte_transform trojkat = { 0 };
	struct gte_transform background_transform = { 0 };
	
	unsigned int program;
	struct texture_clut_palette_atlas palettes = {0};
	struct texture_clut_indexed_atlas textures = {0};
	struct texture_clut_indexed_image image = {0};
	unsigned int texture_width;
	unsigned int texture_height;

	int frame_angle;
	int rotation_velocity; /* kierunek i predkosc obrotu */
	
	/* Wierzcholki robocze */
	struct gpu_vertex a;
	struct gpu_vertex b;
	struct gpu_vertex c;

	/* Światło padające z kierunku: prawo (+X), góra (+Y), tył (+Z) */
	/* Długość wektora: sqrt(2365^2 + 2365^2 + 2365^2) = 4096 (1.0 w Q3.12) */
	/* Normalne uśrednione dla rogów tylnej ściany (Z = +4096) */
	int16_t n4[3] = { -2365, -2365, 2365 }; /* dla v4: (-X, -Y, +Z) */
	int16_t n5[3] = {  2365, -2365, 2365 }; /* dla v5: (+X, -Y, +Z) */
	int16_t n6[3] = {  2365,  2365, 2365 }; /* dla v6: (+X, +Y, +Z) */
	int16_t n7[3] = { -2365,  2365, 2365 }; /* dla v7: (-X, +Y, +Z) */

	/* 1. Przygotowanie światła (raz na obiekt przed trójkątami!) */
	int16_t world_light[3] = { 2365, 2365, 2365 }; /* directional light */ 
	int16_t to_light[3];
	int16_t light_pos[3] = {0, 0, 0};
	int16_t local_light[3];
	int16_t inv_rot[9];	

	/* Przygotowanie kamery */
	struct gte_transform cam_transform = { 0 };
	int16_t cam_yaw = 0;
	int16_t cam_pitch = 0;

	struct gte_transform view_transform  = { 0 }; /* wynikowa transformacja obiektu wzgledem kamery */

	
	/* =========================================
	   Wierzcholki lokalne kwadratu
	   ========================================= */

	/* Przednia sciana: Z = */
	struct gpu_vertex vb0 = {
		 /* x y z r g b a u v pid idi mode */ 
		-32768, -32768, 30000, 255, 0, 0, 255, 0, 0, 1, 1, GPU_RENDER_FLAT
	};

	struct gpu_vertex vb1 = {
		32767, -32768, 30000, 0, 255, 0, 255, 255, 0, 1, 1, GPU_RENDER_FLAT
	};

	struct gpu_vertex vb2 = {
		32767,  32767, 30000, 0, 0, 255, 255, 255, 255, 1, 1, GPU_RENDER_FLAT 
	};

	struct gpu_vertex vb3 = {
		-32768,  32767, 30000, 0, 0, 100, 255, 0, 255, 1, 1, GPU_RENDER_FLAT
	};

	
	
	/* =========================================
	   Wierzcholki lokalne szescianu
	   ========================================= */

	/* Przednia sciana: Z = -1 */
	struct gpu_vertex v0 = {
		/* x y z r g b a u v pid iid mode */
		-4096, -4096, -4096, 128, 128, 128, 255, 0, 0, 1, 1, GPU_RENDER_TEXTURED_GOURAUD
	};

	struct gpu_vertex v1 = {
		4096, -4096, -4096, 128, 128, 128, 255, 255, 0, 1, 1, GPU_RENDER_TEXTURED_GOURAUD
	};

	struct gpu_vertex v2 = {
		4096,  4096, -4096, 128, 128, 128, 255, 255, 255, 1, 1, GPU_RENDER_TEXTURED_GOURAUD
	};

	struct gpu_vertex v3 = {
		-4096,  4096, -4096, 128, 128, 128, 255, 0, 255, 1, 1, GPU_RENDER_TEXTURED_GOURAUD
	};


	/* Tylna sciana: Z = +1 */
	struct gpu_vertex v4 = {
		-4096, -4096,  4096, 128, 128, 128, 255, 255, 0, 0, 0, GPU_RENDER_TEXTURED_GOURAUD
	};

	struct gpu_vertex v5 = {
		4096, -4096,  4096, 128, 128, 128, 255, 0, 0, 0, 0, GPU_RENDER_TEXTURED_GOURAUD
	};

	struct gpu_vertex v6 = {
		4096,  4096,  4096, 128, 128, 128, 255, 0, 255, 0, 0, GPU_RENDER_TEXTURED_GOURAUD
	};

	struct gpu_vertex v7 = {
		-4096,  4096,  4096, 128, 128, 128, 255, 255, 255, 0, 0, GPU_RENDER_TEXTURED_GOURAUD
	};
	
	  /* Uciszenie kompilatora - informuje, że świadomie ich nie używamy */
        (void)argc;
        (void)argv;

	platform_initialize(&platform, WIN_W, WIN_H, "PSX DEMO", 0, 25.0);

	SDL_ShowCursor(SDL_DISABLE);

	gpu_create(&gpu);
	program = create_psx_program();

	renderer_initialize(&renderer, INTERNAL_W, INTERNAL_H);
	renderer_log_info(&renderer);

	/* =========================================
	   LOAD TEXTURE
	   ========================================= */
	/* =========================================
	   Load palette and indexed image 0
	   ========================================= */

	texture_load_clut_png_extract_palette(
		&palettes,
		"assets/clut.png"
	);

	texture_load_clut_png_extract_indices(
		&image,
		"assets/clut.png"
	);

	/* All indexed textures must have the same dimensions. */
	texture_width = image.width;
	texture_height = image.height;

    /* -------------------------------------------------
       Create texture array.
       Two layers = two indexed textures.
       ------------------------------------------------- */

	texture_clut_indexed_atlas_create(
		&textures,
		texture_width,
		texture_height,
		2
	);

	texture_clut_indexed_atlas_upload_layer(
		&textures,
		0,
		&image
	);

	texture_clut_indexed_image_destroy(&image);

	/* -------------------------------------------------
	Load palette and indexed image 1
	------------------------------------------------- */

	texture_load_clut_png_extract_palette(
		&palettes,
		"assets/clut2.png"
	);

	texture_load_clut_png_extract_indices(
		&image,
		"assets/clut2.png"
	);

	texture_clut_indexed_atlas_upload_layer(
		&textures,
		1,
		&image
	);

	texture_clut_indexed_image_destroy(&image);

    /* -------------------------------------------------
       Upload palette atlas to GPU.
       ------------------------------------------------- */

	texture_clut_palette_atlas_upload(&palettes);


	/* zaimplementowane wlasne sortowanie */
	/* glEnable(GL_DEPTH_TEST); */
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	
	/* identity(trojkat.m); */
	trojkat.v[0] = 0;
	trojkat.v[1] = 0;
	trojkat.v[2] = 5*FIX_ONE;
	trojkat.is_identity = 0;


	identity(background_transform.m);
	background_transform.v[0] = 0;
	background_transform.v[1] = 0;
	background_transform.v[2] = 0;
	background_transform.is_identity = 1;



	math3d_initialize_sin_lut();
	frame_angle = 0;
	rotation_velocity = -2;

	/* Inicializacja kamery */
	identity(cam_transform.m);
	cam_transform.v[0] = 0;
	cam_transform.v[1] = 0;
	cam_transform.v[2] = 0;
	cam_transform.is_identity = 0;

	while(platform.running) {
	        int16_t s[9];
		int16_t r[9];
		int16_t m[9];

		/* kamera */
		int16_t cam_rx[9], cam_ry[9], cam_inv_rot[9];
		int16_t rel_pos[3];
	
		platform_begin_frame(&platform);

		platform_poll_events(&platform);
		/* Obsluga wejscia oparta na stanie klatki 
		 * (bez bezposredniego dotykania SDL_Event) 
		 */
		if (platform_key_pressed(&platform, SDL_SCANCODE_F1)) {
			renderer.postprocess_enabled = 
				!renderer.postprocess_enabled;
		}

		/*if (platform_key_pressed(&platform, SDL_SCANCODE_W)) {
			trojkat.v[2] += 1024;
		}

		if (platform_key_pressed(&platform, SDL_SCANCODE_S)) {
			trojkat.v[2] -= 1024;
		}*/

		/* --- 1. Sterowanie kamera (obrot strzalkami, ruch WSAD) --- */
if (platform_key_down(&platform, SDL_SCANCODE_LEFT))  cam_yaw   = (cam_yaw - 4) & LUT_MASK;
if (platform_key_down(&platform, SDL_SCANCODE_RIGHT)) cam_yaw   = (cam_yaw + 4) & LUT_MASK;
if (platform_key_down(&platform, SDL_SCANCODE_UP))    cam_pitch = (cam_pitch - 4) & LUT_MASK;
if (platform_key_down(&platform, SDL_SCANCODE_DOWN))  cam_pitch = (cam_pitch + 4) & LUT_MASK;

if (platform_key_down(&platform, SDL_SCANCODE_W)) cam_transform.v[2] += 256;
if (platform_key_down(&platform, SDL_SCANCODE_S)) cam_transform.v[2] -= 256;
if (platform_key_down(&platform, SDL_SCANCODE_A)) cam_transform.v[0] -= 256;
if (platform_key_down(&platform, SDL_SCANCODE_D)) cam_transform.v[0] += 256;

		renderer_begin(&renderer);
		renderer_set_clear_color(&renderer, 0.05f, 0.05f, 0.1f, 1.0f);
		renderer_clear(&renderer, RENDERER_CLEAR_COLOR | 
				RENDERER_CLEAR_DEPTH);

		/* =======================================================
		   RENDER
		   ======================================================= */

		/*scale(s, sx, sy, sz);
		  rotate_x(rx, angle_x);
		  rotate_y(ry, angle_y);
		  rotate_z(rz, angle_z);

		  Skala -> rotacja X -> rotacja Y -> rotacja Z.
		  multiply_mxm(temp, s, rx);
		  multiply_mxm(m, temp, ry);
		  multiply_mxm(temp, m, rz);*/

		/* rotate_y(trojkat.m, DEG_TO_RAD(t * 16)); */
		

		/* Budujemy macierz rotacji kamery oraz jej odwrotnosc (transpozycje) */
		rotate_x(cam_rx, cam_pitch);
		rotate_y(cam_ry, cam_yaw);
		multiply_mxm(cam_transform.m, cam_ry, cam_rx);
		transpose_m(cam_inv_rot, cam_transform.m);

		/* Budujemy macierz rotacji obiektu*/
		scale(s, FIX_ONE, FIX_ONE, FIX_ONE);
		/* TODO: tabela z wartosciami sin/cos  */
		/* przed przepelnieniem chroni operacja bitowa AND*/
		frame_angle = (frame_angle + rotation_velocity) & LUT_MASK;
		
		rotate_y(r, frame_angle);

		multiply_mxm(m, s, r);

		memcpy(trojkat.m, m, sizeof(m));

		/* wylicz kierunek swiatla */
		sub_v3(to_light, light_pos, trojkat.v);
		normalize_v3(world_light, to_light); 

		/* Pobieramy odwrotność rotacji obiektu i przenosimy światło do przestrzeni lokalnej */
		transpose_m(inv_rot, trojkat.m);
	
		/* -------------------------------------------------------------------------- */
		/* ZASADA PRZENOSZENIA SWIATLA DO PRZESTRZENI LOKALNEJ OBIEKTU:               */
		/*                                                                            */
		/* 1. Tryb Per-Object (szybki, styl PSX GTE - wspolny kierunek dla bryly):    */
		/*    Normalizujemy wektor OD RAZU (dlugosc = 4096) przed multiply_vxm(),     */
		/*    poniewaz przenosimy sam KIERUNEK padania promieni.                      */
		/*                                                                            */
		/* 2. Tryb Per-Vertex (dokladny Point Light / latarka):                       */
		/*    Przenosimy NIEZNORMALIZOWANY wektor (light_pos - obj_pos), aby          */
		/*    zachowac prawdziwa POZYCJE i odleglosc zrodla swiatla. Normalizujemy    */
		/*    dopiero dla kazdego wierzcholka po odjeciu jego pozycji (local - v).    */
		/* -------------------------------------------------------------------------- */
		multiply_vxm(local_light, world_light, inv_rot);


		/* --- 4. Zlozenie obiektu z kamera do wspolnego cam_transform --- */
		multiply_mxm(view_transform.m, trojkat.m, cam_inv_rot);
		sub_v3(rel_pos, trojkat.v, cam_transform.v);
		multiply_vxm(view_transform.v, rel_pos, cam_inv_rot);
		cam_transform.is_identity = 0;

		
		/* =============================================
		   Pre-Renderowane Tlo.				
		   ============================================= */
		a = vb0;
		b = vb1;
		c = vb2;

		if (gte_project_fullscreen(&a.x, &a.y, &a.x, &background_transform) &&
				gte_project_fullscreen(&b.x, &b.y, &b.x, &background_transform) &&
				gte_project_fullscreen(&c.x, &c.y, &c.x, &background_transform))
		{
			gpu_submit_triangle(&gpu, &a, &b, &c);
		}

		/* Trojkat 2 */ 
		a = vb0;
		b = vb2;
		c = vb3;

		if (gte_project_fullscreen(&a.x, &a.y, &a.x, &background_transform) &&
				gte_project_fullscreen(&b.x, &b.y, &b.x, &background_transform) &&
				gte_project_fullscreen(&c.x, &c.y, &c.x, &background_transform))
		{
			gpu_submit_triangle(&gpu, &a, &b, &c);
		}

		/* =========================================
		   PRZEDNIA SCIANA
		   ========================================= */

		/* Trojkat 1 */
		a = v0;
		b = v1;
		c = v2;

		if (gte_project_vertex_3d(&a.x, &a.y, &a.z, &a.x, &view_transform, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&b.x, &b.y, &b.z, &b.x, &view_transform, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&c.x, &c.y, &c.z, &c.x, &view_transform, INTERNAL_W, INTERNAL_H))
		{
			gte_apply_vertex_light(&a, 0);
			gte_apply_vertex_light(&b, 0);
			gte_apply_vertex_light(&c, 200);


			gpu_submit_triangle(&gpu, &a, &b, &c);
		}

		/* Trojkat 2 */
		a = v0;
		b = v2;
		c = v3;

		if (gte_project_vertex_3d(&a.x, &a.y, &a.z, &a.x, &view_transform, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&b.x, &b.y, &b.z, &b.x, &view_transform, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&c.x, &c.y, &c.z, &c.x, &view_transform, INTERNAL_W, INTERNAL_H))
		{
			gte_apply_vertex_light(&a, 0);
			gte_apply_vertex_light(&b, 200);
			gte_apply_vertex_light(&c, 0);

			gpu_submit_triangle(&gpu, &a, &b, &c);
		}


		/* =========================================
		   TYLNA SCIANA
		   ========================================= */

		/* Trojkat 3 */
		a = v5;
		b = v4;
		c = v7;

		if (gte_project_vertex_3d(&a.x, &a.y, &a.z, &a.x, &view_transform, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&b.x, &b.y, &b.z, &b.x, &view_transform, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&c.x, &c.y, &c.z, &c.x, &view_transform, INTERNAL_W, INTERNAL_H))
		{
		        gte_apply_vertex_light(&a, gte_calc_lambert(n5, local_light, 20, 255));
	gte_apply_vertex_light(&b, gte_calc_lambert(n4, local_light, 20, 255));
	gte_apply_vertex_light(&c, gte_calc_lambert(n7, local_light, 20, 255));
		
			gpu_submit_triangle(&gpu, &a, &b, &c);
		}

		/* Trojkat 4 */
		a = v5;
		b = v7;
		c = v6;

		if (gte_project_vertex_3d(&a.x, &a.y, &a.z, &a.x, &view_transform, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&b.x, &b.y, &b.z, &b.x, &view_transform, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&c.x, &c.y, &c.z, &c.x, &view_transform, INTERNAL_W, INTERNAL_H))
		{
			gte_apply_vertex_light(&a, gte_calc_lambert(n5, local_light, 20, 255));
	gte_apply_vertex_light(&b, gte_calc_lambert(n7, local_light, 20, 255));
	gte_apply_vertex_light(&c, gte_calc_lambert(n6, local_light, 20, 255));
			gpu_submit_triangle(&gpu, &a, &b, &c);
		}

		/* =========================================
		   LEWA SCIANA
		   ========================================= */

		/* Trojkat 5 */
		a = v4;
		b = v0;
		c = v3;

		if (gte_project_vertex_3d(&a.x, &a.y, &a.z, &a.x, &view_transform, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&b.x, &b.y, &b.z, &b.x, &view_transform, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&c.x, &c.y, &c.z, &c.x, &view_transform, INTERNAL_W, INTERNAL_H))
		{
			gte_apply_vertex_light(&a, 128);
			gte_apply_vertex_light(&b, 128);
			gte_apply_vertex_light(&c, 128);
			gpu_submit_triangle(&gpu, &a, &b, &c);
		}

		/* Trojkat 6 */
		a = v4;
		b = v3;
		c = v7;

		if (gte_project_vertex_3d(&a.x, &a.y, &a.z, &a.x, &view_transform, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&b.x, &b.y, &b.z, &b.x, &view_transform, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&c.x, &c.y, &c.z, &c.x, &view_transform, INTERNAL_W, INTERNAL_H))
		{
			gte_apply_vertex_light(&a, 128);
			gte_apply_vertex_light(&b, 128);
			gte_apply_vertex_light(&c, 128);
			gpu_submit_triangle(&gpu, &a, &b, &c);
		}


		/* =========================================
		   PRAWA SCIANA
		   ========================================= */

		/* Trojkat 7 */
		a = v1;
		b = v5;
		c = v6;

		if (gte_project_vertex_3d(&a.x, &a.y, &a.z, &a.x, &view_transform, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&b.x, &b.y, &b.z, &b.x, &view_transform, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&c.x, &c.y, &c.z, &c.x, &view_transform, INTERNAL_W, INTERNAL_H))
		{
			gte_apply_vertex_light(&a, 128);
			gte_apply_vertex_light(&b, 128);
			gte_apply_vertex_light(&c, 128);
			gpu_submit_triangle(&gpu, &a, &b, &c);
		}

		/* Trojkat 8 */
		a = v1;
		b = v6;
		c = v2;

		if (gte_project_vertex_3d(&a.x, &a.y, &a.z, &a.x, &view_transform, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&b.x, &b.y, &b.z, &b.x, &view_transform, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&c.x, &c.y, &c.z, &c.x, &view_transform, INTERNAL_W, INTERNAL_H))
		{
			gte_apply_vertex_light(&a, 128);
			gte_apply_vertex_light(&b, 128);
			gte_apply_vertex_light(&c, 128);
			gpu_submit_triangle(&gpu, &a, &b, &c);
		}


		/* =========================================
		   GORNA SCIANA
		   ========================================= */

		/* Trojkat 9 */
		a = v3;
		b = v2;
		c = v6;

		if (gte_project_vertex_3d(&a.x, &a.y, &a.z, &a.x, &view_transform, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&b.x, &b.y, &b.z, &b.x, &view_transform, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&c.x, &c.y, &c.z, &c.x, &view_transform, INTERNAL_W, INTERNAL_H))
		{
			a.u = 0;
			a.v = 0;
			b.u = 255;
			b.v = 0;
			c.u = 255;
			c.v = 255;
			
			gte_apply_vertex_light(&a, 128);
			gte_apply_vertex_light(&b, 128);
			gte_apply_vertex_light(&c, 128);
			gpu_submit_triangle(&gpu, &a, &b, &c);
		}

		/* Trojkat 10 */
		a = v3;
		b = v6;
		c = v7;

		if (gte_project_vertex_3d(&a.x, &a.y, &a.z, &a.x, &view_transform, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&b.x, &b.y, &b.z, &b.x, &view_transform, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&c.x, &c.y, &c.z, &c.x, &view_transform, INTERNAL_W, INTERNAL_H))
		{
			a.u = 0;
			a.v = 0;
			b.u = 255;
			b.v = 255;
			c.u = 0;
			c.v = 255;
			
			gte_apply_vertex_light(&a, 128);
			gte_apply_vertex_light(&b, 128);
			gte_apply_vertex_light(&c, 128);
			gpu_submit_triangle(&gpu, &a, &b, &c);
		}


		/* =========================================
		   DOLNA SCIANA
		   ========================================= */

		/* Trojkat 11 */
		a = v4;
		b = v5;
		c = v1;

		if (gte_project_vertex_3d(&a.x, &a.y, &a.z, &a.x, &view_transform, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&b.x, &b.y, &b.z, &b.x, &view_transform, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&c.x, &c.y, &c.z, &c.x, &view_transform, INTERNAL_W, INTERNAL_H))
		{
			gte_apply_vertex_light(&a, 128);
			gte_apply_vertex_light(&b, 128);
			gte_apply_vertex_light(&c, 128);
			gpu_submit_triangle(&gpu, &a, &b, &c);
		}

		/* Trojkat 12 */
		a = v4;
		b = v1;
		c = v0;

		if (gte_project_vertex_3d(&a.x, &a.y, &a.z, &a.x, &view_transform, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&b.x, &b.y, &b.z, &b.x, &view_transform, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&c.x, &c.y, &c.z, &c.x, &view_transform, INTERNAL_W, INTERNAL_H))
		{
			gte_apply_vertex_light(&a, 128);
			gte_apply_vertex_light(&b, 128);
			gte_apply_vertex_light(&c, 128);
			gpu_submit_triangle(&gpu, &a, &b, &c);
		}
		
		
		/* Bind Textures and Palettes*/
		texture_clut_palette_atlas_bind(&palettes);
		texture_clut_indexed_atlas_bind(&textures);


		/* =======================================================
		   CPU -> GPU
		   ======================================================= */
		shader_use_program(program);

		gpu_sort_triangles(&gpu);
		gpu_receive_data(&gpu);
		renderer_draw_frame(&gpu);
		gpu_reset_stream(&gpu);
		renderer_end(&renderer);

		/* SDL_GetWindowSize(platform.window, &screenW, &screenH); */
		/* TODO: przeniesc Viewport...*/
		glViewport(0, 0, platform.window_w, platform.window_h);

		renderer_set_clear_color(&renderer, 0.0f, 0.0f, 0.0f, 1.0f);
		renderer_clear(&renderer, RENDERER_CLEAR_COLOR);
		renderer_present(&renderer, platform.window_w, platform.window_h);

		platform_end_frame(&platform);
	}

	texture_clut_palette_atlas_destroy(&palettes);
	texture_clut_indexed_atlas_destroy(&textures);
	glUseProgram(0);
	glDeleteProgram(program);
	gpu_destroy(&gpu);
	renderer_destroy(&renderer);
	platform_shutdown(&platform);

	return 0;
}
