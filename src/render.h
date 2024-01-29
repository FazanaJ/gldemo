#pragma once

#include <GL/gl.h>
#include <GL/gl_integration.h>
#include "../include/global.h"
#include "assets.h"
#include "object.h"

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
extern RenderSettings gRenderSettings;
extern int gPrevRenderFlags;
extern int gPrevTextureID;
extern int gPrevCombiner;
extern rspq_block_t *gParticleMaterialBlock;

void init_renderer(void);
void render_game(int updateRate, float updateRateF);
void matrix_ortho(void);
