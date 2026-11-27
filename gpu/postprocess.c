#include "postprocess.h"
#include "shader.h"

#include "GL/glew.h"

#include <assert.h>
#include <stdio.h>

int postprocess_initialize(struct postprocess *pp)
{
    assert(pp != NULL);

    pp->program = 0;
    pp->vao = 0;

    pp->program = create_postprocess_program();

    if (pp->program == 0) {
        printf("Postprocess: nie udalo sie utworzyc programu.\n");
        return 0;
    }

    glGenVertexArrays(1, &pp->vao);

    if (pp->vao == 0) {
        printf("Postprocess: nie udalo sie utworzyc VAO.\n");

        glDeleteProgram(pp->program);
        pp->program = 0;

        return 0;
    }

    return 1;
}


void postprocess_draw(
    struct postprocess *pp,
    const struct framebuffer *fb,
    int x,
    int y,
    int width,
    int height)
{
    GLboolean depth_enabled;
    GLboolean blend_enabled;

    assert(pp != NULL);
    assert(fb != NULL);

    if (pp->program == 0 || pp->vao == 0)
        return;

    depth_enabled = glIsEnabled(GL_DEPTH_TEST);
    blend_enabled = glIsEnabled(GL_BLEND);

    /* Prezentacja nie wymaga testu glebokosci ani blendowania. */
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glViewport(x, y, width, height);

    shader_use_program(pp->program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fb->color);

    glBindVertexArray(pp->vao);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    shader_use_program(0);

    /* Przywracamy poprzedni stan. */
    if (depth_enabled)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    if (blend_enabled)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
}


void postprocess_destroy(struct postprocess *pp)
{
    if (pp == NULL)
        return;

    if (pp->vao != 0)
        glDeleteVertexArrays(1, &pp->vao);

    if (pp->program != 0)
        glDeleteProgram(pp->program);

    pp->vao = 0;
    pp->program = 0;
}
