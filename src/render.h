#pragma once

#include <GL/gl.h>
#include <GL/gl_integration.h>
#include "assets.h"

typedef struct{

    const GLfloat color[4];
    const GLfloat diffuse[4];

    const GLfloat position[4];
    const GLfloat direction[3];

    const float radius;

} light_t;

typedef struct {
    GLfloat m[4][4];
} Matrix;

extern Material gTempMaterials[];

void init_renderer(void);
void setup_light(light_t light);
void set_light(light_t light);
void setup_fog(light_t light);
void project_camera(void);
void render_sky(void);
void render_end();
void render_game(int updateRate, float updateRateF);

void render_bush(void);