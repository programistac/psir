#include "framebuffer.h"

#include "GL/glew.h"

void framebuffer_create(struct framebuffer *fb, int width, int height)
{
        fb->width = width;
        fb->height = height;


        glGenFramebuffers(1, &fb->fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo);

        glGenTextures(1, &fb->color);
        glBindTexture(GL_TEXTURE_2D, fb->color);

        glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RGB,
                fb->width,
                fb->height,
                0,
                GL_RGB,
                GL_UNSIGNED_BYTE,
                NULL
        );

        glTexParameteri(
                GL_TEXTURE_2D,
                GL_TEXTURE_MIN_FILTER,
                GL_NEAREST
        );

        glTexParameteri(
                GL_TEXTURE_2D,
                GL_TEXTURE_MAG_FILTER,
                GL_NEAREST
        );

        glFramebufferTexture2D(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D,
                fb->color,
                0
        );

        glGenRenderbuffers(1,&fb->depth);

        glBindRenderbuffer(GL_RENDERBUFFER, fb->depth);

        glRenderbufferStorage(
                GL_RENDERBUFFER,
                GL_DEPTH_COMPONENT24,
                fb->width,
                fb->height
        );

        glFramebufferRenderbuffer(
                GL_FRAMEBUFFER,
                GL_DEPTH_ATTACHMENT,
                GL_RENDERBUFFER,
                fb->depth
        );

        glBindFramebuffer(GL_FRAMEBUFFER,0);
}

void framebuffer_bind(struct framebuffer *fb)
{
        glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo);
}

void framebuffer_unbind(void)
{
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void framebuffer_destroy(struct framebuffer *fb)
{
        glDeleteFramebuffers(1, &fb->fbo);

        glDeleteTextures(1, &fb->color);

        glDeleteRenderbuffers(1, &fb->depth);

        fb->fbo = 0;
        fb->color = 0;
        fb->depth = 0;

        fb->width = 0;
        fb->height = 0;
}

/* wyswietl low-resolution framebuffer na ekran */
void framebuffer_blit_to_screen(
                struct framebuffer *fb, 
                int dst_x, 
                int dst_y, 
                int dst_w, 
                int dst_h)
{
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fb->fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        glBlitFramebuffer(
                0,
                0,
                fb->width,
                fb->height,

                dst_x,
                dst_y,
                dst_x + dst_w,
                dst_y + dst_h,

                GL_COLOR_BUFFER_BIT,
                GL_NEAREST
        );
}
