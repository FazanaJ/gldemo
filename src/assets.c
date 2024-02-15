#include <libdragon.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>
#include <malloc.h>

#include "assets.h"
#include "../include/global.h"
#define INCLUDE_TEX_TABLE
#include "../include/texture_table.h"
#undef INCLUDE_TEX_TABLE

#include "debug.h"
#include "scene.h"
#include "main.h"
#include "math_util.h"

char *gFontAssetTable[] = {
    "arial.10",
    "mvboli.12"
};

char *sObjectOverlays[OBJ_TOTAL] = {
    NULL,
    "player",
    "projectile",
    "npc",
    "crate",
    "testsphere",
    "barrel",
};

char *gModelIDs[OBJ_TOTAL] = {
    "humanoid",
    "rock",
    "bottombillboard",
    "crate",
    "testsphere",
    "testcylinder",
};

short gObjectModels[OBJ_TOTAL] = {
    0,
    1,
    3,
    1,
    4,
    5,
    6,
};

short playerModelTextures[][9] = {
    {TEXTURE_PLEASE, 0, 0, 0, 0, 0, 0, 0, 0}, // Ears
    {TEXTURE_PLEASE, 0, 0, 0, 0, 0, 0, 0, 0}, // Feet
    {TEXTURE_STONEFLOOR4, 0, 0, 0, 0, 0, 0, 0, 0}, // Hair
    {TEXTURE_PLEASE, 0, 0, 0, 0, 0, 0, 0, 0}, // Hands
    {TEXTURE_EYE1, TEXTURE_INTEROSIGN2, TEXTURE_MOUTH1, TEXTURE_PLEASE, 0, 0, 0, 0, 0}, // Head
    {TEXTURE_TROUSERS, 0, 0, 0, 0, 0, 0, 0, 0}, // Legs
    {TEXTURE_PLEASE, 0, 0, 0, 0, 0, 0, 0, 0}, // Tail
    {TEXTURE_PLEASE, TEXTURE_SHIRT, 0, 0, 0, 0, 0, 0, 0}, // Torso
};

short playerModelFlags[][9] = {
    {0, 0, 0, MATERIAL_CUTOUT | MATERIAL_DECAL, 0, 0, 0, 0, 0},
    {0, 0, 0, 0 | 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0 | 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0 | 0, 0, 0, 0, 0, 0},
    {MATERIAL_CUTOUT | MATERIAL_DECAL, 0, MATERIAL_CUTOUT | MATERIAL_DECAL, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0 | 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0 | 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0 | 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0 | 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0 | 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0 | 0, 0, 0, 0, 0, 0},
};

MaterialList *gMaterialListHead;
MaterialList *gMaterialListTail;
VoidList *gOverlayListHead = NULL;
VoidList *gOverlayListTail = NULL;
ModelList *gModelIDListHead = NULL;
ModelList *gModelIDListTail = NULL;
rdpq_font_t *gFonts[FONT_TOTAL];
#ifdef PUPPYPRINT_DEBUG
short gNumTextures;
short gNumTextureLoads;
#endif

void init_materials(void) {
    bzero(&gRenderSettings, sizeof(RenderSettings));
    gPrevRenderFlags = 0;
    gPrevTextureID = 0;
    gMaterialListHead = NULL;
    gMaterialListTail = NULL;
#ifdef PUPPYPRINT_DEBUG
    gNumTextures = 0;
    gNumTextureLoads = 0;
#endif
    rspq_block_begin();
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_COLOR_MATERIAL);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    gParticleMaterialBlock = rspq_block_end();
}

