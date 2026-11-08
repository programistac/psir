#include "shader.h"
#include <stdio.h>



const char *psx_vertex_shader_source =
    "#version 450 core\n"

    "layout(location = 0) in vec3 position;\n"
    "layout(location = 1) in vec4 color;\n"
    "layout(location = 2) in vec2 uv;\n"
    "layout(location = 3) in uint render_mode;\n"
    "layout(location = 4) in uint texture_layer;\n"
    "layout(location = 5) in uint palette_id;\n"

    "out vec4 vertex_color;\n"
    "noperspective out vec2 vertex_uv;\n"
    "flat out uint vertex_render_mode;\n"
    "flat out uint vertex_texture_layer;\n"
    "flat out uint vertex_palette_id;\n"

    "void main()\n"
    "{\n"
    "    gl_Position = vec4(position, 1.0);\n"

    "    vertex_color = color;\n"
    "    vertex_uv = uv;\n"
    "    vertex_render_mode = render_mode;\n"
    "    vertex_texture_layer = texture_layer;\n"
    "    vertex_palette_id = palette_id;\n"
    "}\n";



/*   Kwantyzacja koloru do 5 bitów (32 stopnie jasności, od 0 do 31)
     Dokładnie tak, jak wyświetlał to prawdziwy PSX! 
     Tylko dla RENDER_MODE = 0 */


const char *psx_fragment_shader_source =
    "#version 450 core\n"

    "in vec4 vertex_color;\n"
    "noperspective in vec2 vertex_uv;\n"
    "flat in uint vertex_render_mode;\n"
    "flat in uint vertex_texture_layer;\n"
    "flat in uint vertex_palette_id;\n"

    "layout(binding = 1) uniform usampler2DArray uIndexTex;\n"
    "layout(binding = 0) uniform sampler2D uPaletteTex;\n"

    "out vec4 FragColor;\n"

    "void main()\n"
    "{\n"

    "    if (vertex_render_mode == 0u)\n"
    "    {\n"
    "        vec3 psx_rgb = floor(vertex_color.rgb * 31.0) / 31.0;\n"
    "        FragColor = vec4(psx_rgb, vertex_color.a);\n"
    "    }\n"
    "    else if (vertex_render_mode == 1u)\n"
    "    {\n"
    "        FragColor = vertex_color;\n"
    "    }\n"
    "    else if (vertex_render_mode == 2u)\n"
    "    {\n"
    "        ivec2 tex_size = textureSize(uIndexTex, 0).xy;\n"

    "        ivec2 texel = ivec2(vertex_uv * vec2(tex_size));\n"
    "        texel = clamp(texel, ivec2(0), tex_size - ivec2(1));\n"

    "        uint index = texelFetch(\n"
    "            uIndexTex,\n"
    "            ivec3(texel, int(vertex_texture_layer)),\n"
    "            0\n"
    "        ).r;\n"

    "        FragColor = texelFetch(\n"
    "            uPaletteTex,\n"
    "            ivec2(int(index), int(vertex_palette_id)),\n"
    "            0\n"
    "        );\n"
    "    }\n"
    "    else if (vertex_render_mode == 3u)\n"
    "    {\n"
    "        FragColor = vertex_color;\n"
    "    }\n"
    "}\n";


unsigned int compile_shader(GLenum type, const char* src)
{
	unsigned int s = glCreateShader(type);
	int ok;

	glShaderSource(s,1,&src,NULL);
	glCompileShader(s);

	glGetShaderiv(s,GL_COMPILE_STATUS,&ok);

	if(!ok) {
		char log[1024];
		glGetShaderInfoLog(s,sizeof(log),NULL,log);
		printf("Shader Error:\n%s\n",log);
	}	

	return s;
}

unsigned int create_psx_program(void)
{
	unsigned int vs = compile_shader(GL_VERTEX_SHADER,
					psx_vertex_shader_source);
	unsigned int fs = compile_shader(GL_FRAGMENT_SHADER, 
					psx_fragment_shader_source);

	unsigned int p = glCreateProgram();

	glAttachShader(p,vs);
	glAttachShader(p,fs);

	glLinkProgram(p);

	glDetachShader(p, vs);
	glDetachShader(p, fs);

	glDeleteShader(vs);
	glDeleteShader(fs);

	return p;
}


void shader_use_program(unsigned int program)
{
	glUseProgram(program);
}


