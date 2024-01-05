#include <libdragon.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>
#include <malloc.h>

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

float gAspectRatio = 1.0f;
RenderNode *gRenderNodeHead = NULL;
RenderNode *gRenderNodeTail = NULL;
RenderList *gMateriallistHead = NULL;
RenderList *gMateriallistTail = NULL;
RenderList *gPrevMatList = NULL;
Matrix gBillboardMatrix;
Matrix gScaleMatrix;
char gZTargetTimer = 0;
char gUseOverrideMaterial = false;
static rspq_block_t *sRenderEndBlock;
rspq_block_t *sRenderSkyBlock;
static rspq_block_t *sBeginModeBlock;
static rspq_block_t *sParticleBlock;
Material gOverrideMaterial;

Material gTempMaterials[] = {
    {NULL, 0, MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_VTXCOL},
    {NULL, 2, MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_CUTOUT | MATERIAL_VTXCOL},
    {NULL, 3, MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_XLU | MATERIAL_VTXCOL},
};

light_t lightNeutral = {
    color: { 0.66f, 0.66f, 0.66f, 0.66f},
    diffuse: {1.0f, 1.0f, 1.0f, 1.0f},
    direction: {0, 0, 0x6000},
    position: {1.0f, 0.0f, 0.0f, 0.0f},
    radius: 10.0f,
};


static void init_particles(void) {
    rspq_block_begin();
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
    sParticleBlock = rspq_block_end();
}

void init_renderer(void) {
    setup_light(lightNeutral);
    init_materials();
    init_particles();

    rspq_block_begin();
    glAlphaFunc(GL_GREATER, 0.5f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_LESS);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glShadeModel(GL_SMOOTH);
    glDepthMask(GL_TRUE);
    glEnable(GL_SCISSOR_TEST);
    sBeginModeBlock = rspq_block_end();

    rspq_block_begin();
    glDisable(GL_MULTISAMPLE_ARB);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glScissor(0, 0, display_get_width(), display_get_height());
    sRenderEndBlock = rspq_block_end();
}

void setup_light(light_t light) {
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, light.color);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light.diffuse);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 2.0f/light.radius);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 1.0f/(light.radius*light.radius));

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, light.diffuse);
}

void set_frustrum(float l, float r, float b, float t, float n, float f) {
    DEBUG_MATRIX_OP();
    Matrix frustum = (Matrix) { .m={
        {(2*n)/(r-l), 0.f, 0.f, 0.f},
        {0.f, (2.f*n)/(t-b), 0.f, 0.f},
        {(r+l)/(r-l), (t+b)/(t-b), -(f+n)/(f-n), -1.f},
        {0.f, 0.f, -(2*f*n)/(f-n), 0.f},
    }};
    glMultMatrixf(frustum.m[0]);
}

void lookat_normalise(GLfloat *d, const GLfloat *v) {
    float inv_mag = 1.0f / sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    d[0] = v[0] * inv_mag;
    d[1] = v[1] * inv_mag;
    d[2] = v[2] * inv_mag;
}

void lookat_cross(GLfloat* p, const GLfloat* a, const GLfloat* b) {
    p[0] = (a[1] * b[2] - a[2] * b[1]);
    p[1] = (a[2] * b[0] - a[0] * b[2]);
    p[2] = (a[0] * b[1] - a[1] * b[0]);
};

