#pragma once

#include <libdragon.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>

enum EnvironmentFlags {
    ENV_FOG = (1 << 0),
};

typedef struct{

    const GLfloat color[4];
    const GLfloat diffuse[4];

    const GLfloat position[4];
    const GLfloat direction[3];

    const float radius;

} light_t;

typedef struct Environment {
    GLfloat fogColour[4];
    GLfloat skyColourTop[4];
    GLfloat skyColourBottom[4];
    GLfloat fogNear;
    GLfloat fogFar;
    char flags;
} Environment;

extern Environment *gEnvironment;

void setup_light(light_t light);
void set_light(light_t light);
void setup_fog(light_t light);
void render_end();