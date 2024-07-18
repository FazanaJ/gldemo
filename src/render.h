#pragma once

#include <GL/gl.h>
#include <GL/gl_integration.h>
#include <t3d/t3d.h>
#include "../include/global.h"
#include "assets.h"
#include "object.h"

#define SEGMENT_MATRIX  0x01
#define SEGMENT_BONES   0x02
#define SEGMENT_VERTS   0x03

#define MODEL_LOAD(x) t3d_model_load(asset_dir(x, DFS_MODEL64))
#define MODEL_FREE(x) t3d_model_free(x)

enum DrawLayer {
    DRAW_NZB,   // No Z Buffer
    DRAW_OPA,   // Opaque geometry
    DRAW_DECAL, // Decal geometry, should come after opaque, but before semitransparent
    DRAW_XLU,   // Semitransparent geometry, do this last.

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

typedef struct RenderNode {
    rspq_block_t *block;
    T3DMat4FP *matrix;
    Material *material;
    unsigned int flags;
    color_t envColour;
    color_t primColour;
    T3DMat4FP *skel;
    struct ObjectModel *objMod;
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
extern int gPrevMaterialID;
extern int gPrevCombiner;
extern rspq_block_t *gParticleMaterialBlock;
extern unsigned int gSortPos[DRAW_TOTAL];
extern const short sLayerSizes[DRAW_TOTAL];
extern void *gSortHeap[DRAW_TOTAL];
extern int gSortRecord[DRAW_TOTAL];

void init_renderer(void);
void render_game(int updateRate, float updateRateF);
void matrix_ortho(void);
void clutter_matrix(Matrix *mtx, int matrixBehaviour, float *pos, unsigned short *angle, float *scale);
