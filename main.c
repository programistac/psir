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

#define INTERNAL_W 320
#define INTERNAL_H 240


int main(int argc,char *argv[])
{       
	struct platform_sdl platform = { 0 };
	struct gpu_data gpu = { 0 };
	struct renderer renderer = { 0 };
	struct gte_transform trojkat = { 0 };
	
	unsigned int program;
	struct texture_clut_palette_atlas texture_atlas;
	struct texture_clut_indexed_image texture_image;

	int frame_angle;
	
	/* Wierzcholki robocze */
	struct gpu_vertex a;
	struct gpu_vertex b;
	struct gpu_vertex c;
	
	/* =========================================
	   Wierzcholki lokalne szescianu
	   ========================================= */

	/* Przednia sciana: Z = -1 */
	struct gpu_vertex v0 = {
		/* x y z r g b a u v mode */
		-4096, -4096, -4096, 255, 0, 0, 255, 0, 0, GPU_RENDER_TEXTURED_FLAT
	};

	struct gpu_vertex v1 = {
		4096, -4096, -4096, 255, 0, 0, 255, 255, 0, GPU_RENDER_TEXTURED_FLAT
	};

	struct gpu_vertex v2 = {
		4096,  4096, -4096, 0, 0, 255, 255, 255, 255, GPU_RENDER_TEXTURED_FLAT
	};

	struct gpu_vertex v3 = {
		-4096,  4096, -4096, 255, 0, 0, 255, 0, 255, GPU_RENDER_TEXTURED_FLAT
	};


	/* Tylna sciana: Z = +1 */
	struct gpu_vertex v4 = {
		-4096, -4096,  4096, 0, 255, 0, 255, 255, 0, GPU_RENDER_TEXTURED_FLAT
	};

	struct gpu_vertex v5 = {
		4096, -4096,  4096, 0, 255, 0, 255, 0, 0, GPU_RENDER_TEXTURED_FLAT
	};

	struct gpu_vertex v6 = {
		4096,  4096,  4096, 0, 255, 0, 255, 0, 255, GPU_RENDER_TEXTURED_FLAT
	};

	struct gpu_vertex v7 = {
		-4096,  4096,  4096, 0, 0, 255, 255, 255, 255, GPU_RENDER_TEXTURED_FLAT
	};
	
	  /* Uciszenie kompilatora - informujemy, że świadomie ich nie używamy */
        (void)argc;
        (void)argv;

	platform_initialize(&platform, WIN_W, WIN_H, "PSX DEMO", 0, 25.0);

	SDL_ShowCursor(SDL_DISABLE);

	gpu_create(&gpu);
	program = create_psx_program();

	renderer_initialize(&renderer, INTERNAL_W, INTERNAL_H);


	/* =========================================
	   LOAD TEXTURE
	   ========================================= */
	texture_load_clut_png_extract_indices(&texture_image, "assets/clut.png");
	texture_load_clut_png_extract_palette(&texture_atlas, "assets/clut.png");
	texture_clut_palette_atlas_upload(&texture_atlas);
	texture_clut_indexed_image_upload(&texture_image);


	glEnable(GL_DEPTH_TEST);

	
	/* identity(trojkat.m); */
	/* rotate_x(trojkat.m, DEG_TO_RAD(45.0f));*/
	trojkat.v[0] = 0;/*4096;*/
	trojkat.v[1] = 0;/*-4096 - 2048 - 1024;*/
	trojkat.v[2] = 5*FIX_ONE;
	trojkat.is_identity = 0;



	math3d_initialize_sin_lut();
	frame_angle = 0;

	while(platform.running) {
	       /* float t;*/
	        int16_t s[9];
		int16_t r[9];
		int16_t m[9];
	
		platform_begin_frame(&platform);

		platform_poll_events(&platform);

		/*t = SDL_GetTicks()*0.001f;*/

		renderer_begin(&renderer);
		renderer_set_clear_color(&renderer, 0.05f, 0.05f, 0.1f, 1.0f);
		renderer_clear(&renderer, RENDERER_CLEAR_COLOR | RENDERER_CLEAR_DEPTH);

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
		

		scale(s, FIX_ONE, FIX_ONE, FIX_ONE);
		/* TODO: tabela z wartosciami sin/cos  */
		/* przed przepelnieniem chroni operacja bitowa AND*/
		frame_angle = (frame_angle +1) & LUT_MASK;
		rotate_y(r, frame_angle);

		multiply_mxm(m, s, r);

		memcpy(trojkat.m, m, sizeof(m));

		/* =========================================
		   PRZEDNIA SCIANA
		   ========================================= */

		/* Trojkat 1 */
		a = v0;
		b = v1;
		c = v2;

		if (gte_project_vertex_3d(&a.x, &a.y, &a.z, &a.x, &trojkat, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&b.x, &b.y, &b.z, &b.x, &trojkat, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&c.x, &c.y, &c.z, &c.x, &trojkat, INTERNAL_W, INTERNAL_H))
		{
			gpu_submit_triangle(&gpu, &a, &b, &c);
		}

		/* Trojkat 2 */
		a = v0;
		b = v2;
		c = v3;

		if (gte_project_vertex_3d(&a.x, &a.y, &a.z, &a.x, &trojkat, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&b.x, &b.y, &b.z, &b.x, &trojkat, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&c.x, &c.y, &c.z, &c.x, &trojkat, INTERNAL_W, INTERNAL_H))
		{
			gpu_submit_triangle(&gpu, &a, &b, &c);
		}


		/* =========================================
		   TYLNA SCIANA
		   ========================================= */

		/* Trojkat 3 */
		a = v5;
		b = v4;
		c = v7;

		if (gte_project_vertex_3d(&a.x, &a.y, &a.z, &a.x, &trojkat, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&b.x, &b.y, &b.z, &b.x, &trojkat, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&c.x, &c.y, &c.z, &c.x, &trojkat, INTERNAL_W, INTERNAL_H))
		{
			gpu_submit_triangle(&gpu, &a, &b, &c);
		}

		/* Trojkat 4 */
		a = v5;
		b = v7;
		c = v6;

		if (gte_project_vertex_3d(&a.x, &a.y, &a.z, &a.x, &trojkat, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&b.x, &b.y, &b.z, &b.x, &trojkat, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&c.x, &c.y, &c.z, &c.x, &trojkat, INTERNAL_W, INTERNAL_H))
		{
			gpu_submit_triangle(&gpu, &a, &b, &c);
		}


		/* =========================================
		   LEWA SCIANA
		   ========================================= */

		/* Trojkat 5 */
		a = v4;
		b = v0;
		c = v3;

		if (gte_project_vertex_3d(&a.x, &a.y, &a.z, &a.x, &trojkat, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&b.x, &b.y, &b.z, &b.x, &trojkat, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&c.x, &c.y, &c.z, &c.x, &trojkat, INTERNAL_W, INTERNAL_H))
		{
			gpu_submit_triangle(&gpu, &a, &b, &c);
		}

		/* Trojkat 6 */
		a = v4;
		b = v3;
		c = v7;

		if (gte_project_vertex_3d(&a.x, &a.y, &a.z, &a.x, &trojkat, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&b.x, &b.y, &b.z, &b.x, &trojkat, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&c.x, &c.y, &c.z, &c.x, &trojkat, INTERNAL_W, INTERNAL_H))
		{
			gpu_submit_triangle(&gpu, &a, &b, &c);
		}


		/* =========================================
		   PRAWA SCIANA
		   ========================================= */

		/* Trojkat 7 */
		a = v1;
		b = v5;
		c = v6;

		if (gte_project_vertex_3d(&a.x, &a.y, &a.z, &a.x, &trojkat, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&b.x, &b.y, &b.z, &b.x, &trojkat, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&c.x, &c.y, &c.z, &c.x, &trojkat, INTERNAL_W, INTERNAL_H))
		{
			gpu_submit_triangle(&gpu, &a, &b, &c);
		}

		/* Trojkat 8 */
		a = v1;
		b = v6;
		c = v2;

		if (gte_project_vertex_3d(&a.x, &a.y, &a.z, &a.x, &trojkat, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&b.x, &b.y, &b.z, &b.x, &trojkat, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&c.x, &c.y, &c.z, &c.x, &trojkat, INTERNAL_W, INTERNAL_H))
		{
			gpu_submit_triangle(&gpu, &a, &b, &c);
		}


		/* =========================================
		   GORNA SCIANA
		   ========================================= */

		/* Trojkat 9 */
		a = v3;
		b = v2;
		c = v6;

		if (gte_project_vertex_3d(&a.x, &a.y, &a.z, &a.x, &trojkat, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&b.x, &b.y, &b.z, &b.x, &trojkat, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&c.x, &c.y, &c.z, &c.x, &trojkat, INTERNAL_W, INTERNAL_H))
		{
			a.u = 0;
			a.v = 0;
			b.u = 255;
			b.v = 0;
			c.u = 255;
			c.v = 255;
			gpu_submit_triangle(&gpu, &a, &b, &c);
		}

		/* Trojkat 10 */
		a = v3;
		b = v6;
		c = v7;

		if (gte_project_vertex_3d(&a.x, &a.y, &a.z, &a.x, &trojkat, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&b.x, &b.y, &b.z, &b.x, &trojkat, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&c.x, &c.y, &c.z, &c.x, &trojkat, INTERNAL_W, INTERNAL_H))
		{
			a.u = 0;
			a.v = 0;
			b.u = 255;
			b.v = 255;
			c.u = 0;
			c.v = 255;
			gpu_submit_triangle(&gpu, &a, &b, &c);
		}


		/* =========================================
		   DOLNA SCIANA
		   ========================================= */

		/* Trojkat 11 */
		a = v4;
		b = v5;
		c = v1;

		if (gte_project_vertex_3d(&a.x, &a.y, &a.z, &a.x, &trojkat, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&b.x, &b.y, &b.z, &b.x, &trojkat, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&c.x, &c.y, &c.z, &c.x, &trojkat, INTERNAL_W, INTERNAL_H))
		{
			gpu_submit_triangle(&gpu, &a, &b, &c);
		}

		/* Trojkat 12 */
		a = v4;
		b = v1;
		c = v0;

		if (gte_project_vertex_3d(&a.x, &a.y, &a.z, &a.x, &trojkat, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&b.x, &b.y, &b.z, &b.x, &trojkat, INTERNAL_W, INTERNAL_H) &&
				gte_project_vertex_3d(&c.x, &c.y, &c.z, &c.x, &trojkat, INTERNAL_W, INTERNAL_H))
		{
			gpu_submit_triangle(&gpu, &a, &b, &c);
		}






		/* =======================================================
		   CPU -> GPU
		   ======================================================= */
		shader_use_program(program);

		texture_clut_palette_atlas_bind(&texture_atlas);
		texture_clut_indexed_image_bind(&texture_image);

		gpu_receive_data(&gpu);
		renderer_draw_frame(&gpu);
		gpu_reset_stream(&gpu);
		renderer_end(&renderer);

		/* SDL_GetWindowSize(platform.window, &screenW, &screenH); */

		glViewport(0, 0, platform.window_w, platform.window_h);

		renderer_set_clear_color(&renderer, 0.0f, 0.0f, 0.0f, 1.0f);
		renderer_clear(&renderer, RENDERER_CLEAR_COLOR);
		renderer_present(&renderer, platform.window_w, platform.window_h);

		platform_end_frame(&platform);
	}
	texture_clut_palette_atlas_destroy(&texture_atlas);
	texture_clut_indexed_image_destroy(&texture_image);
	glUseProgram(0);
	glDeleteProgram(program);
	gpu_destroy(&gpu);
	framebuffer_destroy(&renderer.fb);
	platform_shutdown(&platform);

	return 0;
}
