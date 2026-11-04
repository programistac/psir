#ifndef SHADER_H
#define SHADER_H

#include <GL/glew.h>

/* ========================================================
	Shader PSX
   ======================================================== */

extern const char *psx_vertex_shader_source;
extern const char *psx_fragment_shader_source;

unsigned int compile_shader(GLenum type, const char *src);
void shader_use_program(unsigned int program);

unsigned int create_psx_program(void);


#endif

/* TODO: load shader from file */
