#include <libdragon.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>
#include <malloc.h>
#include <t3d/t3d.h>

#include "render.h"
#include "../include/global.h"

#include "main.h"
#include "camera.h"
#include "debug.h"
#include "assets.h"
#include "math_util.h"
#include "hud.h"
#include "menu.h"
#include "object.h"
#include "scene.h"
#include "input.h"
#include "talk.h"
#include "screenshot.h"

float gAspectRatio = 1.0f;
RenderNode *gRenderNodeHead[DRAW_TOTAL];
RenderNode *gRenderNodeTail[DRAW_TOTAL];
RenderList *gMateriallistHead[DRAW_TOTAL];
RenderList *gMateriallistTail[DRAW_TOTAL];
RenderList *gPrevMatList = NULL;
Matrix gBillboardMatrix;
Matrix gScaleMatrix;
Matrix gViewMatrix;
char gZTargetTimer = 0;
char gUseOverrideMaterial = false;
char gMatrixStackPos;
static rspq_block_t *sRenderEndBlock;
rspq_block_t *sRenderSkyBlock;
static rspq_block_t *sBeginModeBlock;
static rspq_block_t *sParticleBlock;
rspq_block_t *sBushBlock;
rspq_block_t *sShadowBlock;
Material gOverrideMaterial;
Material gBlankMaterial;
Material *gBlobShadowMat;
Material *gOutlineMat;
void *gSortHeap[DRAW_TOTAL];
unsigned int gSortPos[DRAW_TOTAL];
float gHalfFovHor;
float gHalfFovVert;
RenderSettings gRenderSettings;
rspq_block_t *gParticleMaterialBlock;
int gPrevRenderFlags;
int gPrevMaterialID;
#ifdef PUPPYPRINT_DEBUG
int gSortRecord[DRAW_TOTAL];
#endif

Material *gTempMaterials[2];

light_t lightNeutral = {
    colour: {0.66f, 0.66f, 0.66f, 0.66f},
    diffuse: {1.0f, 1.0f, 1.0f, 1.0f},
    direction: {0, 0, 0x6000},
    position: {1.0f, 0.0f, 0.0f, 0.0f},
    radius: 10.0f,
};

const short sLayerSizes[DRAW_TOTAL] = {
    0x3800, // Standard
    0x400, // Decal
    0x800, // Semitransparent
};


static void init_particles(void) {
    rspq_block_begin();
#if OPENGL
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex3f(-5, 5, 0);
    glTexCoord2f(0, 1.024f);
    glVertex3f(-5, -5, 0);
    glTexCoord2f(1.024f, 1.024f);
    glVertex3f(5, -5, 0);
    glTexCoord2f(1.024f, 0);
    glVertex3f(5, 5, 0);
    glEnd();
#endif
    sParticleBlock = rspq_block_end();
}

static inline void setup_light(light_t light) {
#if OPENGL
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, light.colour);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light.diffuse);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 2.0f/light.radius);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 1.0f/(light.radius*light.radius));

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, light.diffuse);
#endif
}

void init_renderer(void) {
    setup_light(lightNeutral);
    init_materials();
    init_particles();

    rspq_block_begin();
#if OPENGL
    glAlphaFunc(GL_GREATER, 0.5f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_LESS);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glShadeModel(GL_SMOOTH);
    glDepthMask(GL_TRUE);
    rdpq_mode_filter(FILTER_BILINEAR);
    rdpq_mode_dithering(DITHER_SQUARE_SQUARE);
    glDitherModeN64(DITHER_SQUARE_SQUARE);
#endif
    sBeginModeBlock = rspq_block_end();

    rspq_block_begin();
#if OPENGL
    glDisable(GL_MULTISAMPLE_ARB);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    glDepthMask(GL_FALSE);
#endif
    sRenderEndBlock = rspq_block_end();

    if (gBlobShadowMat == NULL) {
        gBlobShadowMat = material_init(TEXTURE_SHADOW);
    }

    if (gTempMaterials[0] == NULL) {
        gTempMaterials[0] = material_init(TEXTURE_GRASS0);
    }

    if (gTempMaterials[1] == NULL) {
        gTempMaterials[1] = material_init(TEXTURE_PLANT1);
    }

    for (int i = 0; i < DRAW_TOTAL; i++) {
        if (gSortHeap[i] == NULL) {
            gSortHeap[i] = malloc(sLayerSizes[i]);
            gSortPos[i] = ((unsigned int) gSortHeap[i]) + (sLayerSizes[i] - 0x10);
        }
    }
}

static inline void mtx_invalidate(void* addr) {
    asm volatile (
        "cache 0xD, 0x00(%0);"
        "cache 0xD, 0x10(%0);"
        "cache 0xD, 0x20(%0);"
        "cache 0xD, 0x30(%0);"
        :            
        : "r"(addr)        
    );
}

static inline void set_frustrum(float l, float r, float b, float t, float n, float f) {
    DEBUG_SNAPSHOT_1();
    DEBUG_MATRIX_OP();
    Matrix frustum = (Matrix) { .m={
        {(2*n)/(r-l), 0.f, 0.f, 0.f},
        {0.f, (2.f*n)/(t-b), 0.f, 0.f},
        {(r+l)/(r-l), (t+b)/(t-b), -(f+n)/(f-n), -1.f},
        {0.f, 0.f, -(2*f*n)/(f-n), 0.f},
    }};
    MATRIX_MUL(frustum.m[0], 0, 0);
    get_time_snapshot(PP_MATRIX, DEBUG_SNAPSHOT_1_END);
}

static void lookat_normalise(GLfloat *d, const GLfloat *v) {
    float inv_mag = 1.0f / sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    d[0] = v[0] * inv_mag;
    d[1] = v[1] * inv_mag;
    d[2] = v[2] * inv_mag;
}

static void lookat_cross(GLfloat* p, const GLfloat* a, const GLfloat* b) {
    p[0] = (a[1] * b[2] - a[2] * b[1]);
    p[1] = (a[2] * b[0] - a[0] * b[2]);
    p[2] = (a[0] * b[1] - a[1] * b[0]);
};

static float lookat_dot(const float *a, const float *b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static void mtx_lookat(float eyex, float eyey, float eyez, float centerx, float centery, float centerz, float upx, float upy, float upz) {
    DEBUG_SNAPSHOT_1();
    DEBUG_MATRIX_OP();
    GLfloat eye[3] = {eyex, eyey, eyez};
    GLfloat f[3] = {centerx - eyex, centery - eyey, centerz - eyez};
    GLfloat u[3] = {upx, upy, upz};
    GLfloat s[3];

    lookat_normalise(f, f);

    lookat_cross(s, f, u);
    lookat_normalise(s, s);

    lookat_cross(u, s, f);

    GLfloat m[4][4];
    
    m[0][0] = s[0];
    m[0][1] = u[0];
    m[0][2] = -f[0];
    m[0][3] = 0;

    m[1][0] = s[1];
    m[1][1] = u[1];
    m[1][2] = -f[1];
    m[1][3] = 0;

    m[2][0] = s[2];
    m[2][1] = u[2];
    m[2][2] = -f[2];
    m[2][3] = 0;

    m[3][0] = -lookat_dot(s, eye);
    m[3][1] = -lookat_dot(u, eye);
    m[3][2] = lookat_dot(f, eye);
    m[3][3] = 1;

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            gBillboardMatrix.m[i][j] = m[j][i];
        }
    }
    gBillboardMatrix.m[3][3] = 1.0f;

    memcpy(&gViewMatrix, &m, sizeof(Matrix));

    MATRIX_MUL(&m[0][0], gMatrixStackPos, gMatrixStackPos - 1);
    get_time_snapshot(PP_MATRIX, DEBUG_SNAPSHOT_1_END);
};