void bind_new_texture(MaterialList *material) {
    int repeatH;
    int repeatV;
    int mirrorH;
    int mirrorV;
    int texID = material->textureID;

    if (gTextureIDs[texID].flags & TEX_CLAMP_H) {
        repeatH = false;
    } else {
        repeatH = REPEAT_INFINITE;
    }
    if (gTextureIDs[texID].flags & TEX_CLAMP_V) {
        repeatV = false;
    } else {
        repeatV = REPEAT_INFINITE;
    }
    if (gTextureIDs[texID].flags & TEX_MIRROR_H) {
        mirrorH = MIRROR_REPEAT;
    } else {
        mirrorH = MIRROR_DISABLED;
    }
    if (gTextureIDs[texID].flags & TEX_MIRROR_V) {
        mirrorV = MIRROR_REPEAT;
    } else {
        mirrorV = MIRROR_DISABLED;
    }
    glBindTexture(GL_TEXTURE_2D, material->texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glSpriteTextureN64(GL_TEXTURE_2D, material->sprite, &(rdpq_texparms_t){.s.repeats = repeatH, .t.repeats = repeatV, .s.mirror = mirrorH, .t.mirror = mirrorV});
}

static char *sFileFormatString[] = {
    "sprite",
    "wav64",
    "model64",
    "xm64",
    "font64",
    "dso"
};

static char sFileFormatName[40];

char *asset_dir(char *dir, int format) {
    sprintf(sFileFormatName, "rom:/%s.%s", dir, sFileFormatString[format]);
    return sFileFormatName;
}

void load_font(int fontID) {
    if (gFonts[fontID] == NULL) {
        gFonts[fontID] = rdpq_font_load(asset_dir(gFontAssetTable[fontID - 1], DFS_FONT64));
        rdpq_font_style(gFonts[fontID], 0, &(rdpq_fontstyle_t) { .color = RGBA32(255, 255, 255, 255),});
        rdpq_text_register_font(fontID, gFonts[fontID]);
    }
}

void free_font(int fontID) {
    if (gFonts[fontID] != NULL) {
        rdpq_font_free(gFonts[fontID]);
    }
}

int load_texture(Material *material) {
    DEBUG_SNAPSHOT_1();
    MaterialList *list = gMaterialListHead;
    // Check if the texture is already loaded, and bind it to the index.
    while (list) {
        if (list->textureID == material->textureID) {
            material->index = list;
            return 0;
        }
        list = list->next;
    }
    // It doesn't, so load the texture into RAM and create a new entry.
    list = malloc(sizeof(MaterialList));
    if (list == NULL) {
        return -1;
    }
    if (gMaterialListHead == NULL) {
        gMaterialListHead = list;
        gMaterialListHead->prev = NULL;
        gMaterialListTail = gMaterialListHead;
    } else {
        gMaterialListTail->next = list;
        gMaterialListTail->next->prev = gMaterialListTail;
        gMaterialListTail = gMaterialListTail->next;
    }
    list->next = NULL;
    debugf("Loading texture: %s.", gTextureIDs[material->textureID].file);
    list->sprite = sprite_load(asset_dir(gTextureIDs[material->textureID].file, DFS_SPRITE));
    list->textureID = material->textureID;
    glGenTextures(1, &list->texture);
    bind_new_texture(list);
    list->loadTimer = 10;
    material->index = list;
    debugf(" Time: %2.3fs.\n", ((float) TIMER_MICROS(DEBUG_SNAPSHOT_1_END)) / 1000000.0f);
#ifdef PUPPYPRINT_DEBUG
    gNumTextures++;
#endif
    return 0;
}

static void free_material(MaterialList *material) {
    if (material == gMaterialListHead) {
        if (gMaterialListHead->next) {
            gMaterialListHead = gMaterialListHead->next;
            gMaterialListHead->prev = NULL;
        } else {
            gMaterialListHead = NULL;
            if (material == gMaterialListTail) {
                gMaterialListTail = NULL;
            }
        }
    } else {
        if (material == gMaterialListTail) {
            gMaterialListTail = gMaterialListTail->prev;
        }
        material->prev->next = material->next;
        if (material->next) {
            material->next->prev = material->prev;
        }
    }
    sprite_free(material->sprite);
    glDeleteTextures(1, &material->texture);
    free(material);
#ifdef PUPPYPRINT_DEBUG
    gNumTextures--;
#endif
}

void shadow_generate(Object *obj) {
    DEBUG_SNAPSHOT_1();
    debugf("Allocating dynamic shadow for [%s] object.", sObjectOverlays[obj->objectID]);
    obj->gfx->dynamicShadow = malloc(sizeof(DynamicShadow));
    DynamicShadow *d = obj->gfx->dynamicShadow;
    d->texW = obj->header->dynamicShadow->texW;
    d->texH = obj->header->dynamicShadow->texH;
    d->planeW = obj->header->dynamicShadow->planeW;
    d->planeH = obj->header->dynamicShadow->planeH;
    d->offset = obj->header->dynamicShadow->offset;
    d->angle[0] = 0;
    d->angle[1] = 0;
    d->angle[2] = 0;
    d->staleTimer = 10;
    d->texCount = 0;
    d->surface = surface_alloc(FMT_I8, d->texW, d->texH);
    int w = 0;
    int h = 0;
    float frac = (float) d->texW / 64.0f;
    if (frac <= 1.0f) {
        w = 1;
    } else {
        float newFrac = frac - (int) frac;
        if (newFrac > 0.0f) {
            frac = (int) frac + 1;
        }
        w = frac;
    }
    frac = (float) d->texH / 64.0f;
    if (frac <= 1.0f) {
        h = 1;
    } else {
        float newFrac = frac - (int) frac;
        if (newFrac > 0.0f) {
            frac = (int) frac + 1;
        }
        h = frac;
    }
    d->acrossX = w;
    d->acrossY = h;
    d->texCount = w * h;
    int x = 0;
    int y = 0;
    int texLoads = d->texCount - 1;
    int stepW = MIN(MIN(d->texW, 64), d->texW / w);
    int stepH = MIN(MIN(d->texH, 64), d->texH / h);
    for (int i = 0; i < h; i++) {
        x = 0;
        for (int j = 0; j < w; j++) {
            glGenTextures(1, &d->tex[texLoads]);
            glBindTexture(GL_TEXTURE_2D, d->tex[texLoads--]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            surface_t surf = surface_make_sub(&d->surface, x, y, stepW, stepH);
            glSurfaceTexImageN64(GL_TEXTURE_2D, 0, &surf, &(rdpq_texparms_t){.s.repeats = true, .t.repeats = true});
            x += stepW;
        }
        y += stepH;
    }
    debugf(" Texture count: %d.", d->texCount);
    debugf(" Time: %2.3fs.\n", ((float) TIMER_MICROS(DEBUG_SNAPSHOT_1_END)) / 1000000.0f);
}

void asset_cycle(int updateRate) {
    DEBUG_SNAPSHOT_1();
    if (gMaterialListHead == NULL) {
        get_time_snapshot(PP_MATERIALS, DEBUG_SNAPSHOT_1_END);
        return;
    }
    MaterialList *matList = gMaterialListHead;
    while (matList) {
        MaterialList *curList = matList;
        matList->loadTimer -= updateRate;
        matList = matList->next;
        if (curList->loadTimer <= 0) {
            debugf("Freeing texture: %s.\n", gTextureIDs[curList->textureID].file);
            free_material(curList);
            break;
        }
    }
#ifdef PUPPYPRINT_DEBUG
    gNumTextureLoads = 0;
#endif
    gPrevTextureID = 0;
    gPrevRenderFlags = 0;
    bzero(&gRenderSettings, sizeof(RenderSettings));
    if (gEnvironment->texGen) {
        gEnvironment->skyTimer -= updateRate;
        if (gEnvironment->skyTimer <= 0) {
            debugf("Freeing texture: %s.\n", gTextureIDs[gEnvironment->skyboxTextureID].file);
            for (int i = 0; i < 32; i++) {
                glDeleteTextures(1, &gEnvironment->textureSegments[i]);
            }
            sprite_free(gEnvironment->skySprite);
            gEnvironment->texGen = false;
        }
    }
    get_time_snapshot(PP_MATERIALS, DEBUG_SNAPSHOT_1_END);
}

void sky_texture_generate(Environment *e) {
    DEBUG_SNAPSHOT_1();
    debugf("Loading texture: %s.", gTextureIDs[e->skyboxTextureID].file);
    e->skySprite = sprite_load(asset_dir(gTextureIDs[e->skyboxTextureID].file, DFS_SPRITE));
    surface_t surf = sprite_get_pixels(e->skySprite);
    int x = 0;
    int y = 0;
    for (int i = 0; i < 32; i++) {
        if (i == 16) {
            y += 64;
            x = 0;
        }
        glGenTextures(1, &e->textureSegments[i]);
        glBindTexture(GL_TEXTURE_2D, e->textureSegments[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        surface_t surfPiece = surface_make_sub(&surf, x, y, 32, 64);
        glSurfaceTexImageN64(GL_TEXTURE_2D, 0, &surfPiece, &(rdpq_texparms_t){.s.repeats = false, .t.repeats = false});
        x += 32;
    }
    debugf(" Time: %2.3fs.\n", ((float) TIMER_MICROS(DEBUG_SNAPSHOT_1_END)) / 1000000.0f);
}

rspq_block_t *sky_gradient_generate(Environment *e) {
    int width = display_get_width();
    int height = display_get_height();
    rspq_block_begin();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_MULTISAMPLE_ARB);
    matrix_ortho();
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
    return rspq_block_end();
}

static void init_object_behaviour(Object *obj, int objectID) {
    DEBUG_SNAPSHOT_1();
    void *addr = NULL;
    VoidList *list = gOverlayListHead;
    if (gOverlayListHead) {
        while (list) {
            if (list->id == objectID) {
                addr = list->addr;
                break;
            }
            list = list->next;
        }
    }

    if (addr == NULL) {
        debugf("Loading overlay [%s].", sObjectOverlays[objectID]);
        list = malloc(sizeof(VoidList));
        if (gOverlayListHead == NULL) {
            gOverlayListHead = list;
            list->prev = NULL;
        }
        if (gOverlayListTail) {
            gOverlayListTail->next = list;
            list->prev = gOverlayListTail;
        }
        gOverlayListTail = list;
        addr = dlopen(asset_dir(sObjectOverlays[objectID], DFS_OVERLAY), RTLD_LOCAL);
        list->addr = addr;
        list->id = objectID;
        list->next = NULL;
        list->timer = 10;
#ifdef PUPPYPRINT_DEBUG
        gNumOverlays++;
#endif
        debugf(" Time: %2.3fs.\n", ((float) TIMER_MICROS(DEBUG_SNAPSHOT_1_END)) / 1000000.0f);
    }
    ObjectEntry *entry = dlsym(addr, "entry");
    obj->header = entry;
    obj->loopFunc = entry->loopFunc;
    obj->flags = entry->flags;
    obj->overlay = list;
    obj->hitbox = entry->hitbox;
    if (entry->viewDist) {
        obj->viewDist = entry->viewDist << 4;
    } else {
        obj->viewDist = 500.0f;
    }
    obj->viewDist *= obj->viewDist;
    if (entry->data) {
        obj->data = malloc(entry->data);
        bzero(obj->data, entry->data);
    }
    if (entry->initFunc) {
        (*entry->initFunc)(obj);
    }
}

void check_unused_model(Object *obj) {
    ObjectList *objList = gObjectListHead;
    Object *listObj;

    while (objList) {
        listObj = objList->obj;
        if (listObj->gfx->modelID == obj->gfx->modelID && listObj != obj) {
            return;
        }
        objList = objList->next;
    }
    
    debugf("Freeing model [%s]\n", gModelIDs[obj->gfx->modelID - 1]);
    if (obj->gfx->listEntry == gModelIDListHead) {
        if (gModelIDListHead->next) {
            gModelIDListHead = gModelIDListHead->next;
            gModelIDListHead->prev = NULL;
        } else {
            gModelIDListHead = NULL;
        }
    } else {
        if (obj->gfx->listEntry == gModelIDListTail) {
            gModelIDListTail = gModelIDListTail->prev;
        }
        obj->gfx->listEntry->prev->next = obj->gfx->listEntry->next;
        if (obj->gfx->listEntry->next) {
            obj->gfx->listEntry->next->prev = obj->gfx->listEntry->prev;
        }
    }

    ObjectModel *curMesh = obj->gfx->listEntry->entry;
    while (curMesh) {
        ObjectModel *m = curMesh;
        curMesh = curMesh->next;
        rspq_block_free(m->block);
        free(m);
    }
    model64_free(obj->gfx->listEntry->model64);
    free(obj->gfx->listEntry);
#ifdef PUPPYPRINT_DEBUG
        gNumModels--;
#endif
}

void check_unused_overlay(Object *obj, VoidList *overlay) {
    ObjectList *objList = gObjectListHead;
    Object *listObj;

    while (objList) {
        listObj = objList->obj;
        if (listObj->overlay == overlay && listObj != obj) {
            return;
        }
        objList = objList->next;
    }

    debugf("Freeing overlay [%s]\n", sObjectOverlays[overlay->id]);
    if (overlay == gOverlayListHead) {
        if (gOverlayListHead->next) {
            gOverlayListHead = gOverlayListHead->next;
            gOverlayListHead->prev = NULL;
        } else {
            gOverlayListHead = NULL;
        }
    } else {
        if (overlay == gOverlayListTail) {
            gOverlayListTail = gOverlayListTail->prev;
        }
        overlay->prev->next = overlay->next;
        if (overlay->next) {
            overlay->next->prev = overlay->prev;
        }
    }
    dlclose(overlay->addr);
    free(overlay);
#ifdef PUPPYPRINT_DEBUG
        gNumOverlays--;
#endif
}

/**
 * Check if gObjectListHead is null first.
 * If it is, set that as the new list element, otherwise, make a new element and join it onto the tail.
*/
Object *allocate_object(void) {
    Object *newObj = malloc(sizeof(Object));
    bzero(newObj, sizeof(Object));
    newObj->entry = malloc(sizeof(ObjectList));

    if (gObjectListHead == NULL) {
        gObjectListHead = newObj->entry;
        gObjectListHead->next = NULL;
        gObjectListHead->prev = NULL;
        gObjectListTail = gObjectListHead;
        gObjectListHead->obj = newObj;
    } else {
        ObjectList *list = newObj->entry;
        gObjectListTail->next = list;
        list->prev = gObjectListTail;
        list->next = NULL;
        list->obj = newObj;
        gObjectListTail = list;
    }
    newObj->loopFunc = NULL;
    newObj->gfx = NULL;
    newObj->data = NULL;
    newObj->flags = OBJ_FLAG_NONE;
    newObj->viewDist = SQR(300.0f);
    newObj->overlay = NULL;
#ifdef PUPPYPRINT_DEBUG
    gNumObjects++;
#endif
    return newObj;
}

Clutter *allocate_clutter(void) {
    Clutter *newClutter = malloc(sizeof(Clutter));
    bzero(newClutter, sizeof(Clutter));
    newClutter->entry = malloc(sizeof(ClutterList));

    if (gClutterListHead == NULL) {
        gClutterListHead = newClutter->entry;
        gClutterListHead->next = NULL;
        gClutterListHead->prev = NULL;
        gClutterListTail = gClutterListHead;
        gClutterListHead->clutter = newClutter;
    } else {
        ClutterList *list = newClutter->entry;
        gClutterListTail->next = list;
        list->prev = gClutterListTail;
        list->next = NULL;
        list->clutter = newClutter;
        gClutterListTail = list;
    }
    newClutter->gfx = NULL;
    newClutter->flags = OBJ_FLAG_NONE;
    newClutter->viewDist = SQR(200.0f);
#ifdef PUPPYPRINT_DEBUG
    gNumClutter++;
#endif
    return newClutter;
}

Particle *allocate_particle(void) {
    Particle *newParticle = malloc(sizeof(Particle));
    bzero(newParticle, sizeof(Particle));
    newParticle->entry = malloc(sizeof(ParticleList));

    if (gParticleListHead == NULL) {
        gParticleListHead = newParticle->entry;
        gParticleListHead->next = NULL;
        gParticleListHead->prev = NULL;
        gParticleListTail = gParticleListHead;
        gParticleListHead->particle = newParticle;
    } else {
        ParticleList *list = newParticle->entry;
        gParticleListTail->next = list;
        list->prev = gParticleListTail;
        list->next = NULL;
        list->particle = newParticle;
        gParticleListTail = list;
    }
    newParticle->material = NULL;
    newParticle->flags = OBJ_FLAG_NONE;
#ifdef PUPPYPRINT_DEBUG
    gNumParticles++;
#endif
    return newParticle;
}

static int temp_matrix_grabber(int modelID) {
    switch (modelID) {
    case 1:
        return MTX_TRANSLATE_ROTATE_SCALE;
    case 2:
        return MTX_TRANSLATE_ROTATE_SCALE;
    case 3:
        return MTX_BILLBOARD;
    }
    return MTX_TRANSLATE_ROTATE_SCALE;
}

static void load_object_model(Object *obj, int objectID) {
    DEBUG_SNAPSHOT_1();
    obj->gfx = malloc(sizeof(ObjectGraphics));
    bzero(obj->gfx, sizeof(ObjectGraphics));
    obj->gfx->envColour[0] = 0xFF;
    obj->gfx->envColour[1] = 0xFF;
    obj->gfx->envColour[2] = 0xFF;
    obj->gfx->primColour[0] = 0xFF;
    obj->gfx->primColour[1] = 0xFF;
    obj->gfx->primColour[2] = 0xFF;
    obj->gfx->opacity = 0xFF;
    int modelID = gObjectModels[objectID];
    obj->gfx->modelID = modelID;
    int matrixType = temp_matrix_grabber(modelID);

    if (gModelIDListHead) {
        ModelList *modelList = gModelIDListHead;
        while (modelList) {
            if (modelList->id == modelID) {
                obj->gfx->listEntry = modelList;
                return;
            }
            modelList = modelList->next;
        }
    }

    ModelList *list = malloc(sizeof(ModelList));
    list->entry = NULL;
    obj->gfx->listEntry = list;

    if (gModelIDListHead == NULL) {
        gModelIDListHead = list;
        list->prev = NULL;
    } else {
        gModelIDListTail->next = list;
        list->prev = gModelIDListTail;
    }
    gModelIDListTail = list;

    debugf("Loading model [%s].", gModelIDs[modelID - 1]);
    list->model64 = model64_load(asset_dir(gModelIDs[modelID - 1], DFS_MODEL64));
    list->entry = NULL;
    int numMeshes = model64_get_mesh_count(list->model64);
    ObjectModel *tail = NULL;
    for (int i = 0; i < numMeshes; i++) {
        mesh_t *mesh = model64_get_mesh(list->model64, i);
        int primCount = model64_get_primitive_count(mesh);
        for (int j = 0; j < primCount; j++) {
            ObjectModel *m = malloc(sizeof(ObjectModel));
            primitive_t *prim = model64_get_primitive(mesh, j);
            if (modelID == 1) {
                m->material.flags = playerModelFlags[i][j] | MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_LIGHTING;
                m->material.textureID = playerModelTextures[i][j];
                m->material.combiner = 0;
            } else if (modelID == 4) {
                m->material.flags = MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_LIGHTING | MATERIAL_VTXCOL;
                m->material.textureID = TEXTURE_CRATE;
                m->material.combiner = 0;
            } else {
                m->material.flags = MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_LIGHTING | MATERIAL_VTXCOL;
                m->material.textureID = -1;
                m->material.combiner = 0;
            }
            m->material.index = NULL;
            m->next = NULL;
            if (i == 0 && j == 0) {
                m->matrixBehaviour = matrixType;
            } else {
                m->matrixBehaviour = 0;
            }
            rspq_block_begin();
            //glScalef(10.0f, 9.0f, 10.0f);
            if (modelID == 1) {
                glScalef(0.95f, 1.33f, 1.33f);
            } else {
                glScalef(1.0f, 1.0f, 1.0f);
            }
            model64_draw_primitive(prim);
            m->block = rspq_block_end();

            if (list->entry == NULL) {
                list->entry = m;
            } else {
                tail->next = m;
            }
            tail = m;
        }
    }
    debugf(" Time: %2.3fs.\n", ((float) TIMER_MICROS(DEBUG_SNAPSHOT_1_END)) / 1000000.0f);

#ifdef PUPPYPRINT_DEBUG
        gNumModels++;
#endif
    list->next = NULL;
    list->timer = 10;
    list->id = modelID;
}

void set_object_functions(Object *obj, int objectID) {
    if (sObjectOverlays[objectID]) {
        init_object_behaviour(obj, objectID);
    }
    if (gObjectModels[objectID]) {
        load_object_model(obj, objectID);
    }
}

void free_dynamic_shadow(Object *obj) {
    DynamicShadow *d = obj->gfx->dynamicShadow;
    debugf("Freeing dynamic shadow for [%s] object.\n", sObjectOverlays[obj->objectID]);
    surface_free(&d->surface);
    for (int i = 0; i < d->texCount; i++) {
        glDeleteTextures(1, &d->tex[i]);
    }
    free(obj->gfx->dynamicShadow);
    obj->gfx->dynamicShadow = NULL;
}

/**
 * Remove an object from the list, then reconnect the list.
 * Free the list entry, then the object, and any further elements from RAM.
*/
void free_object(Object *obj) {
    if (obj->entry == gObjectListHead) {
        if (gObjectListHead->next) {
            gObjectListHead = gObjectListHead->next;
            gObjectListHead->prev = NULL;
        } else {
            gObjectListHead = NULL;
        }
    } else {
        if (obj->entry == gObjectListTail) {
            gObjectListTail = gObjectListTail->prev;
        }
        obj->entry->prev->next = obj->entry->next;
        if (obj->entry->next) {
            obj->entry->next->prev = obj->entry->prev;
        }
    }
    if (obj->overlay) {
        rdpq_set_mode_standard();
        check_unused_overlay(obj, obj->overlay);
    }
    free(obj->entry);
    if (obj->data) {
        free(obj->data);
    }
    if (obj->gfx) {
        if (obj->gfx->dynamicShadow) {
            free_dynamic_shadow(obj);
        }
        check_unused_model(obj);
        free(obj->gfx);
    }
    free(obj);
#ifdef PUPPYPRINT_DEBUG
    gNumObjects--;
#endif
}

void free_clutter(Clutter *obj) {
    if (obj->entry == gClutterListHead) {
        if (gClutterListHead->next) {
            gClutterListHead = gClutterListHead->next;
            gClutterListHead->prev = NULL;
        } else {
            gClutterListHead = NULL;
        }
    } else {
        if (obj->entry == gClutterListTail) {
            gClutterListTail = gClutterListTail->prev;
        }
        obj->entry->prev->next = obj->entry->next;
        if (obj->entry->next) {
            obj->entry->next->prev = obj->entry->prev;
        }
    }
    free(obj->entry);
    if (obj->gfx) {
        free(obj->gfx);
    }
    free(obj);
#ifdef PUPPYPRINT_DEBUG
    gNumClutter--;
#endif
}

void free_particle(Particle *obj) {
    if (obj->entry == gParticleListHead) {
        if (gParticleListHead->next) {
            gParticleListHead = gParticleListHead->next;
            gParticleListHead->prev = NULL;
        } else {
            gParticleListHead = NULL;
        }
    } else {
        if (obj->entry == gParticleListTail) {
            gParticleListTail = gParticleListTail->prev;
        }
        obj->entry->prev->next = obj->entry->next;
        if (obj->entry->next) {
            obj->entry->next->prev = obj->entry->prev;
        }
    }
    free(obj->entry);
    if (obj->material) {
        free(obj->material);
    }
    free(obj);
#ifdef PUPPYPRINT_DEBUG
    gNumParticles--;
#endif
}

Object *spawn_object_pos(int objectID, float x, float y, float z) {
    Object *obj = allocate_object();
    if (obj == NULL) {
        return NULL;
    }
    obj->pos[0] = x;
    obj->pos[1] = y;
    obj->pos[2] = z;
    obj->faceAngle[0] = 0;
    obj->faceAngle[1] = 0;
    obj->faceAngle[2] = 0;
    obj->moveAngle[0] = 0;
    obj->moveAngle[1] = 0;
    obj->moveAngle[2] = 0;
    obj->scale[0] = 1.0f;
    obj->scale[1] = 1.0f;
    obj->scale[2] = 1.0f;
    obj->objectID = objectID;
    set_object_functions(obj, objectID);
    return obj;
}

Object *spawn_object_pos_angle(int objectID, float x, float y, float z, short pitch, short roll, short yaw) {
    Object *obj = spawn_object_pos(OBJ_NULL, x, y, z);
    if (obj == NULL) {
        return NULL;
    }
    obj->faceAngle[0] = pitch;
    obj->faceAngle[1] = roll;
    obj->faceAngle[2] = yaw;
    obj->moveAngle[0] = pitch;
    obj->moveAngle[1] = roll;
    obj->moveAngle[2] = yaw;
    obj->objectID = objectID;
    set_object_functions(obj, objectID);
    return obj;
}

Object *spawn_object_pos_angle_scale(int objectID, float x, float y, float z, short pitch, short roll, short yaw, float scaleX, float scaleY, float scaleZ) {
    Object *obj = spawn_object_pos(OBJ_NULL, x, y, z);
    if (obj == NULL) {
        return NULL;
    }
    obj->faceAngle[0] = pitch;
    obj->faceAngle[1] = roll;
    obj->faceAngle[2] = yaw;
    obj->moveAngle[0] = pitch;
    obj->moveAngle[1] = roll;
    obj->moveAngle[2] = yaw;
    obj->scale[0] = scaleX;
    obj->scale[1] = scaleY;
    obj->scale[2] = scaleZ;
    obj->objectID = objectID;
    set_object_functions(obj, objectID);
    return obj;
}

Object *spawn_object_pos_scale(int objectID, float x, float y, float z, float scaleX, float scaleY, float scaleZ) {
    Object *obj = spawn_object_pos(OBJ_NULL, x, y, z);
    if (obj == NULL) {
        return NULL;
    }
    obj->scale[0] = scaleX;
    obj->scale[1] = scaleY;
    obj->scale[2] = scaleZ;
    obj->objectID = objectID;
    set_object_functions(obj, objectID);
    return obj;
}

Clutter *spawn_clutter(int objectID, float x, float y, float z, short pitch, short roll, short yaw) {
    Clutter *clutter = allocate_clutter();
    if (clutter == NULL) {
        return NULL;
    }
    clutter->pos[0] = x;
    clutter->pos[1] = y;
    clutter->pos[2] = z;
    clutter->faceAngle[0] = pitch;
    clutter->faceAngle[1] = roll;
    clutter->faceAngle[2] = yaw;
    clutter->scale[0] = 1.0f;
    clutter->scale[1] = 1.0f;
    clutter->scale[2] = 1.0f;
    clutter->objectID = objectID;
    clutter->gfx = malloc(sizeof(ObjectGraphics));
    clutter->gfx->width = 4.0f;
    clutter->gfx->height = 10.0f;
    clutter->gfx->yOffset = 0;
    return clutter;
}

Particle *spawn_particle(int particleID, float x, float y, float z) {
    Particle *particle = allocate_particle();
    if (particle == NULL) {
        return NULL;
    }
    particle->pos[0] = x;
    particle->pos[1] = y;
    particle->pos[2] = z;
    particle->scale[0] = 1.0f;
    particle->scale[1] = 1.0f;
    particle->scale[2] = 1.0f;
    particle->material = NULL;
    return particle;
}