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

Environment *gEnvironment;
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

light_t light = {
    
    color: { 0.51f, 0.81f, 0.665f, 0.5f},
    diffuse: {1.0f, 1.0f, 1.0f, 1.0f},
    direction: {0.0f, -60.0f, 0.0f},
    position: {1.0f, 0.0f, 0.0f, 0.0f},
    radius: 10.0f,
};

light_t lightNeutral = {
    
    color: { 0.66f, 0.66f, 0.66f, 0.66f},
    diffuse: {1.0f, 1.0f, 1.0f, 1.0f},
    direction: {0.0f, -60.0f, 0.0f},
    position: {1.0f, 0.0f, 0.0f, 0.0f},
    radius: 10.0f,
};

static void init_particles(void) {
    rspq_block_begin();
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex3i(-5, 0, 5);
    glTexCoord2f(0, 1.024f);
    glVertex3i(-5, 0, -5);
    glTexCoord2f(1.024f, 1.024f);
    glVertex3i(5, 0, -5);
    glTexCoord2f(1.024f, 0);
    glVertex3i(5, 0, 5);
    glEnd();
    sParticleBlock = rspq_block_end();
}

void init_renderer(void) {
    setup_light(lightNeutral);
    setup_fog(light);
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

void set_light(light_t light) {
    glPushMatrix();
    glRotatef(light.direction[0], 1, 0, 0);
    glRotatef(light.direction[1], 0, 1, 0);
    glRotatef(light.direction[2], 0, 0, 1);
    glLightfv(GL_LIGHT0, GL_POSITION, light.position);
    glPopMatrix();
}

void setup_fog(light_t light) {
    gEnvironment = malloc(sizeof(Environment));
    gEnvironment->fogColour[0] = light.color[0];
    gEnvironment->fogColour[1] = light.color[1];
    gEnvironment->fogColour[2] = light.color[2];
    gEnvironment->skyColourBottom[0] = light.color[0] * 0.66f;
    gEnvironment->skyColourBottom[1] = light.color[1] * 0.66f;
    gEnvironment->skyColourBottom[2] = light.color[2] * 0.66f;
    gEnvironment->skyColourTop[0] = light.color[0] * 1.33f;
    if (gEnvironment->skyColourTop[0] > 1.0f) {
        gEnvironment->skyColourTop[0] = 1.0f;
    }
    gEnvironment->skyColourTop[1] = light.color[1] * 1.33f;
    if (gEnvironment->skyColourTop[1] > 1.0f) {
        gEnvironment->skyColourTop[1] = 1.0f;
    }
    gEnvironment->skyColourTop[2] = light.color[2] * 1.33f;
    if (gEnvironment->skyColourTop[2] > 1.0f) {
        gEnvironment->skyColourTop[2] = 1.0f;
    }
    gEnvironment->flags = ENV_FOG;
    gEnvironment->fogNear = 150.0f;
    gEnvironment->fogFar = 400.0f;
    glFogf(GL_FOG_START, gEnvironment->fogNear);
    glFogf(GL_FOG_END, gEnvironment->fogFar);
    glFogfv(GL_FOG_COLOR, gEnvironment->fogColour);
}

void project_camera(void) {
    Camera *c = gCamera;
    float nearClip = 5.0f;
    float farClip = 500.0f;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gAspectRatio = (float) display_get_width() / (float) (display_get_height());
    glFrustum(-nearClip * gAspectRatio, nearClip * gAspectRatio, -nearClip, nearClip, nearClip, farClip);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(c->pos[0], c->pos[1], c->pos[2], c->focus[0], c->focus[1], c->focus[2], 0.0f, 0.0f, 1.0f);
}

void render_sky(void) {
    Environment *e = gEnvironment;
    if (sRenderSkyBlock == NULL) {
        rspq_block_begin();
        glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION);
        glDisable(GL_MULTISAMPLE_ARB);
        glLoadIdentity();
        glOrtho(0.0f, display_get_width(), display_get_height(), 0.0f, -1.0f, 1.0f);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glBegin(GL_QUADS);
        glColor3f(e->skyColourTop[0], e->skyColourTop[1], e->skyColourTop[2]);
        glVertex2i(0, 0);
        glColor3f(e->skyColourBottom[0], e->skyColourBottom[1], e->skyColourBottom[2]);
        glVertex2i(0, display_get_height());
        glVertex2i(display_get_width(), display_get_height());
        glColor3f(e->skyColourTop[0], e->skyColourTop[1], e->skyColourTop[2]);
        glVertex2i(display_get_width(), 0);
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
    glVertex3i(-5, 0, 10);
    glColor3f(e->skyColourBottom[0], e->skyColourBottom[1], e->skyColourBottom[2]);
    glTexCoord2f(0, 1.024f);
    glVertex3i(-5, 0, 0);
    glTexCoord2f(1.024f, 1.024f);
    glVertex3i(5, 0, 0);
    glColor3f(e->skyColourTop[0], e->skyColourTop[1], e->skyColourTop[2]);
    glTexCoord2f(1.024f, 0);
    glVertex3i(5, 0, 10);
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
        glScalef(0.66f, 0.66f, 0.66f);
        glBegin(GL_QUADS);
        glColor3f(0, 0, 0);
        glTexCoord2f(0, 0);
        glVertex3i(-5.0f, 5.0f, 0);
        glTexCoord2f(0, 2.048f);
        glVertex3i(-5.0f, -5.0f, 0);
        glTexCoord2f(2.048f, 2.048f);
        glVertex3i(5.0f, -5.0f, 0);
        glTexCoord2f(2.048f, 0);
        glVertex3i(5.0f, 5.0f, 0);
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

void glTranslateRotatef(short angle, GLfloat x, GLfloat y, GLfloat z) {
    float c = coss(angle);
    float s = sins(angle);

    gl_matrix_t rotation = (gl_matrix_t){ .m={
        {c, s, 0.0f, 0.0f},
        {-s, c, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {x, y, z, 1.0f},
    }};

    glMultMatrixf(rotation.m[0]);
}

void glTranslateRotateScalef(short angle, GLfloat x, GLfloat y, GLfloat z, GLfloat scaleX, GLfloat scaleY, GLfloat scaleZ) {
    float c = coss(angle);
    float s = sins(angle);

    gl_matrix_t rotation = (gl_matrix_t){ .m={
        {c, s, 0.0f, 0.0f},
        {-s, c, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {x, y, z, 1.0f},
    }};

    glMultMatrixf(rotation.m[0]);

    gl_matrix_t scale = (gl_matrix_t){ .m={
        {scaleX, 0.0f, 0.0f, 0.0f},
        {0.0f, scaleY, 0.0f, 0.0f},
        {0.0f, 0.0f, scaleZ, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f},
    }};

    glMultMatrixf(scale.m[0]);
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
        glTranslateRotateScalef(gCamera->yaw, 
        particle->pos[0], particle->pos[1], particle->pos[2], 
        particle->scale[0], particle->scale[1], particle->scale[2]);
        rspq_block_run(sParticleBlock);
        glPopMatrix();
        list = list->next;
    }
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

    if (sCurrentScene && sCurrentScene->model) {
        SceneMesh *s = sCurrentScene->meshList;

        while (s != NULL) {
            if (s->material) {
                set_material(s->material, s->flags);
            }
            rspq_block_run(s->renderBlock);
            s = s->next;
        }
    }

    if (gPlayer) {
        render_shadow(gPlayer->pos);
    }

    ClutterList *list = gClutterListHead;
    Clutter *obj;
    while (list) {
        obj = list->clutter;
        if (obj->objectID == CLUTTER_BUSH && !(obj->flags & OBJ_FLAG_INVISIBLE)) {
            glPushMatrix();
            
            set_material(&gTempMaterials[3], MATERIAL_NULL);
            glTranslateRotatef(gCamera->yaw, obj->pos[0], obj->pos[1], obj->pos[2]);            
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
    
    apply_anti_aliasing(AA_ACTOR);

    if (gPlayer) {
        glPushMatrix();
        glTranslateRotateScalef(gPlayer->faceAngle[2], gPlayer->pos[0], gPlayer->pos[1], gPlayer->pos[2], 0.9f, 1.25f, 1.25f);
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
            glTranslateRotatef(gCamera->yaw, obj2->pos[0], obj2->pos[1], obj2->pos[2]);
            render_bush(); 
            glPopMatrix();
        } else if (obj2->objectID == OBJ_NPC) {
            glPushMatrix();
            set_material(&gTempMaterials[1], MATERIAL_NULL);
            glTranslateRotateScalef(obj2->faceAngle[2], obj2->pos[0], obj2->pos[1], obj2->pos[2], 0.9f, 1.25f, 1.25f);
            rspq_block_run(sPlayerBlock);
            glPopMatrix();
        }
        list2 = list2->next;
    }
    
    if (sDecal1Block == NULL) {
        rspq_block_begin();
        glPushMatrix();
        glTranslatef(25, 25, 0);
        glBegin(GL_QUADS);
        glColor3f(1.0f, 0, 1.0f);
        glTexCoord2f(0, 0);
        glVertex3i(-5, 5, 0);
        glTexCoord2f(0, 1.024f);
        glVertex3i(-5, -5, 0);
        glTexCoord2f(1.024f, 1.024f);
        glVertex3i(5, -5, 0);
        glTexCoord2f(1.024f, 0);
        glVertex3i(5, 5, 0);
        glEnd();
        glPopMatrix();
        sDecal1Block = rspq_block_end();
    }
    if (sDecal2Block == NULL) {
        rspq_block_begin();
        glPushMatrix();
        glTranslatef(25, 35, 0);
        glBegin(GL_QUADS);
        glColor3f(1.0f, 0, 1.0f);
        glTexCoord2f(0, 0);
        glVertex3i(-5, 5, 0);
        glTexCoord2f(0, 1.024f);
        glVertex3i(-5, -5, 0);
        glTexCoord2f(1.024f, 1.024f);
        glVertex3i(5, -5, 0);
        glTexCoord2f(1.024f, 0);
        glVertex3i(5, 5, 0);
        glEnd();
        glPopMatrix();
        sDecal2Block = rspq_block_end();
    }
    set_material(&gTempMaterials[2], MATERIAL_DECAL);
    rspq_block_run(sDecal1Block);
    set_material(&gTempMaterials[4], MATERIAL_DECAL);
    rspq_block_run(sDecal2Block);

    
    apply_anti_aliasing(AA_GEO);
    set_particle_render_settings();
    render_particles();

    get_time_snapshot(PP_RENDER, DEBUG_SNAPSHOT_1_END);
    
    render_end();
    gl_context_end();
    render_hud(updateRate, updateRateF);
    render_menus(updateRate, updateRateF);
}