static void mtx_translate_rotate(Matrix *mtx, short angleX, short angleY, short angleZ, GLfloat x, GLfloat y, GLfloat z) {
    DEBUG_MATRIX_OP();
    float sx = sins(angleX);
    float cx = coss(angleX);

    float sy = sins(angleY);
    float cy = coss(angleY);

    float sz = sins(angleZ);
    float cz = coss(angleZ);

    mtx->m[0][0] = (cy * cz);
    mtx->m[0][1] = (cy * sz);
    mtx->m[0][2] = -sy;
    mtx->m[0][3] = 0.0f;
    mtx->m[1][0] = (sx * sy * cz - cx * sz);
    mtx->m[1][1] = (sx * sy * sz + cx * cz);
    mtx->m[1][2] = (sx * cy);
    mtx->m[1][3] = 0.0f;
    mtx->m[2][0] = (cx * sy * cz + sx * sz);
    mtx->m[2][1] = (cx * sy * sz - sx * cz);
    mtx->m[2][2] = (cx * cy);
    mtx->m[2][3] = 0.0f;
    mtx->m[3][0] = x;
    mtx->m[3][1] = y;
    mtx->m[3][2] = z;
    mtx->m[3][3] = 1.0f;
}

static void mtx_translate(Matrix *mtx, float x, float y, float z) {
    DEBUG_MATRIX_OP();
    bzero(mtx, sizeof(Matrix));

    mtx->m[0][0] = 1.0f;
    mtx->m[1][1] = 1.0f;
    mtx->m[2][2] = 1.0f;
    mtx->m[3][0] = x;
    mtx->m[3][1] = y;
    mtx->m[3][2] = z;
    mtx->m[3][3] = 1.0f;
}

static void mtx_rotate(Matrix *mtx, short angleX, short angleY, short angleZ) {
    DEBUG_MATRIX_OP();
    float sx = sins(angleX);
    float cx = coss(angleX);

    float sy = sins(angleY);
    float cy = coss(angleY);

    float sz = sins(angleZ);
    float cz = coss(angleZ);

    mtx->m[0][0] = (cy * cz);
    mtx->m[0][1] = (cy * sz);
    mtx->m[0][2] = -sy;
    mtx->m[0][3] = 0.0f;
    mtx->m[1][0] = (sx * sy * cz - cx * sz);
    mtx->m[1][1] = (sx * sy * sz + cx * cz);
    mtx->m[1][2] = (sx * cy);
    mtx->m[1][3] = 0.0f;
    mtx->m[2][0] = (cx * sy * cz + sx * sz);
    mtx->m[2][1] = (cx * sy * sz - sx * cz);
    mtx->m[2][2] = (cx * cy);
    mtx->m[2][3] = 0.0f;
    mtx->m[3][0] = 0.0f;
    mtx->m[3][1] = 0.0f;
    mtx->m[3][2] = 0.0f;
    mtx->m[3][3] = 1.0f;
}

static void mtx_billboard(Matrix *mtx, float x, float y, float z) {
    DEBUG_MATRIX_OP();
    gBillboardMatrix.m[3][0] = x;
    gBillboardMatrix.m[3][1] = y;
    gBillboardMatrix.m[3][2] = z;

    memcpy(mtx, &gBillboardMatrix, sizeof(Matrix));
}

static void mtx_scale(Matrix *mtx, float scaleX, float scaleY, float scaleZ) {
    DEBUG_MATRIX_OP();
    float s[3] = {scaleX, scaleY, scaleZ};
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            mtx->m[i][j] *= s[i];
        }
    }
}

static inline void linear_mtxf_mul_vec3f_and_translate(Matrix m, float dst[3], float v[3]) {
    for (int i = 0; i < 3; i++) {
        dst[i] = ((m.m[0][i] * v[0]) + (m.m[1][i] * v[1]) + (m.m[2][i] * v[2]) +  m.m[3][i]);
    }
}

static inline void linear_mtxf_mul_vec2f_and_translate(Matrix m, float dst[3], float v[3]) {
    for (int i = 0; i < 3; i+=2) {
        dst[i] = ((m.m[0][i] * v[0]) + (m.m[1][i] * v[1]) + (m.m[2][i] * v[2]) +  m.m[3][i]);
    }
}

#define VALIDDEPTHMIDDLE (-19919.0f / 2.0f)
#define VALIDDEPTHRANGE (19900.0f / 2.0f)

static inline int render_inside_view(float width, float height, float screenPos[3]) {
    float hScreenEdge = -screenPos[2] * gHalfFovHor;
    if (fabsf(screenPos[0]) > hScreenEdge + width) {
        return false;
    }
    float vScreenEdge = -screenPos[2] * gHalfFovVert;
    if (fabsf(screenPos[1]) > vScreenEdge + height) {
        return false;
    }
    if (screenPos[2] - VALIDDEPTHMIDDLE >= VALIDDEPTHRANGE + width) {
        return false;
    }
    return true;
}

static inline void *render_alloc(int size, int layer) {
    gSortPos[layer] -= size;
    return (void *) gSortPos[layer];
}

static void set_draw_matrix(Matrix *mtx, int matrixType, float *pos, unsigned short *angle, float *scale) {
    DEBUG_SNAPSHOT_1();
    switch (matrixType) {
    case MTX_TRANSLATE:
        mtx_translate(mtx, pos[0], pos[1], pos[2]);
        break;
    case MTX_ROTATE:
        mtx_rotate(mtx, angle[0], angle[1], angle[2]);
        break;
    case MTX_TRANSLATE_ROTATE_SCALE:
        mtx_translate_rotate(mtx, angle[0], angle[1], angle[2], pos[0], pos[1], pos[2]);
        mtx_scale(mtx, scale[0], scale[1], scale[2]);
        break;
    case MTX_BILLBOARD:
        mtx_billboard(mtx, pos[0], pos[1], pos[2]);
        break;
    case MTX_BILLBOARD_SCALE:
        mtx_billboard(mtx, pos[0], pos[1], pos[2]);
        mtx_scale(mtx, scale[0], scale[1], scale[2]);
        break;
    }
    get_time_snapshot(PP_MATRIX, DEBUG_SNAPSHOT_1_END);
}

