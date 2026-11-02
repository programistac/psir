#ifndef SHADER_H
#define SHADER_H

#include <GL/glew.h>

/* ========================================================
	Shader PSX
   ======================================================== */

static const char *psx_vertex_shader_source =
	"#version 450 core\n"

        "layout(location = 0) in vec3 position;\n"
        "layout(location = 1) in vec4 color;\n"
		"layout(location = 2) in vec2 uv;\n"
        "layout(location = 3) in uint render_mode;\n"

        "out vec4 vertex_color;\n"
        "noperspective out vec2 vertex_uv;\n"
        "flat out uint vertex_render_mode;\n"

        "void main()\n"
        "{\n"
        "    gl_Position = vec4(position, 1.0);\n"

        "    vertex_color = color;\n"
        "    vertex_uv = uv;\n" /*tymczasowe*/
        "    vertex_render_mode = render_mode;\n"
        "}\n";


/*   Kwantyzacja koloru do 5 bitów (32 stopnie jasności, od 0 do 31)
     Dokładnie tak, jak wyświetlał to prawdziwy PSX! 
     Tylko dla RENDER_MODE = 0 */

static const char *psx_fragment_shader_source =
        "#version 450 core\n"

        "in vec4 vertex_color;\n"
        "noperspective in vec2 vertex_uv;\n"
        "flat in uint vertex_render_mode;\n"

	"layout(binding = 1) uniform usampler2D uIndexTex;\n"
	"layout(binding = 0) uniform sampler2D uPaletteTex;\n"
	"uniform int uPaletteID;\n"

        "out vec4 FragColor;\n"

        "void main()\n"
        "{\n"

	"	if (vertex_render_mode == 0u)\n"
        "	{\n"
	"		vec3 psx_rgb = floor(vertex_color.rgb * 31.0) / 31.0;\n"
        "		FragColor = vec4(psx_rgb, vertex_color.a);\n"
        "	}\n"
        "	else if (vertex_render_mode == 1u)\n"
        "	{\n"
        "		FragColor = vertex_color;\n"
        "	}\n"
        "	else if (vertex_render_mode == 2u)\n"
        "	{\n"
        "		vec2 uv = vertex_uv;\n"
	"\n"
	"		// Get palette index\n"
	"		uint index = texture(uIndexTex, uv).r;\n"
	"\n"
	"		// Lookup PSX RGB5A1 color\n"
	"		vec4 tex = texelFetch(\n"
	"			uPaletteTex,\n"
	"			ivec2(int(index), uPaletteID),0\n"
	"		);\n"
	"\n"
	"		FragColor = tex;\n"
        "	}\n"
        "	else if (vertex_render_mode == 3u)\n"
        "	{\n"
        "		FragColor = vertex_color;\n"
        "	}\n"
        "}\n";

unsigned int compile_shader(GLenum type, const char *src);
void shader_use_program(unsigned int program);

unsigned int create_psx_program(void);


#endif

/* TODO: load shader from file */
