#include <libdragon.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>

#include "textures.h"
#include "../include/global.h"

void setup_textures(GLuint textures[], sprite_t *sprites[], const char *texture_path[], int texture_number) {

    for (uint32_t i = 0; i < texture_number; i++){

        sprites[i] = sprite_load(texture_path[i]);
    }

    glGenTextures(texture_number, textures);

    GLenum min_filter = GL_LINEAR;

    for (uint32_t i = 0; i < texture_number; i++){
        int repeat;

        if (i == 2 || i == 3) {
            repeat = false;
        } else {
            repeat = REPEAT_INFINITE;
        }

        glBindTexture(GL_TEXTURE_2D, textures[i]);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);

        glSpriteTextureN64(GL_TEXTURE_2D, sprites[i], &(rdpq_texparms_t){.s.repeats = repeat, .t.repeats = repeat});
    }

}