static void set_light(light_t light) {
    MATRIX_PUSH();
    Matrix mtx;
    mtx_rotate(&mtx, light.direction[0], light.direction[1], light.direction[2]);
    MATRIX_MUL(mtx.m[0], gMatrixStackPos, gMatrixStackPos - 1);
#if OPENGL
    glLightfv(GL_LIGHT0, GL_POSITION, light.position);
#endif
    MATRIX_POP();
}

static void project_camera(void) {
    Camera *c = gCamera;
    float nearClip = 5.0f;
    float farClip = 1000.0f;
    float fov = gCamera->fov / 50.0f;

#if OPENGL
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
#endif
    gAspectRatio = (float) display_get_width() / (float) (display_get_height()) * fov;
    set_frustrum(-nearClip * gAspectRatio, nearClip * gAspectRatio, -nearClip * fov, nearClip * fov, nearClip, farClip);
#if OPENGL
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
#endif
    mtx_lookat(c->pos[0], c->pos[1], c->pos[2], c->focus[0], c->focus[1], c->focus[2], 0.0f, 1.0f, 0.0f);
    float aspect = display_get_width() / display_get_height();
    //glDepthRange(50.0f, 500.0f);

    gHalfFovVert = (gCamera->fov + 2.0f) * 180.0f + 0.5f;
    gHalfFovHor = aspect * gHalfFovVert;
    float cx, sx;
    sx = sins(gHalfFovVert);
    cx = coss(gHalfFovVert);
    gHalfFovVert = sx / cx;
    sx = sins(gHalfFovHor);
    cx = coss(gHalfFovHor);
    gHalfFovHor = sx / cx;
}

void matrix_ortho(void) {
#if OPENGL
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
#endif
    set_frustrum(0.0f, display_get_width(), display_get_height(), 0.0f, -1.0f, 1.0f);
#if OPENGL
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
#endif
}

void set_particle_render_settings(void) {
    rspq_block_run(gParticleMaterialBlock);
    gRenderSettings.cutout = false;
    gRenderSettings.xlu = true;
    gRenderSettings.depthRead = true;
    gRenderSettings.vertexColour = false;
    gRenderSettings.inter = false;
    gRenderSettings.decal = false;
    gRenderSettings.backface = false;
    gRenderSettings.frontface = false;
    gRenderSettings.ci = false;
    gRenderSettings.texture = true;
#if OPENGL
    if (gEnvironment->flags & ENV_FOG) {
        if (!gRenderSettings.fog) {
            glEnable(GL_FOG);
            gRenderSettings.fog = true;
        }
    } else {
        if (gRenderSettings.fog) {
            glDisable(GL_FOG);
            gRenderSettings.fog = false;
        }
    }
#endif
}

static void material_mode(int flags) {
#if OPENGL
    if (flags & MATERIAL_CUTOUT) {
        if (!gRenderSettings.cutout) {
            glEnable(GL_ALPHA_TEST);
            rdpq_mode_alphacompare(64);
            gRenderSettings.cutout = true;
        }
    } else {
        if (gRenderSettings.cutout) {
            glDisable(GL_ALPHA_TEST);
            rdpq_set_mode_standard();
            gRenderSettings.cutout = false;
        }
    }
    if (flags & MATERIAL_XLU) {
        if (!gRenderSettings.xlu) {
            glEnable(GL_BLEND);
            rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
            glDepthMask(GL_FALSE);
            gRenderSettings.xlu = true;
        }
    } else {
        if (gRenderSettings.xlu) {
            glDisable(GL_BLEND);
            rdpq_mode_blender(0);
            glDepthMask(GL_TRUE);
            gRenderSettings.xlu = false;
        }
    }
    if (flags & MATERIAL_DEPTH_READ) {
        if (!gRenderSettings.depthRead) {
            rdpq_mode_zbuf(true, true);
            glEnable(GL_DEPTH_TEST);
            gRenderSettings.depthRead = true;
        }
    } else {
        if (gRenderSettings.depthRead) {
            rdpq_mode_zbuf(false, false);
            glDisable(GL_DEPTH_TEST);
            gRenderSettings.depthRead = false;
        }
    }
    if (flags & MATERIAL_LIGHTING) {
        if (!gRenderSettings.lighting) {
            glEnable(GL_LIGHTING);
            gRenderSettings.lighting = true;
        }
    } else {
        if (gRenderSettings.lighting) {
            glDisable(GL_LIGHTING);
            gRenderSettings.lighting = false;
        }
    }
    if (flags & MATERIAL_FOG && gEnvironment->flags & ENV_FOG) {
        if (!gRenderSettings.fog) {
            rdpq_mode_fog(RDPQ_FOG_STANDARD);
            gRenderSettings.fog = true;
        }
    } else {
        if (gRenderSettings.fog) {
            rdpq_mode_fog(0);
            gRenderSettings.fog = false;
        }
    }
    if (flags & MATERIAL_VTXCOL) {
        if (!gRenderSettings.vertexColour) {
            glEnable(GL_COLOR_MATERIAL);
            gRenderSettings.vertexColour = true;
        }
    } else {
        if (gRenderSettings.vertexColour) {
            glDisable(GL_COLOR_MATERIAL);
            gRenderSettings.vertexColour = false;
        }
    }
    if (flags & MATERIAL_BACKFACE) {
        if (!gRenderSettings.backface) {
            glDisable(GL_CULL_FACE);
            gRenderSettings.backface = true;
        }
    } else {
        if (gRenderSettings.backface) {
            glEnable(GL_CULL_FACE);
            gRenderSettings.backface = false;
        }
    }
    if (flags & MATERIAL_FRONTFACE) {
        if (!gRenderSettings.frontface) {
            glCullFace(GL_FRONT); 
            gRenderSettings.frontface = true;
        }
    } else {
        if (gRenderSettings.frontface) {
            glCullFace(GL_BACK); 
            gRenderSettings.frontface = false;
        }
    }
    if (flags & MATERIAL_DECAL) {
        if (!gRenderSettings.decal) {
            glDepthFunc(GL_EQUAL);
            gRenderSettings.decal = true;
            gRenderSettings.inter = false;
        }
    } else if (flags & MATERIAL_INTER) {
        if (!gRenderSettings.inter) {
            glDepthFunc(GL_LESS_INTERPENETRATING_N64);
            gRenderSettings.decal = false;
            gRenderSettings.inter = true;
        }
    } else {
        if (gRenderSettings.inter || gRenderSettings.decal) {
            glDepthFunc(GL_LESS);
            gRenderSettings.inter = false;
            gRenderSettings.decal = false;
        }
    }
    if (flags & MATERIAL_CI) {
        if (!gRenderSettings.ci) {
            rdpq_mode_tlut(TLUT_RGBA16);
            gRenderSettings.ci = true;
        }
    } else {
        if (gRenderSettings.ci) {
            rdpq_mode_tlut(TLUT_NONE);
            gRenderSettings.ci = false;
        }
    }
#endif
    gPrevRenderFlags = flags;
}

