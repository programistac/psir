#include "gpu.h"

#include <GL/glew.h>
#include <assert.h>
#include <stdio.h>

void gpu_destroy(struct gpu_data *data)
{
	glDeleteVertexArrays(VAO_NUM, data->vaos);
	glDeleteBuffers(BUFFER_NUM, data->buffers);
}

void gpu_create(struct gpu_data *data)
{
	assert(data != NULL);

	unsigned int i; /* for loop counter */

	data->gpu_primitive_count = 0;

/* ===========================================================
	VAO + VBO
   =========================================================== */

	glCreateVertexArrays(VAO_NUM, data->vaos);
	glCreateBuffers(BUFFER_NUM, data->buffers);

	for (i = 0; i < VAO_NUM; ++i)
		assert(data->vaos[i] != 0);

	for (i = 0; i < BUFFER_NUM; ++i)
		assert(data->buffers[i] != 0);

	glNamedBufferStorage(
		data->buffers[BUFFER_VERTEX],
		GPU_MAX_VERTICES * sizeof(struct gpu_vertex),
		NULL,
		GL_DYNAMIC_STORAGE_BIT
	);

/* ===========================================================
	Vertex Buffer Binding - DSA (Direct State Access)
   =========================================================== */
		/* DSA vs Bind to Edit */
		/* w ksiazce jest opisane Bind to Edit */
		/* DSA pozwala mi poinformowac opengl
		ktorym obiektem chce teraz manipulowac 
		bez zmiany jego globalnego stanu */


	glVertexArrayVertexBuffer(
		data->vaos[VAO_DATA],
		0,
		data->buffers[BUFFER_VERTEX],
		0,
		sizeof(struct gpu_vertex)
	); /* przypisz VERTEX_BUFFER do punktu wiazania 0 */

/* ===========================================================
	Position
   =========================================================== */

	glEnableVertexArrayAttrib(
		data->vaos[VAO_DATA],
		ATTRIB_POSITION
	); /* wlacz atrybut odpowiadajacy za pozycje */

	glVertexArrayAttribFormat(
		data->vaos[VAO_DATA],
		ATTRIB_POSITION,
		3,
		GL_SHORT,
		GL_TRUE,
		offsetof(struct gpu_vertex, x)
	); /* opisz format oraz przesuniecie danych w strukturze wierzcholkow */

	glVertexArrayAttribBinding(
		data->vaos[VAO_DATA],
		ATTRIB_POSITION,
		0
	); /* podlacz atrybut pozycji z punktem wiazania bufora 0 */

/* ===========================================================
	Color
   =========================================================== */

	glEnableVertexArrayAttrib(
		data->vaos[VAO_DATA],
		ATTRIB_COLOR
	);

	glVertexArrayAttribFormat(
		data->vaos[VAO_DATA],
		ATTRIB_COLOR,
		4,
		GL_UNSIGNED_BYTE,
		GL_TRUE,
		offsetof(struct gpu_vertex, r)
	);

	glVertexArrayAttribBinding(
		data->vaos[VAO_DATA],
		ATTRIB_COLOR,
		0
	);

/* ===========================================================
	TEXTURE
   =========================================================== */
	glEnableVertexArrayAttrib(
		data->vaos[VAO_DATA],
		ATTRIB_TEXTURE
	);

	glVertexArrayAttribFormat(
		data->vaos[VAO_DATA],
		ATTRIB_TEXTURE,
		2,
		GL_UNSIGNED_BYTE,
		GL_TRUE,
		offsetof(struct gpu_vertex, u)
	);

	glVertexArrayAttribBinding(
		data->vaos[VAO_DATA],
		ATTRIB_TEXTURE,
		0
	);

/* ===========================================================
	Render Mode
   =========================================================== */

	glEnableVertexArrayAttrib(
		data->vaos[VAO_DATA],
		ATTRIB_RENDER_MODE
	);

	glVertexArrayAttribIFormat(
		data->vaos[VAO_DATA],
		ATTRIB_RENDER_MODE,
		1,
		GL_UNSIGNED_INT,
		offsetof(struct gpu_vertex, render_mode)
	);

	glVertexArrayAttribBinding(
		data->vaos[VAO_DATA],
		ATTRIB_RENDER_MODE,
		0
	);
}

void gpu_receive_data(struct gpu_data *data)
{
	glNamedBufferSubData(
		data->buffers[BUFFER_VERTEX],
		0,
		data->gpu_primitive_count * sizeof(struct gpu_vertex),
		data->gpu_primitive_stream
	);
}

void gpu_reset_stream(struct gpu_data *data)
{
	data->gpu_primitive_count = 0;
}

void gpu_submit_triangle(
	struct gpu_data *gpu,
	struct gpu_vertex *a,
	struct gpu_vertex *b,
	struct gpu_vertex *c
)
{
	assert(gpu != NULL);
	assert(a != NULL);
	assert(b != NULL);
	assert(c != NULL);
	assert(gpu->gpu_primitive_count <= GPU_MAX_VERTICES);


	if (gpu->gpu_primitive_count + 3 > GPU_MAX_VERTICES) {
		fprintf(stderr, "GPU: primitive stream is full!\n");
		return;
	}

	gpu->gpu_primitive_stream[gpu->gpu_primitive_count++] = *a;
	gpu->gpu_primitive_stream[gpu->gpu_primitive_count++] = *b;
	gpu->gpu_primitive_stream[gpu->gpu_primitive_count++] = *c;
}
