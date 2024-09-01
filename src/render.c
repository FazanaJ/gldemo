#include <libdragon.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>
#include <malloc.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>
#include <t3d/t3danim.h>
#include <t3d/t3ddebug.h>

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
static rspq_block_t *sRenderEndBlock;
rspq_block_t *sRenderSkyBlock;
static rspq_block_t *sBeginModeBlock;
static rspq_block_t *sParticleBlock;
Material gOverrideMaterial;
Material gBlankMaterial;
Material *gBlobShadowMat;
void *gSortHeap[DRAW_TOTAL];
unsigned int gSortPos[DRAW_TOTAL];
float gHalfFovHor;
float gHalfFovVert;
RenderSettings gRenderSettings;
rspq_block_t *gParticleMaterialBlock;
int gPrevRenderFlags;
int gPrevMaterialID;
unsigned long long gOtherModeFlags;
unsigned long long gPrevOtherModeFlags;
unsigned int sT3dFlags;
unsigned int sPrevT3dFlags;
T3DViewport gViewport;
#ifdef PUPPYPRINT_DEBUG
int gSortRecord[DRAW_TOTAL];
#endif

light_t lightNeutral = {
    colour: {0.66f, 0.66f, 0.66f, 0.66f},
    diffuse: {1.0f, 1.0f, 1.0f, 1.0f},
    direction: {0, 0, 0x6000},
    position: {1.0f, 0.0f, 0.0f, 0.0f},
    radius: 10.0f,
};

const short sLayerSizes[DRAW_TOTAL] = {
    0x200, // Non ZBuffer
    0x3800, // Standard
    0x400, // Decal
    0x800, // Semitransparent
    0x400, // Misc
};


tstatic void init_particles(void) {
    rspq_block_begin();
    
    sParticleBlock = rspq_block_end();
}

tstatic inline void setup_light(light_t light) {
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
    gViewport = t3d_viewport_create();

    rspq_block_begin();
    rdpq_set_mode_standard();
    rdpq_mode_filter(FILTER_BILINEAR);
    rdpq_mode_dithering(DITHER_SQUARE_SQUARE);
    rdpq_mode_persp(true);
    __rdpq_mode_change_som(0x300000000001, 0x200000000000);
    sBeginModeBlock = rspq_block_end();

    rspq_block_begin();
    rdpq_set_mode_standard();
    rdpq_mode_tlut(TLUT_NONE);
    rdpq_mode_fog(0);
    rdpq_mode_zbuf(false, false);
    rdpq_mode_blender(0);
    rdpq_mode_alphacompare(0);
    sRenderEndBlock = rspq_block_end();

    if (gBlobShadowMat == NULL) {
        gBlobShadowMat = material_init(MATERIAL_SHADOW);
    }

    for (int i = 0; i < DRAW_TOTAL; i++) {
        if (gSortHeap[i] == NULL) {
            gSortHeap[i] = malloc(sLayerSizes[i] + 0x10);
            gSortHeap[i] = (void *) (((int) gSortHeap[i] + 0xF) & ~0xF);
            gSortPos[i] = ((unsigned int) gSortHeap[i]) + (sLayerSizes[i] - 0x10);
        }
    }
}

tstatic inline void mtx_invalidate(void* addr) {
    asm volatile (
        "cache 0xD, 0x00(%0);"
        "cache 0xD, 0x10(%0);"
        "cache 0xD, 0x20(%0);"
        "cache 0xD, 0x30(%0);"
        :            
        : "r"(addr)        
    );
}

tstatic inline void set_frustrum(float l, float r, float b, float t, float n, float f) {
    DEBUG_SNAPSHOT_1();
    DEBUG_MATRIX_OP();
    /*Matrix frustum = (Matrix) { .m={
        {(2*n)/(r-l), 0.f, 0.f, 0.f},
        {0.f, (2.f*n)/(t-b), 0.f, 0.f},
        {(r+l)/(r-l), (t+b)/(t-b), -(f+n)/(f-n), -1.f},
        {0.f, 0.f, -(2*f*n)/(f-n), 0.f},
    }};*/
    //MATRIX_MUL(frustum.m[0], 0, 0);
    get_time_snapshot(PP_MATRIX, DEBUG_SNAPSHOT_1_END);
}

tstatic void lookat_normalise(GLfloat *d, const GLfloat *v) {
    float inv_mag = 1.0f / sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    d[0] = v[0] * inv_mag;
    d[1] = v[1] * inv_mag;
    d[2] = v[2] * inv_mag;
}

tstatic void lookat_cross(GLfloat* p, const GLfloat* a, const GLfloat* b) {
    p[0] = (a[1] * b[2] - a[2] * b[1]);
    p[1] = (a[2] * b[0] - a[0] * b[2]);
    p[2] = (a[0] * b[1] - a[1] * b[0]);
};