void material_texture(Material *m) {
    if (m->tex0) {
        if (!gRenderSettings.texture) {
    #if OPENGL
            glEnable(GL_TEXTURE_2D);
    #endif
            gRenderSettings.texture = true;
        }
        rspq_block_run(m->block);
        if (gMaterialIDs[m->entry->materialID].moveS0 || gMaterialIDs[m->entry->materialID].moveT0 ||
            gMaterialIDs[m->entry->materialID].moveS1 || gMaterialIDs[m->entry->materialID].moveT1) {
            material_run_partial(m);
        }
        gNumTextureLoads++;
        gPrevMaterialID = m->entry->materialID;
    } else {
        if (gRenderSettings.texture) {
    #if OPENGL
            glDisable(GL_TEXTURE_2D);
    #endif
            gRenderSettings.texture = false;
        }
    }
}

void material_set(Material *material, int flags) {
    DEBUG_SNAPSHOT_1();
    flags |= material->flags;
    if (gPrevRenderFlags != flags) {
        material_mode(flags);
    }
    if (material->tex0 && gPrevMaterialID != material->tex0->spriteID) {
        material_texture(material);
    }
    get_time_snapshot(PP_MATERIALS, DEBUG_SNAPSHOT_1_END);
}

static void render_sky_gradient(Environment *e) {
    DEBUG_SNAPSHOT_1();
    if (sRenderSkyBlock == NULL) {
        sRenderSkyBlock = sky_gradient_generate(e);
    }
    if (gConfig.graphics != G_PERFORMANCE) {
        rspq_block_run(sRenderSkyBlock);
    } else {
        int width = display_get_width();
        int height = display_get_height();
        rdpq_set_mode_fill(RGBA32((e->skyColourTop[0] + e->skyColourBottom[0]) / 2,
                                  (e->skyColourTop[1] + e->skyColourBottom[1]) / 2,
                                  (e->skyColourTop[2] + e->skyColourBottom[2]) / 2,
                                   255));
        rdpq_fill_rectangle(0, 0, width, height);
        rdpq_set_mode_standard();
    }
    get_time_snapshot(PP_BG, DEBUG_SNAPSHOT_1_END);
}

static void render_sky_texture(Environment *e) {
    DEBUG_SNAPSHOT_1();
    e->skyTimer = 10;
    if (e->texGen == false) {
        sky_texture_generate(e);
        e->texGen = true;
    } else {
        Matrix mtx;
        MATRIX_PUSH();
        mtx_translate(&mtx, gCamera->pos[0], gCamera->pos[1] + 50.0f, gCamera->pos[2]);
        MATRIX_MUL(mtx.m, gMatrixStackPos, gMatrixStackPos - 1);
#if OPENGL
        glEnable(GL_TEXTURE_2D);
        glDisable(GL_CULL_FACE);
        glColor3f(1, 1, 1);
#endif
        int base = gCamera->yaw + 0x8000;
        if (gCamera->viewPitch >= 0x2C00 || gCamera->mode == CAMERA_PHOTO) {
            for (int i = 0; i < 16; i++) {
                short rot = base - ((0x10000 / 16) * i);
                if (fabs(rot) >= 0x4000) {
                    continue;
                }
                float pX = 100.0f * sins((0x10000 / 16) * i);
                float pZ = 100.0f * coss((0x10000 / 16) * i);
#if OPENGL
                glBindTexture(GL_TEXTURE_2D, e->textureSegments[15 - i]);
                glBegin(GL_QUADS);
                glTexCoord2f(1.024f, 0.0f);
                glVertex3f(pX * 0.66f, 75, pZ * 0.66f);
                glTexCoord2f(1.024f, 1.024f);
                glVertex3f(pX, 0.0f, pZ);
#endif
                pX = 100.0f * sins((0x10000 / 16) * (i + 1));
                pZ = 100.0f * coss((0x10000 / 16) * (i + 1));
#if OPENGL
                glTexCoord2f(0.0f, 1.024f);
                glVertex3f(pX, 0.0f, pZ);
                glTexCoord2f(0.0f, 0.0f);
                glVertex3f(pX * 0.66f, 75, pZ * 0.66f);
                glEnd();
#endif
                gNumTextureLoads++;
            }
        }
        
        for (int i = 0; i < 16; i++) {
            short rot = base - ((0x10000 / 16) * i);
            if (fabs(rot) >= 0x4000) {
                continue;
            }
            float pX = 100.0f * sins((0x10000 / 16) * i);
            float pZ = 100.0f * coss((0x10000 / 16) * i);
#if OPENGL
            glBindTexture(GL_TEXTURE_2D, e->textureSegments[31 - i]);
            glBegin(GL_QUADS);
            glTexCoord2f(1.024f, 0.0f);
            glVertex3f(pX, 0.0f, pZ);
            glTexCoord2f(1.024f, 1.024f);
            glVertex3f(pX * 0.66f, -75, pZ * 0.66f);
#endif
            pX = 100.0f * sins((0x10000 / 16) * (i + 1));
            pZ = 100.0f * coss((0x10000 / 16) * (i + 1));
#if OPENGL
            glTexCoord2f(0.0f, 1.024f);
            glVertex3f(pX * 0.66f, -75, pZ * 0.66f);
            glTexCoord2f(0.0f, 0.0f);
            glVertex3f(pX, 0.0f, pZ);
            glEnd();
#endif
            gNumTextureLoads++;
        }
        MATRIX_POP();
    }
    get_time_snapshot(PP_BG, DEBUG_SNAPSHOT_1_END);
}

static void render_bush(void) {
    Environment *e = gEnvironment;
#if OPENGL
    glBegin(GL_QUADS);
    glColor3f(e->skyColourTop[0], e->skyColourTop[1], e->skyColourTop[2]);
    glTexCoord2f(0, 0);
    glVertex3f(-5, 10, 0);
    glColor3f(e->skyColourBottom[0], e->skyColourBottom[1], e->skyColourBottom[2]);
    glTexCoord2f(0, 1.024f);
    glVertex3f(-5, 0, 0);
    glTexCoord2f(2.048f, 1.024f);
    glVertex3f(5, 0, 0);
    glColor3f(e->skyColourTop[0], e->skyColourTop[1], e->skyColourTop[2]);
    glTexCoord2f(2.048f, 0);
    glVertex3f(5, 10, 0);
    glEnd();
#endif
}

static inline void render_end(void) {
    rspq_block_run(sRenderEndBlock);
#if OPENGL
        gl_context_end();
#endif
    rdpq_set_scissor(0, 0, display_get_width(), display_get_height());
}

