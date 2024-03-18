#pragma once

#include <GL/gl.h>
#include <GL/gl_integration.h>
#include <t3d/t3d.h>
#include "../include/global.h"
#include "assets.h"
#include "object.h"

#if OPENGL
#define MATRIX_PUSH() glPushMatrix()
#define MATRIX_POP() glPopMatrix()
#define MATRIX_MUL(x, stackPos, stackPrev) glMultMatrixf((GLfloat *) x)
#define MODEL_LOAD(x) model64_load(asset_dir(x, DFS_MODEL64))
#define MODEL_FREE(x) model64_free(x)
#elif TINY3D
#define MATRIX_PUSH()
#define MATRIX_POP()
//#define MATRIX_MUL(x, stackPos, stackPrev) t3d_matrix_set_mul((T3DMat4FP *) x, stackPos, stackPrev)
#define MATRIX_MUL(x, stackPos, stackPrev)
#define MODEL_LOAD(x) t3d_model_load(asset_dir(x, DFS_MODEL64))
#define MODEL_FREE(x) t3d_model_free(x)
#else
#define MATRIX_PUSH()
#define MATRIX_POP()
#define MATRIX_MUL(x, stackPos, stackPrev)
#define MODEL_LOAD(x) 0
#define MODEL_FREE(x)
#endif

enum DrawLayer {
    DRAW_OPA,
    DRAW_DECAL,
    DRAW_XLU,

    DRAW_TOTAL
};

enum ChunkFlags {
    CHUNK_HAS_MODEL = (1 << 0),
};

typedef struct {
    const GLfloat colour[4];
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
extern unsigned int gSortPos[DRAW_TOTAL];
extern const short sLayerSizes[DRAW_TOTAL];
extern void *gSortHeap[DRAW_TOTAL];
extern int gSortRecord[DRAW_TOTAL];

void init_renderer(void);
void render_game(int updateRate, float updateRateF);
void matrix_ortho(void);