tstatic float lookat_dot(const float *a, const float *b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

tstatic void mtx_lookat(float eyex, float eyey, float eyez, float centerx, float centery, float centerz, float upx, float upy, float upz) {
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

    //MATRIX_MUL(&m[0][0], 0, 0);
    get_time_snapshot(PP_MATRIX, DEBUG_SNAPSHOT_1_END);
};

tstatic void mtx_translate_rotate(Matrix *mtx, short angleX, short angleY, short angleZ, GLfloat x, GLfloat y, GLfloat z) {
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

tstatic void mtx_translate(Matrix *mtx, float x, float y, float z) {
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

tstatic void mtx_rotate(Matrix *mtx, short angleX, short angleY, short angleZ) {
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

tstatic void mtx_billboard(Matrix *mtx, float x, float y, float z) {
    DEBUG_MATRIX_OP();
    gBillboardMatrix.m[3][0] = x;
    gBillboardMatrix.m[3][1] = y;
    gBillboardMatrix.m[3][2] = z;

    memcpy(mtx, &gBillboardMatrix, sizeof(Matrix));
}

tstatic void mtx_scale(Matrix *mtx, float scaleX, float scaleY, float scaleZ) {
    DEBUG_MATRIX_OP();
    float s[3] = {scaleX, scaleY, scaleZ};
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            mtx->m[i][j] *= s[i];
        }
    }
}

tstatic inline void linear_mtxf_mul_vec3f_and_translate(Matrix m, float dst[3], float v[3]) {
    for (int i = 0; i < 3; i++) {
        dst[i] = ((m.m[0][i] * v[0]) + (m.m[1][i] * v[1]) + (m.m[2][i] * v[2]) +  m.m[3][i]);
    }
}

tstatic inline void linear_mtxf_mul_vec2f_and_translate(Matrix m, float dst[3], float v[3]) {
    for (int i = 0; i < 3; i+=2) {
        dst[i] = ((m.m[0][i] * v[0]) + (m.m[1][i] * v[1]) + (m.m[2][i] * v[2]) +  m.m[3][i]);
    }
}

#define VALIDDEPTHMIDDLE (-19919.0f / 2.0f)
#define VALIDDEPTHRANGE (19900.0f / 2.0f)

tstatic inline int render_inside_view(float width, float height, float screenPos[3]) {
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

tstatic inline void *render_alloc(int size, int layer) {
    gSortPos[layer] -= size;
    return (void *) gSortPos[layer];
}

tstatic void set_draw_matrix(Matrix *mtx, int matrixType, float *pos, unsigned short *angle, float *scale) {
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

void clutter_matrix(Matrix *mtx, int matrixBehaviour, float *pos, unsigned short *angle, float *scale) {
    set_draw_matrix(mtx, matrixBehaviour, pos, angle, scale);
}

tstatic void light_apply(light_t light) {
    DEBUG_SNAPSHOT_1();
    static T3DVec3 lightDirVec = {{1.0f, 1.0f, 1.0f}};
    static uint8_t colorAmbient[4] = {0xAA, 0xAA, 0xAA, 0xFF};
    static uint8_t colorDir[4]     = {0xFF, 0xAA, 0xAA, 0xFF};
    t3d_vec3_norm(&lightDirVec);
    t3d_light_set_ambient(colorAmbient);
    t3d_light_set_directional(0, colorDir, &lightDirVec);
    t3d_light_set_count(4);
    get_time_snapshot(PP_LIGHTS, DEBUG_SNAPSHOT_1_END);
}

tstatic void camera_project(void) {
    Camera *c = gCamera;
    float fov = gCamera->fov / 50.0f;
    static float prevFov = 0.0f;
    t3d_viewport_set_projection(&gViewport, T3D_DEG_TO_RAD(85.0f), 10.0f, 2500.0f);
    t3d_viewport_look_at(&gViewport, (T3DVec3 *) &gCamera->pos, (T3DVec3 *) &gCamera->focus, &(T3DVec3){{0,1,0}});
    mtx_lookat(c->pos[0], c->pos[1], c->pos[2], c->focus[0], c->focus[1], c->focus[2], 0.0f, 1.0f, 0.0f);
    float aspect = display_get_width() / display_get_height();

    if (prevFov != fov) {
        gHalfFovVert = (gCamera->fov + 2.0f) * 180.0f + 0.5f;
        gHalfFovHor = aspect * gHalfFovVert;
        float cx, sx;
        sx = sins(gHalfFovVert);
        cx = coss(gHalfFovVert);
        gHalfFovVert = sx / cx;
        sx = sins(gHalfFovHor);
        cx = coss(gHalfFovHor);
        gHalfFovHor = sx / cx;
        prevFov = fov;
    }
}

void matrix_ortho(void) {
    set_frustrum(0.0f, display_get_width(), display_get_height(), 0.0f, -1.0f, 1.0f);
}

tstatic void set_particle_render_settings(void) {
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

tstatic void material_mode(int flags) {
    if (flags & MAT_CUTOUT) {
        if (!gRenderSettings.cutout) {
            rdpq_mode_alphacompare(192);
            gRenderSettings.cutout = true;
            if (gRenderSettings.xlu) {
                rdpq_mode_blender(0);
                gRenderSettings.xlu = false;
            }
        }
    } else {
        if (gRenderSettings.cutout) {
            rdpq_mode_alphacompare(0);
            gRenderSettings.cutout = false;
        }
        if (flags & MAT_XLU) {
            if (!gRenderSettings.xlu) {
                rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
                gRenderSettings.xlu = true;
            }
        } else {
            if (gRenderSettings.xlu) {
                rdpq_mode_blender(0);
                gRenderSettings.xlu = false;
            }
        }
    }
    if (flags & MAT_DEPTH_READ) {
        sT3dFlags |= T3D_FLAG_DEPTH;
        if (!gRenderSettings.depthRead) {
            int write;
            if (flags & MAT_DECAL || flags & MAT_XLU) {
                write = false;
            } else {
                write = true;
            }
            rdpq_mode_zbuf(true, write);
            gRenderSettings.depthRead = true;
        }
    } else {
        sT3dFlags &= ~T3D_FLAG_DEPTH;
        if (gRenderSettings.depthRead) {
            rdpq_mode_zbuf(false, false);
            gRenderSettings.depthRead = false;
        }
    }
    if ((flags & MAT_LIGHTING) == 0 && (flags & MAT_VTXCOL) == 0) {
        sT3dFlags &= ~T3D_FLAG_SHADED;
    } else {
        sT3dFlags |= T3D_FLAG_SHADED;
    }
    if (flags & MAT_FOG && gEnvironment->flags & ENV_FOG) {
        if (!gRenderSettings.fog) {
            t3d_fog_set_enabled(true);
            rdpq_mode_fog(RDPQ_FOG_STANDARD);
            gRenderSettings.fog = true;
        }
    } else {
        if (gRenderSettings.fog) {
            rdpq_mode_fog(0);
            t3d_fog_set_enabled(false);
            gRenderSettings.fog = false;
        }
    }
    if (flags & MAT_BACKFACE) {
        sT3dFlags &= ~T3D_FLAG_CULL_BACK;
    } else {
        sT3dFlags |= T3D_FLAG_CULL_BACK;
    }
    if (flags & MAT_FRONTFACE) {
        sT3dFlags |= T3D_FLAG_CULL_FRONT;
    } else {
        sT3dFlags &= ~T3D_FLAG_CULL_FRONT;
    }
    if (flags & MAT_DECAL) {
        if (!gRenderSettings.decal) {
            gRenderSettings.decal = true;
            gRenderSettings.inter = false;
        }
    } else if (flags & MAT_INTER) {
        if (!gRenderSettings.inter) {
            gRenderSettings.decal = false;
            gRenderSettings.inter = true;
        }
    } else {
        if (gRenderSettings.inter || gRenderSettings.decal) {
            gRenderSettings.inter = false;
            gRenderSettings.decal = false;
        }
    }
    if (flags & MAT_CI) {
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
    gPrevRenderFlags = flags;
}

tstatic void material_texture(Material *m) {
    rspq_block_run(m->block);
    if (m->tex0) {
        sT3dFlags |= T3D_FLAG_TEXTURED;
        if (gMaterialIDs[m->entry->materialID].moveS0 || gMaterialIDs[m->entry->materialID].moveT0 ||
            gMaterialIDs[m->entry->materialID].moveS1 || gMaterialIDs[m->entry->materialID].moveT1) {
            if (gTextureIDs[m->tex0->spriteID].flipbook == 0) {
                material_run_partial(m);
            }
        }
        if (gTextureIDs[m->tex0->spriteID].flipbook) {
            material_run_flipbook(m);
        }
        gNumTextureLoads++;
        gPrevMaterialID = m->entry->materialID;
    } else {
        sT3dFlags &= ~T3D_FLAG_TEXTURED;
    }
}

tstatic void material_set(Material *material, int flags) {
    DEBUG_SNAPSHOT_1();
    flags |= material->flags;
    rdpq_sync_pipe();
    if (gPrevRenderFlags != flags) {
        material_mode(flags);
    }
    if (gPrevMaterialID != material->entry->materialID) {
        material_texture(material);
    }
    
    if (sT3dFlags != sPrevT3dFlags) {
        t3d_state_set_drawflags(sT3dFlags);
        sPrevT3dFlags = sT3dFlags;
    }
    get_time_snapshot(PP_MATERIALS, DEBUG_SNAPSHOT_1_END);
}

tstatic void render_sky_flat(Environment *e) {
    DEBUG_SNAPSHOT_1();
    rdpq_clear(RGBA32((e->skyColourTop[0] + e->skyColourBottom[0]) / 2,
                              (e->skyColourTop[1] + e->skyColourBottom[1]) / 2,
                              (e->skyColourTop[2] + e->skyColourBottom[2]) / 2,
                               255));
    get_time_snapshot(PP_BG, DEBUG_SNAPSHOT_1_END);
}

tstatic void render_sky_gradient(Environment *e) {
    DEBUG_SNAPSHOT_1();
    if (sRenderSkyBlock == NULL) {
        sRenderSkyBlock = sky_gradient_generate(e);
    }
    rspq_block_run(sRenderSkyBlock);
    get_time_snapshot(PP_BG, DEBUG_SNAPSHOT_1_END);
}

tstatic void render_sky_texture(Environment *e) {
    DEBUG_SNAPSHOT_1();
    e->skyTimer = 10;
    if (e->texGen == false) {
        sky_texture_generate(e);
        e->texGen = true;
    }
    Matrix mtx;
    mtx_translate(&mtx, gCamera->pos[0], gCamera->pos[1] + 225.0f, gCamera->pos[2]);
    t3d_mat4_to_fixed_3x4(e->skyMtx, (T3DMat4  *) &mtx);
    t3d_segment_set(SEGMENT_MATRIX, e->skyMtx);
    material_mode(MAT_CI);
    // run block
    int base = gCamera->yaw + 0x8000;
    rspq_block_run(e->skyInit);
    for (int i = 0; i < 32; i++) {
        short rot = base - ((0x10000 / 16) * i);
        if (fabs(rot) >= 0x4000) {
            continue;
        }
        rspq_block_run(e->skySegment[i]);
    }
    t3d_matrix_pop(1);
    get_time_snapshot(PP_BG, DEBUG_SNAPSHOT_1_END);
}

static inline void render_end(void) {
    rspq_block_run(sRenderEndBlock);
    rdpq_set_scissor(0, 0, display_get_width(), display_get_height());
}

#define SHAD_SIZ 16

tstatic void render_shadow(float pos[3], float height) {
    T3DVertPacked* vertices = render_alloc(sizeof(T3DVertPacked) * 2, DRAW_MISC);
    T3DMat4FP *mtx = (T3DMat4FP *) render_alloc(sizeof(T3DMat4FP), DRAW_MISC);
    vertices[0] = (T3DVertPacked){
        .posA = {SHAD_SIZ, 0, -SHAD_SIZ}, .stA[0] = 2048, .stA[1] = 0,
        .posB = {-SHAD_SIZ, 0, -SHAD_SIZ}, .stB[0] = 0, .stB[1] = 0,
    };
    vertices[1] = (T3DVertPacked){
        .posA = {-SHAD_SIZ, 0, SHAD_SIZ}, .stA[0] = 0, .stA[1] = 2048,
        .posB = {SHAD_SIZ, 0, SHAD_SIZ}, .stB[0] = 2048, .stB[1] = 2048,
    };
    
    t3d_mat4fp_from_srt_euler(mtx, (float[3]){1.0f, 1.0f, 1.0f}, (float[3]){0, 0, 0}, (float[3]){pos[0], height + 1.0f, pos[2]});
    rdpq_set_prim_color(RGBA32(0, 0, 0, 127));
    data_cache_hit_writeback(vertices, sizeof(T3DVertPacked) * 2);
    data_cache_hit_writeback(mtx, sizeof(T3DMat4FP));
    t3d_matrix_push(mtx);
    t3d_vert_load(vertices, 0, 4);
    t3d_tri_draw(0, 1, 2);
    t3d_tri_draw(2, 3, 0);
    t3d_tri_sync();
    t3d_matrix_pop(1);
}

tstatic void render_aa(int mode) {
    switch (gConfig.graphics) {
    case G_PERFORMANCE:
        rdpq_mode_antialias(AA_NONE);
        break;
    case G_DEFAULT:
        if (mode == AA_ACTOR) {
            goto mrFancyPants;
        }
        rdpq_mode_antialias(AA_REDUCED);
        break;
    case G_BEAUTIFUL:
        mrFancyPants:
        rdpq_mode_antialias(AA_STANDARD);
        break;
    }
}

tstatic void render_ztarget_scissor(void) {
    int targetPos;
    if (gConfig.regionMode == TV_PAL) {
        targetPos = gZTargetTimer * (1.5f * 1.2f);
    } else {
        targetPos = gZTargetTimer * 1.5f;
    }
    rdpq_set_scissor(0, targetPos, display_get_width(), display_get_height() - (targetPos));
}

tstatic void render_set_modes(void) {
    gPrevMaterialID = -1;
    gPrevRenderFlags = -1;
    rspq_block_run(sBeginModeBlock);
    if (gConfig.graphics == G_BEAUTIFUL) {
        *(volatile uint32_t*)0xA4400000 |= 0x10000;
    } else {
        *(volatile uint32_t*)0xA4400000 &= ~0x10000;
    }
}

static RenderNode *sPrevEntry;

tstatic void find_material_list(RenderNode *node, int layer) {
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
        sPrevEntry = node;
        return;
    }
    RenderList *matList;
    if (gRenderNodeHead[layer] == NULL) {
        gRenderNodeHead[layer] = node;
        matList = (RenderList *) render_alloc(sizeof(RenderList), layer);
        gMateriallistHead[layer] = matList;
        sPrevEntry = node;
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
                sPrevEntry = node;
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

tstatic void add_render_node(RenderNode *entry, rspq_block_t *block, Material *material, int flags, int layer) {
    DEBUG_SNAPSHOT_1();
    entry->block = block;
    entry->material = material;
    entry->flags = flags;
    entry->skel = NULL;
    find_material_list(entry, layer);
    get_time_snapshot(PP_BATCH, DEBUG_SNAPSHOT_1_END);
}

tstatic void layer_reset(int layer) {
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

tstatic void pop_render_list(int layer) {
    if (gRenderNodeHead[layer] == NULL) {
        return;
    }
    static color_t prevPrim;
    RenderNode *renderList = gRenderNodeHead[layer];
    while (renderList) {
        if (renderList->matrix) {
            t3d_segment_set(SEGMENT_MATRIX, renderList->matrix);
        }
        if (renderList->material) {
            material_set(renderList->material, renderList->flags);
        }

        if (color_to_packed32(renderList->primColour) != color_to_packed32(prevPrim)) {
            rdpq_set_prim_color(renderList->primColour);
            prevPrim = renderList->primColour;
        }
        //rdpq_set_env_color(renderList->envColour);
        if (renderList->skel) {
            t3d_segment_set(SEGMENT_BONES, renderList->skel);
        }
        rspq_block_run(renderList->block);
        renderList = renderList->next;
    }
    layer_reset(layer);
}

tstatic void render_particles(void) {
    DEBUG_SNAPSHOT_1();
    ParticleList *list = gParticleListHead;
    Particle *particle;

#if OPENGL
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
#endif
    //set_particle_render_settings();
    gRenderSettings.depthRead = true;
    
    while (list) {
        particle = list->particle;
        ////MATRIX_PUSH();
        /*if (particle->material) {
            material_set(particle->material, 0);
        }*/
        Matrix mtx;
        mtx_billboard(&mtx, particle->pos[0], particle->pos[1], particle->pos[2]);
        mtx_scale(&mtx, particle->scale[0], particle->scale[1], particle->scale[2]);
        //MATRIX_MUL(mtx.m, 0, 0);
        rspq_block_run(sParticleBlock);
        //MATRIX_POP();
        list = list->next;
    }
    get_time_snapshot(PP_RENDERPARTICLES, DEBUG_SNAPSHOT_1_END);
}

tstatic inline int render_inside_view_world(float width, float screenPos[3]) {
    float hScreenEdge = -screenPos[2] * gHalfFovHor;
    if (fabsf(screenPos[0]) > hScreenEdge + width) {
        return false;
    }
    if (screenPos[2] - VALIDDEPTHMIDDLE >= VALIDDEPTHRANGE + width) {
        return false;
    }
    return true;
}

tstatic int render_world_visible(SceneChunk *c) {
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

tstatic void render_world(void) {
    DEBUG_SNAPSHOT_1();
    int i = 0;
    if (gCurrentScene && gCurrentScene->model) {
        SceneChunk *s = gCurrentScene->chunkList;

        while (s != NULL) {
            if (gScreenshotStatus != SCREENSHOT_SHOW && render_world_visible(s)) {
                SceneMesh *c = s->meshList;
                i++;
                while (c != NULL) {
                    if (c->flags & MESH_INVISIBLE) {
                        s->flags |= CHUNK_HAS_MODEL;
                        c->material = material_init(c->materialID);
                        c = c->next;
                        continue;
                    }
                    int layer;

                    if (c->renderBlock == NULL) {
                        scene_generate_chunk(c);
                    }

                    if (c->material->flags & MAT_INVISIBLE) {
                        c = c->next;
                        continue;
                    }

                    if ((c->material->flags & MAT_DEPTH_READ) == false) {
                        layer = DRAW_NZB;
                    } else if (c->material->flags & MAT_DECAL) {
                        layer = DRAW_DECAL;
                    } else if (c->material->flags & MAT_XLU) {
                        layer = DRAW_XLU;
                    } else {
                        layer = DRAW_OPA;
                    }

                    RenderNode *entry = (RenderNode *) render_alloc(sizeof(RenderNode), layer);
                    entry->matrix = NULL;
                    entry->primColour = c->primC;
                    //Material *mat = gUseOverrideMaterial ? &gOverrideMaterial : c->material;
                    add_render_node(entry, c->renderBlock, c->material, MAT_NULL, layer);
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
    //debugf("Chunks visible: %d\n", i);
    pop_render_list(DRAW_NZB);
    pop_render_list(DRAW_OPA);
    pop_render_list(DRAW_DECAL);
    get_time_snapshot(PP_RENDERLEVEL, DEBUG_SNAPSHOT_1_END);
    profiler_wait();
}

tstatic void shadow_render(void) {
    profiler_wait();
    DEBUG_SNAPSHOT_1();
    ObjectList *list = gObjectListHead;
    Object *obj;

    
    material_set(gBlobShadowMat, MAT_DECAL | MAT_XLU | MAT_DEPTH_READ);
    while (list) {
        obj = list->obj;
        if (obj->flags & OBJ_FLAG_IN_VIEW && (obj->flags & OBJ_FLAG_SHADOW || (gConfig.graphics == G_PERFORMANCE && obj->flags & OBJ_FLAG_SHADOW_DYNAMIC))) { 
            float height;
            if (obj->collision) {
                height = obj->collision->floorHeight;
                if (obj->pos[1] - height > 10.0f) {
                    goto next;
                }
            } else {
                height = obj->pos[1];
            }
            render_shadow(obj->pos, height);
        }
        next:
        list = list->next;
    }

    if (gConfig.graphics == G_PERFORMANCE) {
        get_time_snapshot(PP_SHADOWS, DEBUG_SNAPSHOT_1_END);
        profiler_wait();
        return;
    }
    list = gObjectListHead;
    while (list) {
        obj = list->obj;
        if (obj->flags & OBJ_FLAG_SHADOW_DYNAMIC && obj->flags & OBJ_FLAG_IN_VIEW && obj->gfx && obj->gfx->dynamicShadow) {
            DynamicShadow *d = obj->gfx->dynamicShadow;
            Matrix mtxF;
            d->staleTimer = 10;
            float floorHeight = MAX(obj->collision->floorHeight, obj->collision->hitboxHeight);
            float pos[3] = {obj->pos[0], floorHeight + 1.0f, obj->pos[2]};
            float scale[3] = {obj->scale[0] * 1.66f, 1.0f, obj->scale[2] * 2.25f};
            unsigned short angle[3] = {0, d->angle[1] + 0x4000, 0};
            set_draw_matrix(&mtxF, MTX_TRANSLATE_ROTATE_SCALE, pos, angle, scale);
            t3d_mat4_to_fixed_3x4(d->mtx[1], (T3DMat4  *) &mtxF);
            t3d_segment_set(SEGMENT_MATRIX, d->mtx[1]);
            rspq_block_run(d->block);
        }
        list = list->next;
    }
    get_time_snapshot(PP_SHADOWS, DEBUG_SNAPSHOT_1_END);
    profiler_wait();
}

tstatic void render_clutter(void) {
    if (gConfig.graphics == G_PERFORMANCE) {
        return;
    }
    DEBUG_SNAPSHOT_1();
    ClutterList *list = gClutterListHead;
    Clutter *obj; 
    while (list) {
        obj = list->clutter;
        if (obj->flags & OBJ_FLAG_IN_VIEW && !(obj->flags & OBJ_FLAG_INVISIBLE)) {
            ObjectModel *m = obj->gfx->listEntry->entry;
            obj->gfx->listEntry->timer = 10;
            if (obj->gfx->listEntry->active == false) {
                clutter_model_generate(obj);
            }
            int layer;
            if (m->material->flags & MAT_DECAL || m->material->flags & MAT_XLU) {
                layer = DRAW_XLU;
            } else {
                layer = DRAW_OPA;
            }
            RenderNode *entry = (RenderNode *) render_alloc(sizeof(RenderNode), layer);
            entry->matrix = obj->matrix;
            if (m->matrixBehaviour == MTX_BILLBOARD || m->matrixBehaviour == MTX_BILLBOARD_SCALE) {
                Matrix mtx;
                //data_cache_hit_invalidate(&obj->matrix, sizeof(T3DMat4FP));
                //entry->matrix = UncachedAddr(&obj->matrix);
                set_draw_matrix(&mtx, m->matrixBehaviour, obj->pos, obj->faceAngle, obj->scale);
                t3d_mat4_to_fixed_3x4(obj->matrix, (T3DMat4  *) &mtx);
            }
            entry->primColour = m->colour;
            add_render_node(entry, m->block, m->material, MAT_NULL, layer);
        }
        list = list->next;
    }
    pop_render_list(DRAW_OPA);
    get_time_snapshot(PP_RENDERCLUTTER, DEBUG_SNAPSHOT_1_END);
    profiler_wait();
}

tstatic void render_objects(float updateRateF) {
    profiler_wait();
    DEBUG_SNAPSHOT_1();
    ObjectList *list = gObjectListHead;
    Object *obj;
    
    while (list) {
        obj = list->obj;
        if (obj->flags & OBJ_FLAG_IN_VIEW && !(obj->flags & OBJ_FLAG_INVISIBLE) && obj->gfx) {
            ObjectModel *m = obj->gfx->listEntry->entry;
            T3DMat4FP *prevMtx = NULL;
            obj->gfx->listEntry->timer = 10;
            if (obj->gfx->listEntry->active == false) {
                object_model_generate(obj);
            }
            while (m) {
                int layer;
                if (m->material->flags & MAT_DECAL || m->material->flags & MAT_XLU) {
                    layer = DRAW_XLU;
                } else {
                    layer = DRAW_OPA;
                }
                RenderNode *entry = (RenderNode *) render_alloc(sizeof(RenderNode), layer);
                if (m->matrixBehaviour != MTX_NONE) {
                    Matrix tempMtx;
                    entry->matrix = (T3DMat4FP *) render_alloc(sizeof(T3DMat4FP) + 0x10, layer);
                    entry->matrix = (T3DMat4FP *) (((int) entry->matrix + 0xF) & ~0xF);
                    data_cache_hit_invalidate(entry->matrix, sizeof(T3DMat4FP));
                    entry->matrix = UncachedAddr(entry->matrix);
                    set_draw_matrix(&tempMtx, m->matrixBehaviour, obj->pos, obj->faceAngle, obj->scale);
                    t3d_mat4_to_fixed_3x4(entry->matrix, (T3DMat4  *) &tempMtx);
                    prevMtx = entry->matrix;
                } else {
                    entry->matrix = prevMtx;
                }
                entry->primColour = m->colour;
                add_render_node(entry, m->block, m->material, MAT_NULL, layer);
                if (obj->animation) {
                    //entry->skel = obj->animation->skeleton.bufferCount == 1 ? obj->animation->skeleton.boneMatricesFP : t3d_segment_placeholder(T3D_SEGMENT_SKELETON);
                    entry->skel = obj->animation->skeleton[0].boneMatricesFP;
                }
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
T3DViewport gShadowViewport;

tstatic void reset_shadow_perspective(void) {
    if (sDynamicShadowBlock == NULL) {
        gShadowViewport = t3d_viewport_create();
        /*rspq_block_begin();
        material_mode(MAT_NULL);
        rdpq_set_mode_standard();
        rdpq_mode_antialias(AA_NONE);
        rdpq_mode_blender(0);
        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
        rdpq_set_prim_color(RGBA32(255, 255, 255, 255));
        sDynamicShadowBlock = rspq_block_end();*/
        //rspq_block_begin();
        //rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
        //rdpq_set_prim_color(RGBA32(255, 255, 255, 255));
        //material_mode(MAT_NULL);
        //t3d_state_set_drawflags(T3D_FLAG_CULL_BACK);
        sDynamicShadowBlock = (rspq_block_t *) 1;
        //sDynamicShadowBlock = rspq_block_end();
    }
    //rspq_block_run(sDynamicShadowBlock);
}

tstatic void shadow_update(void) {
    profiler_wait();
    DEBUG_SNAPSHOT_1();
    ObjectList *list = gObjectListHead;
    Object *obj;

    render_set_modes();
    float pos[3] = {0.0f, 0.0f, 0.0f};
    float scale[3] = {-1.0f, 1.0f, 1.0f};
    reset_shadow_perspective();
        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
        rdpq_set_prim_color(RGBA32(255, 255, 255, 255));
        material_mode(MAT_NULL);
        t3d_state_set_drawflags(T3D_FLAG_CULL_FRONT);
    
    while (list) {
        obj = list->obj;
        if (obj->header->dynamicShadow == NULL) {
            list = list->next;
            continue;
        }
        if (obj->flags & OBJ_FLAG_SHADOW_DYNAMIC && obj->flags & OBJ_FLAG_IN_VIEW && !(obj->flags & OBJ_FLAG_INVISIBLE) && obj->gfx) {
            if (obj->overlay == NULL) {
                obj_overlay_init(obj, obj->objectID);
            }
            if (obj->gfx->dynamicShadow == false) {
                shadow_generate(obj);
            }
            DynamicShadow *d = obj->gfx->dynamicShadow;
            
            rdpq_attach(&obj->gfx->dynamicShadow->surface, NULL);
            t3d_screen_clear_color(RGBA32(0, 0, 0, 0));
            t3d_viewport_attach(&gShadowViewport);
            t3d_viewport_set_area(&gShadowViewport, 0, 0, d->texW, d->texH);
            t3d_viewport_set_projection(&gShadowViewport, T3D_DEG_TO_RAD(85.0f), 10.0f, 500.0f);
            gShadowViewport.guardBandScale = 3;
            t3d_viewport_look_at(&gShadowViewport, (T3DVec3 *) obj->gfx->dynamicShadow->camPos, (T3DVec3 *) obj->gfx->dynamicShadow->camFocus, &(T3DVec3){{0,1,0}});
            obj->gfx->dynamicShadow->staleTimer = 10;
            ObjectModel *m = obj->gfx->listEntry->entry;
            if (obj->gfx->listEntry->active == false) {
                object_model_generate(obj);
            }
            int numMats = 0;
            while (m) {
                if (m->matrixBehaviour != MTX_NONE) {
                    Matrix matrix;
                    unsigned short angle[3] = {obj->faceAngle[0], -obj->faceAngle[1] + 0x8000 + obj->gfx->dynamicShadow->angle[1], obj->faceAngle[2]};
                    set_draw_matrix(&matrix, m->matrixBehaviour, pos, angle, scale);
                    t3d_mat4_to_fixed_3x4(obj->gfx->dynamicShadow->mtx[0], (T3DMat4  *) &matrix);
                    t3d_segment_set(SEGMENT_MATRIX, obj->gfx->dynamicShadow->mtx[0]);
                    numMats++;
                }
                
                if (obj->animation) {
                    t3d_segment_set(SEGMENT_BONES, obj->animation->skeleton[0].boneMatricesFP);
                }
                rspq_block_run(m->block);
                m = m->next;
            }
            rdpq_detach();
        }
        list = list->next;
    }
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

tstatic void render_visibility(void) {
    DEBUG_SNAPSHOT_1();
    ObjectList *list = gObjectListHead;
    Object *obj;

    while (list) {
        obj = list->obj;
        //obj->flags |= OBJ_FLAG_IN_VIEW;
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

    //return;

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
        shadow_update();
    }
    if (gScreenshotStatus > SCREENSHOT_NONE) {
        screenshot_generate();
    } else {
        rdpq_attach(gFrameBuffers, display_get_zbuf());
    }
    sPrevT3dFlags = 0;
    t3d_screen_clear_depth();
    t3d_viewport_attach(&gViewport);
    if (gScreenshotStatus != SCREENSHOT_SHOW) {
        rdpq_mode_antialias(AA_NONE);
        render_ztarget_scissor();
        camera_project();
        if (gConfig.graphics == G_PERFORMANCE) {
            render_sky_flat(gEnvironment);
        } else if (gEnvironment->skyboxTextureID == -1) {
            render_sky_gradient(gEnvironment);
        } else {
            render_sky_texture(gEnvironment);
        }
        render_set_modes();
        light_apply(lightNeutral);
        render_visibility();
        render_aa(AA_GEO);
        render_world();
        shadow_render();
        render_clutter();
        render_aa(AA_ACTOR);
        render_objects(updateRateF);
        render_aa(AA_GEO);
        render_particles();
        render_end();
        if (gScreenshotStatus == SCREENSHOT_GENERATE) {
            clear_dynamic_shadows();
        }
    } else if (gScreenshotStatus == SCREENSHOT_SHOW) {
        //render_world();
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
    layer_reset(DRAW_MISC);
    get_time_snapshot(PP_RENDER, DEBUG_SNAPSHOT_1_END);
    if (gScreenshotStatus <= SCREENSHOT_NONE) {
        render_hud(updateRate, updateRateF);
        render_menus(updateRate, updateRateF);
    }
    rspq_wait();
#ifdef PUPPYPRINT_DEBUG
    offset = gDebugData->timer[PP_HALT][gDebugData->iteration] - offset;
    add_time_offset(PP_RENDER, offset);
#endif
}