static void render_shadow(float pos[3], float height) {
    MATRIX_PUSH();
#if OPENGL
    glTranslatef(pos[0], height + 0.1f, pos[2]);
    if (sShadowBlock == NULL) {
        rspq_block_begin();
        glScalef(0.5f, 0.5f, 0.5f);
        glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
        glBegin(GL_QUADS);
        glTexCoord2f(2.048f, 0);        glVertex3f(5.0f, 0.0f, -5.0f);
        glTexCoord2f(0, 0);             glVertex3f(-5.0f, 0.0f, -5.0f);
        glTexCoord2f(0, 2.048f);        glVertex3f(-5.0f, 0.0f, 5.0f);
        glTexCoord2f(2.048f, 2.048f);   glVertex3f(5.0f, 0.0f, 5.0f);
        glEnd();
        sShadowBlock = rspq_block_end();
    }
    rspq_block_run(sShadowBlock);
#endif
    MATRIX_POP();
}

static void apply_anti_aliasing(int mode) {
    switch (gConfig.graphics) {
    case G_PERFORMANCE:
#if OPENGL
        glDisable(GL_MULTISAMPLE_ARB);
#endif
        rdpq_mode_antialias(AA_NONE);
        break;
    case G_DEFAULT:
        if (mode == AA_ACTOR) {
            goto mrFancyPants;
        }
#if OPENGL
        glEnable(GL_MULTISAMPLE_ARB);
        glHint(GL_MULTISAMPLE_HINT_N64, GL_FASTEST);
#endif
        rdpq_mode_antialias(AA_REDUCED);
        break;
    case G_BEAUTIFUL:
        mrFancyPants:
#if OPENGL
        glEnable(GL_MULTISAMPLE_ARB);
        glHint(GL_MULTISAMPLE_HINT_N64, GL_NICEST);
#endif
        rdpq_mode_antialias(AA_STANDARD);
        break;
    }
}

static void render_ztarget_scissor(void) {
    int targetPos;
    if (gConfig.regionMode == TV_PAL) {
        targetPos = gZTargetTimer * (1.5f * 1.2f);
    } else {
        targetPos = gZTargetTimer * 1.5f;
    }
    rdpq_set_scissor(0, targetPos, display_get_width(), display_get_height() - (targetPos));
}

static void apply_render_settings(void) {
    gPrevMaterialID = -1;
    gPrevRenderFlags = -1;
    rspq_block_run(sBeginModeBlock);
    if (gConfig.graphics == G_BEAUTIFUL) {
        *(volatile uint32_t*)0xA4400000 |= 0x10000;
    } else {
        *(volatile uint32_t*)0xA4400000 &= ~0x10000;
    }
}

void find_material_list(RenderNode *node, int layer) {
    // idk if this section is faster, yet.
    if (node->material->entry == NULL) {
        goto lmao;
    }
    if (gPrevMatList && node->material->entry->materialID == gPrevMatList->entryHead->material->entry->materialID) {
        RenderList *list = gPrevMatList;
        if (list->entryHead == gRenderNodeHead[layer]) {
            gRenderNodeHead[layer] = node;
        } else {
            list->entryHead->prev->next = node;
        }
        node->next = list->entryHead;
        node->prev = list->entryHead->prev;
        list->entryHead = node;
        gPrevMatList = list;
        return;
    }
    RenderList *matList;
    if (gRenderNodeHead[layer] == NULL) {
        gRenderNodeHead[layer] = node;
        matList = (RenderList *) render_alloc(sizeof(RenderList), layer);
        gMateriallistHead[layer] = matList;
    } else {
        RenderList *list = gMateriallistHead[layer];
        while (list) {
            if (list->entryHead->material->entry->materialID == node->material->entry->materialID) {
                if (list->entryHead == gRenderNodeHead[layer]) {
                    gRenderNodeHead[layer] = node;
                } else {
                    list->entryHead->prev->next = node;
                }
                node->next = list->entryHead;
                node->prev = list->entryHead->prev;
                list->entryHead = node;
                gPrevMatList = list;
                return;
            }
            list = list->next;
        }
        lmao:
        matList = (RenderList *) render_alloc(sizeof(RenderList), layer);
        gMateriallistTail[layer]->next = matList;
        gRenderNodeTail[layer]->next = node;
    }
    node->next = NULL;
    matList->entryHead = node;
    matList->next = NULL;
    gMateriallistTail[layer] = matList;
    node->prev = gRenderNodeTail[layer];
    gRenderNodeTail[layer] = node;
    gPrevMatList = matList;
}

void add_render_node(RenderNode *entry, rspq_block_t *block, Material *material, int flags, int layer) {
    DEBUG_SNAPSHOT_1();
    entry->block = block;
    entry->material = material;
    entry->flags = flags;
    find_material_list(entry, layer);
    get_time_snapshot(PP_BATCH, DEBUG_SNAPSHOT_1_END);
}

void pop_render_list(int layer) {
    if (gRenderNodeHead[layer] == NULL) {
        return;
    }
    RenderNode *renderList = gRenderNodeHead[layer];
    glEnable(GL_RDPQ_TEXTURING_N64);
    glEnable(GL_RDPQ_MATERIAL_N64);
    while (renderList) {
        MATRIX_PUSH();
        if (renderList->matrix) {
            MATRIX_MUL(renderList->matrix->m, gMatrixStackPos, gMatrixStackPos - 1);
        }
        if (renderList->material) {
            material_set(renderList->material, renderList->flags);
        }
        rdpq_set_prim_color(RGBA32(89, 125, 151, 64));
        //rdpq_set_env_color(renderList->envColour);
        rspq_block_run(renderList->block);
        MATRIX_POP();
        renderList = renderList->next;
    }
    glDisable(GL_RDPQ_MATERIAL_N64);
    glDisable(GL_RDPQ_TEXTURING_N64);
    gRenderNodeHead[layer] = NULL;
    gRenderNodeTail[layer] = NULL;
    gMateriallistHead[layer] = NULL;
    gMateriallistTail[layer] = NULL;
    gPrevMatList = NULL;
#ifdef PUPPYPRINT_DEBUG
    int total = gSortPos[layer] - ((unsigned int) gSortHeap[layer]);
    if (total < gSortRecord[layer]) {
        gSortRecord[layer] = total;
    }
#endif
    gSortPos[layer] = ((unsigned int) gSortHeap[layer]) + sLayerSizes[layer];
}

static void render_particles(void) {
    DEBUG_SNAPSHOT_1();
    ParticleList *list = gParticleListHead;
    Particle *particle;

#if OPENGL
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
#endif
    gRenderSettings.depthRead = true;
    
    while (list) {
        particle = list->particle;
        MATRIX_PUSH();
        /*if (particle->material) {
            material_set(particle->material, 0);
        }*/
        Matrix mtx;
        mtx_billboard(&mtx, particle->pos[0], particle->pos[1], particle->pos[2]);
        mtx_scale(&mtx, particle->scale[0], particle->scale[1], particle->scale[2]);
        MATRIX_MUL(mtx.m, gMatrixStackPos, gMatrixStackPos - 1);
        rspq_block_run(sParticleBlock);
        MATRIX_POP();
        list = list->next;
    }
    get_time_snapshot(PP_RENDERPARTICLES, DEBUG_SNAPSHOT_1_END);
}

