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

float gAspectRatio = 1.0f;
static rspq_block_t *sRenderEndBlock;
static rspq_block_t *sRenderSkyBlock;
static rspq_block_t *sBeginModeBlock;
static rspq_block_t *sParticleBlock;
char gZTargetTimer = 0;

Material gTempMaterials[] = {
    {NULL, 0, MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_VTXCOL},
    {NULL, -1, MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_LIGHTING | MATERIAL_VTXCOL},
    {NULL, 1, MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_XLU},
    {NULL, 2, MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_CUTOUT | MATERIAL_VTXCOL},
    {NULL, 1, MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_CUTOUT | MATERIAL_VTXCOL},
    {NULL, 3, MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_XLU | MATERIAL_VTXCOL},
};

static model64_t *gPlayerModel;

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
    glVertex3i(-5, 5, 0);
    glTexCoord2f(0, 1.024f);
    glVertex3i(-5, -5, 0);
    glTexCoord2f(1.024f, 1.024f);
    glVertex3i(5, -5, 0);
    glTexCoord2f(1.024f, 0);
    glVertex3i(5, 5, 0);
    glEnd();
    sParticleBlock = rspq_block_end();
}

void init_renderer(void) {
    setup_light(lightNeutral);
    init_materials();
    init_particles();
    gPlayerModel = model64_load(asset_dir("humanoid", DFS_MODEL64));

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

Matrix gBillboardMatrix;
Matrix gScaleMatrix;

void set_frustrum(float l, float r, float b, float t, float n, float f) {
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

void mtx_translate_rotate(short angleX, short angleY, short angleZ, GLfloat x, GLfloat y, GLfloat z) {
    float sx = sins(angleX);
    float cx = coss(angleX);

    float sy = sins(angleY);
    float cy = coss(angleY);

    float sz = sins(angleZ);
    float cz = coss(angleZ);

    Matrix rotation = (Matrix){ .m={
        {cy * cz, cy * sz, -sy, 0.0f},
        {sx * sy * cz - cx * sz, sx * sy * sz + cx * cz, sx * cy, 0.0f},
        {cx * sy * cz + sx * sz, cx * sy * sz - sx * cz, cx * cy, 0.0f},
        {x, y, z, 1.0f},
    }};

    glMultMatrixf(rotation.m[0]);
}

void mtx_rotate(short angleX, short angleY, short angleZ) {
    float sx = sins(angleX);
    float cx = coss(angleX);

    float sy = sins(angleY);
    float cy = coss(angleY);

    float sz = sins(angleZ);
    float cz = coss(angleZ);

    Matrix rotation = (Matrix){ .m={
        {cy * cz, cy * sz, -sy, 0.0f},
        {sx * sy * cz - cx * sz, sx * sy * sz + cx * cz, sx * cy, 0.0f},
        {cx * sy * cz + sx * sz, cx * sy * sz - sx * cz, cx * cy, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f},
    }};

    glMultMatrixf(rotation.m[0]);
}

void mtx_translate_rotate_scale(short angleX, short angleY, short angleZ, GLfloat x, GLfloat y, GLfloat z, GLfloat scaleX, GLfloat scaleY, GLfloat scaleZ) {
    float sx = sins(angleX);
    float cx = coss(angleX);

    float sy = sins(angleY);
    float cy = coss(angleY);

    float sz = sins(angleZ);
    float cz = coss(angleZ);

    Matrix rotation = (Matrix){ .m={
        {cy * cz, cy * sz, -sy, 0.0f},
        {sx * sy * cz - cx * sz, sx * sy * sz + cx * cz, sx * cy, 0.0f},
        {cx * sy * cz + sx * sz, cx * sy * sz - sx * cz, cx * cy, 0.0f},
        {x, y, z, 1.0f},
    }};

    glMultMatrixf(rotation.m[0]);

    rotation.m[0][0] = scaleX;
    rotation.m[0][1] = 0.0f;
    rotation.m[1][0] = 0.0f;
    rotation.m[1][1] = scaleY;
    rotation.m[2][2] = scaleZ;
    rotation.m[3][0] = 0.0f;
    rotation.m[3][1] = 0.0f;
    rotation.m[3][2] = 0.0f;

    glMultMatrixf(rotation.m[0]);
}

void mtx_lookat(float eyex, float eyey, float eyez, float centerx, float centery, float centerz, float upx, float upy, float upz) {
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

void mtx_billboard(float x, float y, float z) {
    gBillboardMatrix.m[3][0] = x;
    gBillboardMatrix.m[3][1] = y;
    gBillboardMatrix.m[3][2] = z;

    glMultMatrixf(gBillboardMatrix.m[0]);
}

void mtx_scale(float scaleX, float scaleY, float scaleZ) {
    gScaleMatrix.m[3][0] = scaleX;
    gScaleMatrix.m[3][1] = scaleY;
    gScaleMatrix.m[3][2] = scaleZ;

    glMultMatrixf(gScaleMatrix.m[0]);
}

void set_light(light_t light) {
    glPushMatrix();
    mtx_rotate(light.direction[0], light.direction[1], light.direction[2]);
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
    glVertex3i(-5, 10, 0);
    glColor3f(e->skyColourBottom[0], e->skyColourBottom[1], e->skyColourBottom[2]);
    glTexCoord2f(0, 1.024f);
    glVertex3i(-5, 0, 0);
    glTexCoord2f(2.048f, 1.024f);
    glVertex3i(5, 0, 0);
    glColor3f(e->skyColourTop[0], e->skyColourTop[1], e->skyColourTop[2]);
    glTexCoord2f(2.048f, 0);
    glVertex3i(5, 10, 0);
    glEnd();
}

void render_end(void) {
    rspq_block_run(sRenderEndBlock);
}

rspq_block_t *sPlayerBlock;
rspq_block_t *sBushBlock;
rspq_block_t *sShadowBlock;
rspq_block_t *sDecal1Block;
rspq_block_t *sDecal2Block;


void render_shadow(float pos[3]) {
    set_material(&gTempMaterials[5], MATERIAL_DECAL);
    glPushMatrix();
    glTranslatef(pos[0], pos[1], pos[2]);
    if (sShadowBlock == NULL) {
        rspq_block_begin();
        glScalef(0.5f, 0.5f, 0.5f);
        glBegin(GL_QUADS);
        glColor3f(0, 0, 0);
        glTexCoord2f(2.048f, 0);        glVertex3i(5.0f, 0.0f, -5.0f);
        glTexCoord2f(0, 0);             glVertex3i(-5.0f, 0.0f, -5.0f);
        glTexCoord2f(0, 2.048f);        glVertex3i(-5.0f, 0.0f, 5.0f);
        glTexCoord2f(2.048f, 2.048f);   glVertex3i(5.0f, 0.0f, 5.0f);
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

void render_particles(void) {
    ParticleList *list = gParticleListHead;
    Particle *particle;
    
    while (list) {
        particle = list->particle;
        glPushMatrix();
        if (particle->material) {
            set_texture(particle->material);
        }
        mtx_billboard(particle->pos[0], particle->pos[1], particle->pos[2]);
        mtx_scale(particle->scale[0], particle->scale[1], particle->scale[2]);
        rspq_block_run(sParticleBlock);
        glPopMatrix();
        list = list->next;
    }
}

void render_world(void) {
    if (sCurrentScene && sCurrentScene->model) {
        SceneMesh *s = sCurrentScene->meshList;

        while (s != NULL) {
            if (s->material) {
                set_material(s->material, s->flags);
            }
            rspq_block_run(s->renderBlock);
            /*glPushMatrix();
            glScalef(5.0f, 5.0f, 5.0f);
            model64_draw_primitive(s->mesh);
            glPopMatrix();*/
            s = s->next;
        }
    }
}

void render_object_shadows(void) {
    if (gPlayer) {
        render_shadow(gPlayer->pos);
    }
}

void render_clutter(void) {
    ClutterList *list = gClutterListHead;
    Clutter *obj;
    while (list) {
        obj = list->clutter;
        if (obj->objectID == CLUTTER_BUSH && !(obj->flags & OBJ_FLAG_INVISIBLE)) {
            glPushMatrix();
            
            set_material(&gTempMaterials[3], MATERIAL_NULL);
            mtx_billboard(obj->pos[0], obj->pos[1], obj->pos[2]);        
            if (sBushBlock == NULL) {
                rspq_block_begin();
                render_bush(); 
                sBushBlock = rspq_block_end();
            }
            rspq_block_run(sBushBlock);
            glPopMatrix();
        }
        list = list->next;
    }
}

void render_objects(void) {
    if (gPlayer) {
        glPushMatrix();
        mtx_translate_rotate_scale(0, gPlayer->faceAngle[1], 0, gPlayer->pos[0], gPlayer->pos[1], gPlayer->pos[2], 9.0f, 8.0f, 9.0f);
        set_material(&gTempMaterials[1], MATERIAL_NULL);
        if (sPlayerBlock == NULL) {
            rspq_block_begin();
            model64_draw(gPlayerModel);
            sPlayerBlock = rspq_block_end();
        }
        rspq_block_run(sPlayerBlock);
        glPopMatrix();
    }
    
    ObjectList *list2 = gObjectListHead;
    Object *obj2;
    
    while (list2) {
        obj2 = list2->obj;
        if (obj2->objectID == OBJ_PROJECTILE) {
            glPushMatrix();
            set_material(&gTempMaterials[2], MATERIAL_NULL);
            mtx_billboard(obj2->pos[0], obj2->pos[1], obj2->pos[2]);
            render_bush(); 
            glPopMatrix();
        } else if (obj2->objectID == OBJ_NPC) {
            glPushMatrix();
            set_material(&gTempMaterials[1], MATERIAL_NULL);
            mtx_translate_rotate_scale(0, obj2->faceAngle[1], 0, obj2->pos[0], obj2->pos[1], obj2->pos[2], 9.0f, 8.0f, 9.0f);
            rspq_block_run(sPlayerBlock);
            glPopMatrix();
        }
        list2 = list2->next;
    }
    
    if (sDecal1Block == NULL) {
        rspq_block_begin();
        glPushMatrix();
        glTranslatef(25, 0, 25);
        glBegin(GL_QUADS);
        glColor3f(1.0f, 0, 1.0f);
        glTexCoord2f(1.024f, 0);        glVertex3i(5.0f, 0.0f, -5.0f);
        glTexCoord2f(0, 0);             glVertex3i(-5.0f, 0.0f, -5.0f);
        glTexCoord2f(0, 1.024f);        glVertex3i(-5.0f, 0.0f, 5.0f);
        glTexCoord2f(1.024f, 1.024f);   glVertex3i(5.0f, 0.0f, 5.0f);
        glEnd();
        glPopMatrix();
        sDecal1Block = rspq_block_end();
    }
    if (sDecal2Block == NULL) {
        rspq_block_begin();
        glPushMatrix();
        glTranslatef(25, 0, 35);
        glBegin(GL_QUADS);
        glColor3f(1.0f, 0, 1.0f);
        glTexCoord2f(1.024f, 0);        glVertex3i(5.0f, 0.0f, -5.0f);
        glTexCoord2f(0, 0);             glVertex3i(-5.0f, 0.0f, -5.0f);
        glTexCoord2f(0, 1.024f);        glVertex3i(-5.0f, 0.0f, 5.0f);
        glTexCoord2f(1.024f, 1.024f);   glVertex3i(5.0f, 0.0f, 5.0f);
        glEnd();
        glPopMatrix();
        sDecal2Block = rspq_block_end();
    }
    set_material(&gTempMaterials[2], MATERIAL_DECAL);
    rspq_block_run(sDecal1Block);
    set_material(&gTempMaterials[4], MATERIAL_DECAL);
    rspq_block_run(sDecal2Block);
}

void render_game(int updateRate, float updateRateF) {
    DEBUG_SNAPSHOT_1();
    rdpq_attach(gFrameBuffers, &gZBuffer);
    gl_context_begin();
    glClear(GL_DEPTH_BUFFER_BIT);
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
    get_time_snapshot(PP_RENDER, DEBUG_SNAPSHOT_1_END);
    render_end();
    gl_context_end();
    render_hud(updateRate, updateRateF);
    render_menus(updateRate, updateRateF);
}