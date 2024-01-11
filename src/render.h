#pragma once

#include <GL/gl.h>
#include <GL/gl_integration.h>
#include "assets.h"

typedef struct {

    const GLfloat color[4];
    const GLfloat diffuse[4];

    const GLfloat position[4];
    const short direction[3];

    const float radius;

} light_t;

typedef struct {
    GLfloat m[4][4];
} Matrix;

typedef struct RenderNode {
    rspq_block_t *block;
    Matrix *matrix;
    Material *material;
    int flags;
    struct RenderNode *next;
    struct RenderNode *prev;
} RenderNode;

typedef struct RenderList {
    RenderNode *entryHead;
    struct RenderList *next;
} RenderList;

extern rspq_block_t *sRenderSkyBlock;

void init_renderer(void);
void render_game(int updateRate, float updateRateF);