static inline int render_inside_view_world(float width, float screenPos[3]) {
    float hScreenEdge = -screenPos[2] * gHalfFovHor;
    if (fabsf(screenPos[0]) > hScreenEdge + width) {
        return false;
    }
    if (screenPos[2] - VALIDDEPTHMIDDLE >= VALIDDEPTHRANGE + width) {
        return false;
    }
    return true;
}

static int render_world_visible(SceneChunk *c) {
    DEBUG_SNAPSHOT_1();
    float screenPos[3];
    float pos[3];
    float size[3];
    float width;
    size[0] = c->bounds[1][0] - c->bounds[0][0];
    size[1] = c->bounds[1][1] - c->bounds[0][1];
    size[2] = c->bounds[1][2] - c->bounds[0][2];
    pos[0] = c->bounds[0][0] + (size[0] / 2);
    pos[1] = c->bounds[0][1] + (size[1] / 2);
    pos[2] = c->bounds[0][2] + (size[2] / 2);
    width = MAX(size[0], size[2]);

    float dist = DIST2_Z(gCamera->focus, pos);


    if (dist > (width + 400.0f) * (width + 400.0f)) {
        return false;
    }

    linear_mtxf_mul_vec2f_and_translate(gViewMatrix, screenPos, pos);
    if (render_inside_view_world(width, screenPos)) {
        get_time_snapshot(PP_CULLING, DEBUG_SNAPSHOT_1_END);
        return true;
    } else {
        get_time_snapshot(PP_CULLING, DEBUG_SNAPSHOT_1_END);
        return false;
    }


    return true;
}

static void render_world(int updateRate) {
    DEBUG_SNAPSHOT_1();
    int i = 0;
    if (gCurrentScene && gCurrentScene->model) {
        SceneChunk *s = gCurrentScene->chunkList;

        while (s != NULL) {
            if (gScreenshotStatus != SCREENSHOT_SHOW && render_world_visible(s)) {
                SceneMesh *c = s->meshList;
                i++;
                while (c != NULL) {
                    int layer;

                    if (c->renderBlock == NULL) {
                        scene_generate_chunk(c);
                    }

                    if (c->material->flags & MATERIAL_DECAL) {
                        layer = DRAW_DECAL;
                    } else if (c->material->flags & MATERIAL_XLU) {
                        layer = DRAW_XLU;
                    } else {
                        layer = DRAW_OPA;
                    }
                    
                    RenderNode *entry = (RenderNode *) render_alloc(sizeof(RenderNode), layer);
                    entry->matrix = NULL;
                    //Material *mat = gUseOverrideMaterial ? &gOverrideMaterial : c->material;
                    add_render_node(entry, c->renderBlock, c->material, MATERIAL_NULL, layer);
                    c = c->next;
                }
                s->flags |= CHUNK_HAS_MODEL;
            } else {
                if (s->flags & CHUNK_HAS_MODEL) {
                    rspq_call_deferred((void *) scene_clear_chunk, s);
                }
            }
            s = s->next;
        }
    }
    pop_render_list(DRAW_OPA);
    pop_render_list(DRAW_DECAL);
    get_time_snapshot(PP_RENDERLEVEL, DEBUG_SNAPSHOT_1_END);
    profiler_wait();
}

static void render_object_shadows(void) {
    profiler_wait();
    DEBUG_SNAPSHOT_1();
    ObjectList *list = gObjectListHead;
    Object *obj;
    
    glEnable(GL_RDPQ_TEXTURING_N64);
    glEnable(GL_RDPQ_MATERIAL_N64);
    material_set(gBlobShadowMat, MATERIAL_DECAL);
    while (list) {
        obj = list->obj;
        if (obj->flags & OBJ_FLAG_IN_VIEW && (obj->flags & OBJ_FLAG_SHADOW || (gConfig.graphics == G_PERFORMANCE && obj->flags & OBJ_FLAG_SHADOW_DYNAMIC))) { 
            float height;
            if (obj->collision) {
                height = obj->collision->floorHeight;
            } else {
                height = obj->pos[1];
            }
            render_shadow(obj->pos, height);
        }
        list = list->next;
    }

    glDisable(GL_RDPQ_TEXTURING_N64);
    glDisable(GL_RDPQ_MATERIAL_N64);
    if (gConfig.graphics == G_PERFORMANCE) {
        get_time_snapshot(PP_SHADOWS, DEBUG_SNAPSHOT_1_END);
        profiler_wait();
        return;
    }
    list = gObjectListHead;
    material_set(&gBlankMaterial, MATERIAL_DECAL | MATERIAL_XLU | MATERIAL_DEPTH_READ);
#if OPENGL
    glEnable(GL_TEXTURE_2D);
#endif
    while (list) {
        obj = list->obj;
        if (obj->flags & OBJ_FLAG_SHADOW_DYNAMIC && obj->flags & OBJ_FLAG_IN_VIEW && obj->gfx && obj->gfx->dynamicShadow) {
            DynamicShadow *d = obj->gfx->dynamicShadow;
            Matrix matrix;
            MATRIX_PUSH();
            float floorHeight = MAX(obj->collision->floorHeight, obj->collision->hitboxHeight);
            float pos[3] = {obj->pos[0], floorHeight + 0.1f, obj->pos[2]};
            unsigned short angle[3] = {0, d->angle[1] + 0x4000, 0};
            set_draw_matrix(&matrix, MTX_TRANSLATE_ROTATE_SCALE, pos, angle, obj->scale);
            MATRIX_MUL(&matrix.m, gMatrixStackPos, gMatrixStackPos - 1);
#if OPENGL
            glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
#endif
            float width = d->planeW / d->acrossX;
            float height = d->planeH / d->acrossY;
            float offset = d->offset;
            int texLoads = 0;
            float x;
            float y = offset;
            for (int i = 0; i < d->acrossY; i++) {
                x = 0.0f - (d->planeW / 2);
                for (int j = 0; j < d->acrossX; j++) {
#if OPENGL
                    glBindTexture(GL_TEXTURE_2D, d->tex[texLoads++]);
#endif
                    gNumTextureLoads++;
#if OPENGL
                    glBegin(GL_QUADS);
                    glTexCoord2f(0.0f, 1.024f);        glVertex3f(x + width, 0.0f, y);
                    glTexCoord2f(1.024f, 1.024f);             glVertex3f(x, 0.0f, y);
                    glTexCoord2f(1.024f, 0.0f);        glVertex3f(x, 0.0f, y + height);
                    glTexCoord2f(0.0f, 0.0f);   glVertex3f(x + width, 0.0f, y + height);
                    glEnd();
#endif
                    x += width;
                }
                y += height;
            }

            MATRIX_POP();
        }
        list = list->next;
    }
    get_time_snapshot(PP_SHADOWS, DEBUG_SNAPSHOT_1_END);
    profiler_wait();
}

