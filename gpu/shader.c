#include "shader.h"
#include <stdio.h>

unsigned int compile_shader(GLenum type, const char* src)
{
	unsigned int s = glCreateShader(type);

	glShaderSource(s,1,&src,NULL);
	glCompileShader(s);

	int ok;
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

	return p;
}


void shader_use_program(unsigned int program)
{
	glUseProgram(program);
}