float lookat_dot(const float *a, const float *b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void mtx_lookat(float eyex, float eyey, float eyez, float centerx, float centery, float centerz, float upx, float upy, float upz) {
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

    glMultMatrixf(&m[0][0]);
};

void mtx_translate_rotate(Matrix *mtx, short angleX, short angleY, short angleZ, GLfloat x, GLfloat y, GLfloat z) {
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

void mtx_translate(Matrix *mtx, float x, float y, float z) {
    DEBUG_MATRIX_OP();
    bzero(mtx, sizeof(Matrix));

    mtx->m[3][0] = x;
    mtx->m[3][1] = y;
    mtx->m[3][2] = z;
    mtx->m[3][3] = 1.0f;
}

void mtx_rotate(Matrix *mtx, short angleX, short angleY, short angleZ) {
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

void mtx_billboard(Matrix *mtx, float x, float y, float z) {
    DEBUG_MATRIX_OP();
    gBillboardMatrix.m[3][0] = x;
    gBillboardMatrix.m[3][1] = y;
    gBillboardMatrix.m[3][2] = z;

    memcpy(mtx, &gBillboardMatrix, sizeof(Matrix));
}

void mtx_scale(Matrix *mtx, float scaleX, float scaleY, float scaleZ) {
    DEBUG_MATRIX_OP();
    float s[3] = {scaleX, scaleY, scaleZ};
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            mtx->m[i][j] *= s[i];
        }
    }
}

void set_draw_matrix(Matrix *mtx, int matrixType, float *pos, u_uint16_t *angle, float *scale) {
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

void set_light(light_t light) {
    glPushMatrix();
    Matrix mtx;
    mtx_rotate(&mtx, light.direction[0], light.direction[1], light.direction[2]);
    glMultMatrixf(mtx.m[0]);
    glLightfv(GL_LIGHT0, GL_POSITION, light.position);
    glPopMatrix();
}

void project_camera(void) {
    Camera *c = gCamera;
    float nearClip = 5.0f;
    float farClip = 500.0f;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gAspectRatio = (float) display_get_width() / (float) (display_get_height());
    set_frustrum(-nearClip * gAspectRatio, nearClip * gAspectRatio, -nearClip, nearClip, nearClip, farClip);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    mtx_lookat(c->pos[0], c->pos[1], c->pos[2], c->focus[0], c->focus[1], c->focus[2], 0.0f, 1.0f, 0.0f);
}

void render_sky(void) {
    Environment *e = gEnvironment;
    if (sRenderSkyBlock == NULL) {
        int width = display_get_width();
        int height = display_get_height();
        rspq_block_begin();
        glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION);
        glDisable(GL_MULTISAMPLE_ARB);
        glLoadIdentity();
        glOrtho(0.0f, width, height, 0.0f, -1.0f, 1.0f);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glBegin(GL_QUADS);
        glColor3f(e->skyColourTop[0], e->skyColourTop[1], e->skyColourTop[2]);
        glVertex2i(0, 0);
        glColor3f(e->skyColourBottom[0], e->skyColourBottom[1], e->skyColourBottom[2]);
        glVertex2i(0, height);
        glVertex2i(width, height);
        glColor3f(e->skyColourTop[0], e->skyColourTop[1], e->skyColourTop[2]);
        glVertex2i(width, 0);
        glColor3f(1, 1, 1);
        glEnd();
        sRenderSkyBlock = rspq_block_end();
    }
    rspq_block_run(sRenderSkyBlock);
}

void render_bush(void) {
    Environment *e = gEnvironment;
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
}

void render_end(void) {
    rspq_block_run(sRenderEndBlock);
}

rspq_block_t *sBushBlock;
rspq_block_t *sShadowBlock;

void render_shadow(float pos[3]) {
    glPushMatrix();
    glTranslatef(pos[0], pos[1], pos[2]);
    if (sShadowBlock == NULL) {
        rspq_block_begin();
        glScalef(0.5f, 0.5f, 0.5f);
        glBegin(GL_QUADS);
        glColor3f(0, 0, 0);
        glTexCoord2f(2.048f, 0);        glVertex3f(5.0f, 0.0f, -5.0f);
        glTexCoord2f(0, 0);             glVertex3f(-5.0f, 0.0f, -5.0f);
        glTexCoord2f(0, 2.048f);        glVertex3f(-5.0f, 0.0f, 5.0f);
        glTexCoord2f(2.048f, 2.048f);   glVertex3f(5.0f, 0.0f, 5.0f);
        glEnd();
        sShadowBlock = rspq_block_end();
    }
    rspq_block_run(sShadowBlock);
    glPopMatrix();
}

void apply_anti_aliasing(int mode) {
    switch (gConfig.antiAliasing) {
    case AA_OFF:
        glDisable(GL_MULTISAMPLE_ARB);
        rdpq_mode_antialias(AA_NONE);
        break;
    case AA_FAST:
        if (mode == AA_ACTOR) {
            goto mrFancyPants;
        }
        glEnable(GL_MULTISAMPLE_ARB);
        glHint(GL_MULTISAMPLE_HINT_N64, GL_FASTEST);
        rdpq_mode_antialias(AA_REDUCED);
        break;
    case AA_FANCY:
        mrFancyPants:
        glEnable(GL_MULTISAMPLE_ARB);
        glHint(GL_MULTISAMPLE_HINT_N64, GL_NICEST);
        rdpq_mode_antialias(AA_STANDARD);
        break;
    }
}

void apply_render_settings(void) {
    int targetPos;
    rspq_block_run(sBeginModeBlock);
    if (gConfig.regionMode == TV_PAL) {
        targetPos = gZTargetTimer * (1.5f * 1.2f);
    } else {
        targetPos = gZTargetTimer * 1.5f;
    }
    glScissor(0, targetPos, display_get_width(), display_get_height() - (targetPos * 2));
    if (gConfig.dedither) {
        *(volatile uint32_t*)0xA4400000 |= 0x10000;
    } else {
        *(volatile uint32_t*)0xA4400000 &= ~0x10000;
    }
}

#ifdef PUPPYPRINT_DEBUG
int showAll = 1;
#else
#define showAll 1
#endif

void find_material_list(RenderNode *node) {
    // idk if this section is faster, yet.
    if (gPrevMatList && node->material->textureID == gPrevMatList->entryHead->material->textureID) {
        RenderList *list = gPrevMatList;
        if (list->entryHead == gRenderNodeHead) {
            gRenderNodeHead = node;
        } else {
            list->entryHead->prev->next = node;
        }
        node->next = list->entryHead;
        node->prev = list->entryHead->prev;
        list->entryHead = node;
        gPrevMatList = list;
        return;
    }
    // -----
    RenderList *matList;
    if (gRenderNodeHead == NULL) {
        gRenderNodeHead = node;
        matList = malloc(sizeof(RenderList));
        gMateriallistHead = matList;
    } else {
        RenderList *list = gMateriallistHead;
        while (list) {
            if (list->entryHead->material->textureID == node->material->textureID) {
                if (list->entryHead == gRenderNodeHead) {
                    gRenderNodeHead = node;
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
        matList = malloc(sizeof(RenderList));
        gMateriallistTail->next = matList;
        gRenderNodeTail->next = node;
    }
    node->next = NULL;
    matList->entryHead = node;
    matList->next = NULL;
    gMateriallistTail = matList;
    node->prev = gRenderNodeTail;
    gRenderNodeTail = node;
    gPrevMatList = matList;
}

void add_render_node(RenderNode *entry, rspq_block_t *block, Material *material, int flags) {
    DEBUG_SNAPSHOT_1();
    entry->block = block;
    entry->material = material;
    entry->flags = flags;
    find_material_list(entry);
    get_time_snapshot(PP_BATCH, DEBUG_SNAPSHOT_1_END);
}

void pop_render_list(void) {
    RenderNode *renderList = gRenderNodeHead;
    while (renderList) {
        RenderNode *oldList = renderList;
        glPushMatrix();
        if (renderList->matrix) {
            glMultMatrixf((GLfloat *) renderList->matrix->m);
            free(renderList->matrix);
        }
        if (renderList->material) {
            set_material(renderList->material, renderList->flags);
        }
        rspq_block_run(renderList->block);
        glPopMatrix();
        renderList = renderList->next;
        free(oldList);
    }
    RenderList *matList = gMateriallistHead;
    while (matList) {
        RenderList *oldList = matList;
        matList = matList->next;
        free(oldList);
    }
    gRenderNodeHead = NULL;
    gRenderNodeTail = NULL;
    gMateriallistHead = NULL;
    gMateriallistTail = NULL;
    gPrevMatList = NULL;
}

void render_particles(void) {
    DEBUG_SNAPSHOT_1();
    ParticleList *list = gParticleListHead;
    Particle *particle;
    
    while (list) {
        particle = list->particle;
        glPushMatrix();
        if (particle->material) {
            set_texture(particle->material);
        }
        Matrix mtx;
        mtx_billboard(&mtx, particle->pos[0], particle->pos[1], particle->pos[2]);
        mtx_scale(&mtx, particle->scale[0], particle->scale[1], particle->scale[2]);
        glMultMatrixf((GLfloat *) mtx.m);
        rspq_block_run(sParticleBlock);
        glPopMatrix();
        list = list->next;
    }
    get_time_snapshot(PP_RENDERPARTICLES, DEBUG_SNAPSHOT_1_END);
}

void render_world(void) {
    DEBUG_SNAPSHOT_1();
    if (sCurrentScene && sCurrentScene->model) {
        SceneMesh *s = sCurrentScene->meshList;

        while (s != NULL) {
            RenderNode *entry = malloc(sizeof(RenderNode));
            entry->matrix = NULL;
            if (showAll) {
                Material *mat = gUseOverrideMaterial ? &gOverrideMaterial : s->material;
                add_render_node(entry, s->renderBlock, mat, s->flags);
            } else {
                set_material(s->material, s->flags);
                rspq_block_run(s->renderBlock);
                free(entry);
            }
            s = s->next;
        }
    }
    pop_render_list();
    get_time_snapshot(PP_RENDERLEVEL, DEBUG_SNAPSHOT_1_END);
#ifdef PUPPYPRINT_DEBUG
    if (gDebugData && gDebugData->enabled) {
        rspq_wait();
    }
#endif
}

void render_object_shadows(void) {
    DEBUG_SNAPSHOT_1();
    ObjectList *list = gObjectListHead;
    Object *obj;
    
    set_material(&gTempMaterials[2], MATERIAL_DECAL);
    while (list) {
        obj = list->obj;
        if (obj->flags & OBJ_FLAG_SHADOW) { 
            render_shadow(obj->pos);
        }
        list = list->next;
    }
    get_time_snapshot(PP_SHADOWS, DEBUG_SNAPSHOT_1_END);
}

void render_clutter(void) {
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
        if (obj->objectID == CLUTTER_BUSH && !(obj->flags & OBJ_FLAG_INVISIBLE)) {
            RenderNode *entry = malloc(sizeof(RenderNode));
            entry->matrix = malloc(sizeof(Matrix));
            mtx_billboard(entry->matrix, obj->pos[0], obj->pos[1], obj->pos[2]);
            if (showAll) {
                Material *mat = gUseOverrideMaterial ? &gOverrideMaterial : &gTempMaterials[1];
                add_render_node(entry, sBushBlock, mat, MATERIAL_NULL);
            } else {
                glPushMatrix();
                glMultMatrixf((GLfloat *) entry->matrix->m);
                set_material(&gTempMaterials[1], MATERIAL_NULL);
                rspq_block_run(sBushBlock);
                free(entry->matrix);
                free(entry);
                glPopMatrix();
            }
        } else if (obj->objectID == CLUTTER_ROCK && !(obj->flags & OBJ_FLAG_INVISIBLE)) {
            RenderNode *entry = malloc(sizeof(RenderNode));
            entry->matrix = malloc(sizeof(Matrix));
            mtx_billboard(entry->matrix, obj->pos[0], obj->pos[1], obj->pos[2]);
            if (showAll) {
                Material *mat = gUseOverrideMaterial ? &gOverrideMaterial : &gTempMaterials[0];
                add_render_node(entry, sBushBlock, mat, MATERIAL_NULL);
            } else {
                glPushMatrix();
                glMultMatrixf((GLfloat *) entry->matrix->m);
                set_material(&gTempMaterials[0], MATERIAL_NULL);
                rspq_block_run(sBushBlock);
                free(entry->matrix);
                free(entry);
                glPopMatrix();
            }
        }
        list = list->next;
    }
    pop_render_list();
    get_time_snapshot(PP_RENDERCLUTTER, DEBUG_SNAPSHOT_1_END);
#ifdef PUPPYPRINT_DEBUG
    if (gDebugData && gDebugData->enabled) {
        rspq_wait();
    }
#endif
}

void render_objects(void) {
    DEBUG_SNAPSHOT_1();
    ObjectList *list = gObjectListHead;
    Object *obj;
    
    while (list) {
        obj = list->obj;
        if (obj->gfx) { 
            ObjectModel *m = obj->gfx->listEntry->entry;
            Matrix *prevMtx = NULL;
            while (m) {
                RenderNode *entry = malloc(sizeof(RenderNode));
                if (m->matrixBehaviour != MTX_NONE) {
                    entry->matrix = malloc(sizeof(Matrix));
                    prevMtx = entry->matrix;
                    set_draw_matrix(entry->matrix, m->matrixBehaviour, obj->pos, obj->faceAngle, obj->scale);
                } else {
                    if (prevMtx) {
                        entry->matrix = malloc(sizeof(Matrix));
                        memcpy(entry->matrix, prevMtx, sizeof(Matrix));
                    } else {
                        entry->matrix = NULL;
                    }
                }
                if (showAll) {
                    Material *mat = gUseOverrideMaterial ? &gOverrideMaterial : &m->material;
                    add_render_node(entry, m->block, mat, MATERIAL_NULL);
                } else {
                    glPushMatrix();
                    glMultMatrixf((GLfloat *) entry->matrix->m);
                    set_material(&m->material, MATERIAL_NULL);
                    rspq_block_run(m->block);
                    free(entry->matrix);
                    free(entry);
                    glPopMatrix();
                }
                m = m->next;
            }
        }
        list = list->next;
    }

    pop_render_list();
    get_time_snapshot(PP_RENDEROBJECTS, DEBUG_SNAPSHOT_1_END);
#ifdef PUPPYPRINT_DEBUG
    if (gDebugData && gDebugData->enabled) {
        rspq_wait();
    }
#endif
}

void render_game(int updateRate, float updateRateF) {
    DEBUG_SNAPSHOT_1();
    if (gScreenshotStatus > SCREENSHOT_NONE) {
        screenshot_generate();
    } else {
        rdpq_attach_clear(gFrameBuffers, &gZBuffer);
    }
    gl_context_begin();
    glClear(GL_DEPTH_BUFFER_BIT);
    if (gScreenshotStatus != SCREENSHOT_SHOW) {
        render_sky();
        apply_anti_aliasing(AA_GEO);
        project_camera();
        apply_render_settings();
        set_light(lightNeutral);
        render_world();
        render_object_shadows();
        render_clutter();
        apply_anti_aliasing(AA_ACTOR);
        render_objects();
        apply_anti_aliasing(AA_GEO);
        set_particle_render_settings();
        render_particles();
        render_end();
        gl_context_end();
    } else if (gScreenshotStatus == SCREENSHOT_SHOW) {
        render_end();
        gl_context_end();
        rdpq_set_mode_copy(false);
        rdpq_tex_blit(&gScreenshot, 0, 0, NULL);
        rdpq_set_mode_standard();
    }
    get_time_snapshot(PP_RENDER, DEBUG_SNAPSHOT_1_END);
    if (gScreenshotStatus <= SCREENSHOT_NONE) {
        render_hud(updateRate, updateRateF);
        render_menus(updateRate, updateRateF);
    }
#ifdef PUPPYPRINT_DEBUG
    if (get_input_pressed(INPUT_CRIGHT, 0)) {
        showAll ^= 1;
    }
#endif
}