static void render_clutter(void) {
    if (gConfig.graphics == G_PERFORMANCE) {
        return;
    }
    DEBUG_SNAPSHOT_1();
    ClutterList *list = gClutterListHead;
    Clutter *obj; 
    if (sBushBlock == NULL) {
        rspq_block_begin();
        render_bush();
        sBushBlock = rspq_block_end();
    }
    while (list) {
        obj = list->clutter;
        if (obj->flags & OBJ_FLAG_IN_VIEW && !(obj->flags & OBJ_FLAG_INVISIBLE)) {
            if (obj->objectID == CLUTTER_BUSH) {
                RenderNode *entry = (RenderNode *) render_alloc(sizeof(RenderNode), DRAW_OPA);
                entry->matrix = (Matrix *) render_alloc(sizeof(Matrix), DRAW_OPA);
                
                mtx_billboard(entry->matrix, obj->pos[0], obj->pos[1], obj->pos[2]);
                //Material *mat = gUseOverrideMaterial ? &gOverrideMaterial : &gTempMaterials[1];
                add_render_node(entry, sBushBlock, gTempMaterials[1], MATERIAL_NULL, DRAW_OPA);
            } else if (obj->objectID == CLUTTER_ROCK) {
                RenderNode *entry = (RenderNode *) render_alloc(sizeof(RenderNode), DRAW_OPA);
                entry->matrix = (Matrix *) render_alloc(sizeof(Matrix), DRAW_OPA);
                mtx_billboard(entry->matrix, obj->pos[0], obj->pos[1], obj->pos[2]);
                //Material *mat = gUseOverrideMaterial ? &gOverrideMaterial : &gTempMaterials[0];
                add_render_node(entry, sBushBlock, gTempMaterials[0], MATERIAL_NULL, DRAW_OPA);
            }
        }
        list = list->next;
    }
    pop_render_list(DRAW_OPA);
    get_time_snapshot(PP_RENDERCLUTTER, DEBUG_SNAPSHOT_1_END);
    profiler_wait();
}

static void render_objects(void) {
    profiler_wait();
    DEBUG_SNAPSHOT_1();
    ObjectList *list = gObjectListHead;
    Object *obj;
    
    while (list) {
        obj = list->obj;
        if (obj->flags & OBJ_FLAG_IN_VIEW && !(obj->flags & OBJ_FLAG_INVISIBLE) && obj->gfx) {
            ObjectModel *m = obj->gfx->listEntry->entry;
            Matrix *prevMtx = NULL;
            obj->gfx->listEntry->timer = 10;
            if (obj->animID != ANIM_NONE) {
#if OPENGL
                model64_update(obj->gfx->listEntry->model64, 1.0f / 60.0f);
#endif
            }
            if (obj->gfx->listEntry->active == false) {
                object_model_generate(obj);
            }
            while (m) {
                int layer;
                if (m->material->flags & MATERIAL_DECAL || m->material->flags & MATERIAL_XLU) {
                    layer = DRAW_XLU;
                } else {
                    layer = DRAW_OPA;
                }
                RenderNode *entry = (RenderNode *) render_alloc(sizeof(RenderNode), layer);
                if (m->matrixBehaviour != MTX_NONE) {
                    entry->matrix = (Matrix *) render_alloc(sizeof(Matrix), layer);
                    prevMtx = entry->matrix;
                    set_draw_matrix(entry->matrix, m->matrixBehaviour, obj->pos, obj->faceAngle, obj->scale);
                } else {
                    entry->matrix = prevMtx;
                }
                //Material *mat = gUseOverrideMaterial ? &gOverrideMaterial : &m->material;
                add_render_node(entry, m->block, m->material, MATERIAL_NULL, layer);
                /*if (obj->flags & OBJ_FLAG_OUTLINE) {
                    RenderNode *entry2 = (RenderNode *) render_alloc(sizeof(RenderNode), DRAW_XLU);
                    entry2->matrix = (Matrix *) render_alloc(sizeof(Matrix), DRAW_XLU);
                    if (m->matrixBehaviour != MTX_NONE) {
                        set_draw_matrix(entry2->matrix, m->matrixBehaviour, obj->pos, obj->faceAngle, obj->scale);
                    } else {
                        memcpy(entry2->matrix, prevMtx, sizeof(Matrix));
                    }
                    mtx_scale(entry2->matrix, 1.05f, 1.00f, 1.05f);
                    entry2->primColour = RGBA32(255, 0, 0, 192);
                    add_render_node(entry2, m->block, m->material, MATERIAL_FRONTFACE, DRAW_XLU);
                }*/
                m = m->next;
            }
        }
        list = list->next;
    }

    pop_render_list(DRAW_OPA);
    pop_render_list(DRAW_XLU);
    get_time_snapshot(PP_RENDEROBJECTS, DEBUG_SNAPSHOT_1_END);
    profiler_wait();
}

rspq_block_t *sDynamicShadowBlock;

static void reset_shadow_perspective(void) {
    if (sDynamicShadowBlock == NULL) {
        float nearClip = 5.0f;
        float farClip = 500.0f;
        rspq_block_begin();
#if OPENGL
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
#endif
        set_frustrum(-nearClip, nearClip, -nearClip, nearClip, nearClip, farClip);
#if OPENGL
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
#endif
        material_set(&gBlankMaterial, MATERIAL_CAM_ONLY);
#if OPENGL
        glEnable(GL_RDPQ_MATERIAL_N64);
#endif
        rdpq_set_mode_copy(false);
        rdpq_mode_antialias(AA_NONE);
        rdpq_mode_blender(0);
        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
        rdpq_set_prim_color(RGBA32(255, 255, 255, 255));
        sDynamicShadowBlock = rspq_block_end();
    }
    rspq_block_run(sDynamicShadowBlock);
}

static void generate_dynamic_shadows(void) {
    profiler_wait();
    DEBUG_SNAPSHOT_1();
    ObjectList *list = gObjectListHead;
    Object *obj;

    apply_render_settings();
    float pos[3] = {0.0f, 0.0f, 0.0f};
    float scale[3] = {1.0f, 1.0f, 1.0f};
    reset_shadow_perspective();
    
    while (list) {
        obj = list->obj;
        if (obj->flags & OBJ_FLAG_SHADOW_DYNAMIC && obj->flags & OBJ_FLAG_IN_VIEW && !(obj->flags & OBJ_FLAG_INVISIBLE) && obj->gfx && obj->overlay) {
            if (obj->gfx->dynamicShadow == false) {
                if (obj->header->dynamicShadow == NULL) {
                    list = list->next;
                    continue;
                }
                shadow_generate(obj);
            }
            rdpq_attach(&obj->gfx->dynamicShadow->surface, NULL);
            rdpq_clear(RGBA32(0, 0, 0, 0));
#if OPENGL
            gl_context_begin();
#endif
            MATRIX_PUSH();
            mtx_lookat(obj->gfx->dynamicShadow->camPos[0], obj->gfx->dynamicShadow->camPos[1], obj->gfx->dynamicShadow->camPos[2],
                       obj->gfx->dynamicShadow->camFocus[0], obj->gfx->dynamicShadow->camFocus[1], obj->gfx->dynamicShadow->camFocus[2], 0.0f, 1.0f, 0.0f);
            obj->gfx->dynamicShadow->staleTimer = 10;
            ObjectModel *m = obj->gfx->listEntry->entry;
            if (obj->gfx->listEntry->active == false) {
                object_model_generate(obj);
            }
            while (m) {
                Matrix matrix;
                if (m->matrixBehaviour != MTX_NONE) {
                    unsigned short angle[3] = {obj->faceAngle[0], obj->faceAngle[1] + obj->gfx->dynamicShadow->angle[1], obj->faceAngle[2]};
                    set_draw_matrix(&matrix, m->matrixBehaviour, pos, angle, scale);
                    MATRIX_MUL(&matrix.m, gMatrixStackPos, gMatrixStackPos - 1);
                }
                MATRIX_PUSH();
                rspq_block_run(m->block);
                MATRIX_POP();
                m = m->next;
            }
            MATRIX_POP();
#if OPENGL
            gl_context_end();
#endif
            rdpq_detach();
        }
        list = list->next;
    }
#if OPENGL
    glDisable(GL_RDPQ_MATERIAL_N64);
#endif
    get_time_snapshot(PP_SHADOWS, DEBUG_SNAPSHOT_1_END);
    profiler_wait();
}

void clear_dynamic_shadows(void) {
    ObjectList *objList = gObjectListHead;
    Object *obj;
    while (objList) {
        obj = objList->obj;
        if (obj->gfx && obj->gfx->dynamicShadow) {
            free_dynamic_shadow(obj);
        }
        objList = objList->next;
    }
}

static void render_determine_visible(void) {
    DEBUG_SNAPSHOT_1();
    ObjectList *list = gObjectListHead;
    Object *obj;

    while (list) {
        obj = list->obj;
        if (!(obj->flags & OBJ_FLAG_INVISIBLE)) {
            float screenPos[3];
            float pos[3] = {obj->pos[0], obj->pos[1] + (obj->gfx->yOffset * obj->scale[1]), obj->pos[2]};
            float highScale = MAX(obj->scale[0], obj->scale[2]);
            linear_mtxf_mul_vec3f_and_translate(gViewMatrix, screenPos, pos);
            if (render_inside_view(obj->gfx->width * highScale, obj->gfx->height * obj->scale[1], screenPos)) {
                obj->flags |= OBJ_FLAG_IN_VIEW;
            } else {
                obj->flags &= ~OBJ_FLAG_IN_VIEW;
            }
        }
        list = list->next;
    }

    ClutterList *cList = gClutterListHead;
    Clutter *clu; 
    while (cList) {
        clu = cList->clutter;
        if (!(clu->flags & OBJ_FLAG_INVISIBLE)) {
            float screenPos[3];
            float pos[3] = {clu->pos[0], clu->pos[1] + (clu->gfx->yOffset * clu->scale[1]), clu->pos[2]};
            float highScale = MAX(clu->scale[0], clu->scale[2]);
            linear_mtxf_mul_vec3f_and_translate(gViewMatrix, screenPos, pos);
            if (render_inside_view(clu->gfx->width * highScale, clu->gfx->height * clu->scale[1], screenPos)) {
                clu->flags |= OBJ_FLAG_IN_VIEW;
            } else {
                clu->flags &= ~OBJ_FLAG_IN_VIEW;
            }
        }
        cList = cList->next;
    }
    get_time_snapshot(PP_CULLING, DEBUG_SNAPSHOT_1_END);
}

void render_game(int updateRate, float updateRateF) {
#ifdef PUPPYPRINT_DEBUG
    DEBUG_SNAPSHOT_1();
    unsigned int offset = gDebugData->timer[PP_HALT][gDebugData->iteration];
    for (int i = 0; i < DRAW_TOTAL; i++) {
        gSortRecord[i] = sLayerSizes[i];
    }
#endif
    if (gScreenshotStatus != SCREENSHOT_SHOW && gConfig.graphics != G_PERFORMANCE) {
        generate_dynamic_shadows();
    }
    if (gScreenshotStatus > SCREENSHOT_NONE) {
        screenshot_generate();
    } else {
        rdpq_attach(gFrameBuffers, &gZBuffer);
    }
#if OPENGL
    gl_context_begin();
    glClear(GL_DEPTH_BUFFER_BIT);
#elif TINY3D
    t3d_frame_start();
    t3d_screen_clear_depth();
    gMatrixStackPos = 1;
#endif
    if (gScreenshotStatus != SCREENSHOT_SHOW) {
        glDisable(GL_MULTISAMPLE_ARB);
        rdpq_mode_antialias(AA_NONE);
        render_ztarget_scissor();
        if (gEnvironment->skyboxTextureID == -1 || gConfig.graphics == G_PERFORMANCE) {
            render_sky_gradient(gEnvironment);
        }
        project_camera();
        if (gEnvironment->skyboxTextureID != -1 && gConfig.graphics != G_PERFORMANCE) {
            render_sky_texture(gEnvironment);
        }
        apply_render_settings();
        set_light(lightNeutral);
        render_determine_visible();
        apply_anti_aliasing(AA_GEO);
        render_world(updateRate);
        //apply_anti_aliasing(AA_GEO);
        render_object_shadows();
        //apply_anti_aliasing(AA_GEO);
        render_clutter();
        apply_anti_aliasing(AA_ACTOR);
        render_objects();
        apply_anti_aliasing(AA_GEO);
        set_particle_render_settings();
        //render_particles();
        render_end();
        if (gScreenshotStatus == SCREENSHOT_GENERATE) {
            clear_dynamic_shadows();
        }
    } else if (gScreenshotStatus == SCREENSHOT_SHOW) {
        render_world(updateRate);
        render_end();
        if (gScreenshotType == FMT_RGBA16) {
            rdpq_set_mode_copy(false);
        } else {
            int sineTime = 16 * sins(gGlobalTimer * 0x2000);
            rdpq_set_mode_fill(RGBA32(151, 100, 60, 255));
            rdpq_fill_rectangle(0, 0, display_get_width(), display_get_height());
            rdpq_set_mode_standard();
            rdpq_set_prim_color(RGBA32(234, 219, 203, 239 + sineTime));
            rdpq_mode_combiner(RDPQ_COMBINER_TEX_FLAT);
            rdpq_mode_dithering(DITHER_NOISE_NOISE);
            rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
        }
        rdpq_tex_blit(&gScreenshot, 0, 0, NULL);
        rdpq_set_mode_standard();
        rdpq_mode_dithering(DITHER_SQUARE_SQUARE);
    }
    get_time_snapshot(PP_RENDER, DEBUG_SNAPSHOT_1_END);
    if (gScreenshotStatus <= SCREENSHOT_NONE) {
        render_hud(updateRate, updateRateF);
        render_menus(updateRate, updateRateF);
    }
#ifdef PUPPYPRINT_DEBUG
    offset = gDebugData->timer[PP_HALT][gDebugData->iteration] - offset;
    add_time_offset(PP_RENDER, offset);
#endif
}