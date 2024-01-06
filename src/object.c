#include <libdragon.h>
#include <malloc.h>

#include "object.h"
#include "../include/global.h"

#include "debug.h"
#include "math_util.h"
#include "main.h"
#include "camera.h"
#include "menu.h"
#include "collision.h"
#include "talk.h"

ObjectList *gObjectListHead = NULL;
ObjectList *gObjectListTail = NULL;
ClutterList *gClutterListHead = NULL;
ClutterList *gClutterListTail = NULL;
ParticleList *gParticleListHead = NULL;
ParticleList *gParticleListTail = NULL;
VoidList *gOverlayListHead = NULL;
VoidList *gOverlayListTail = NULL;
ModelList *gModelIDListHead = NULL;
ModelList *gModelIDListTail = NULL;
Object *gPlayer;
#ifdef PUPPYPRINT_DEBUG
short gNumObjects = 0;
short gNumClutter = 0;
short gNumParticles = 0;
short gNumModels = 0;
short gNumOverlays = 0;
#endif
char gGamePaused = false;

char *sObjectOverlays[] = {
    NULL,
    "player",
    "projectile",
    "npc",
};

char *gModelIDs[] = {
    "humanoid",
    "rock",
    "bottombillboard"
};

short gObjectModels[] = {
    0,
    1,
    3,
    1,
};

void init_object_behaviour(Object *obj, int objectID) {
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
        debugf("Loading overlay [%s]\n", sObjectOverlays[objectID]);
#ifdef PUPPYPRINT_DEBUG
        gNumOverlays++;
#endif
    }
    ObjectEntry *entry = dlsym(addr, "entry");
    obj->loopFunc = entry->loopFunc;
    obj->flags = entry->flags;
    obj->overlay = list;
    if (entry->viewDist) {
        obj->viewDist = entry->viewDist << 4;
    } else {
        obj->viewDist = 500.0f;
    }
    if (entry->data) {
        obj->data = malloc(entry->data);
        bzero(obj->data, entry->data);
    }
    if (entry->initFunc) {
        (*entry->initFunc)(obj);
    }
    //*(volatile int *) 0 = 0;
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
    debugf("Freed model [%s]\n", gModelIDs[obj->gfx->modelID - 1]);
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
    debugf("Freed overlay [%s]\n", sObjectOverlays[overlay->id]);
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

void object_move(Object *obj, float updateRateF) {
    if (obj->forwardVel != 0.0f) {
        obj->pos[0] += ((obj->forwardVel * sins(obj->moveAngle[1])) / 20.0f) * updateRateF;
        obj->pos[2] += ((obj->forwardVel * coss(obj->moveAngle[1])) / 20.0f) * updateRateF;
    }
}

void object_gravity(Object *obj, float updateRateF) {
    return;
    float weightMax = -(obj->weight * 10.0f);
    if (obj->yVel > weightMax) {
        obj->yVel -= (obj->weight) * updateRateF;
        if (obj->yVel < weightMax) {
            obj->yVel = weightMax;
        }
    }
    if (obj->yVel != 0.0f) {
        obj->pos[1] += (obj->yVel / 10.0f) * updateRateF;
        if (obj->yVel < 0.0f && obj->pos[1] - obj->floorHeight < 0.0f) {
            obj->pos[1] = obj->floorHeight;
            obj->yVel = 0.0f;
        }
    }
}

int temp_matrix_grabber(int modelID) {
    switch (modelID) {
    case 1:
        return MTX_TRANSLATE_ROTATE_SCALE;
    case 2:
        return MTX_TRANSLATE_ROTATE_SCALE;
    case 3:
        return MTX_BILLBOARD;
    }
    return MTX_TRANSLATE;
}

short playerModelTextures[6][4] = {
    {TEXTURE_NONE, 0, 0, 0},
    {TEXTURE_NONE, 0, 0, 0},
    {TEXTURE_NONE, 0, 0, 0},
    {TEXTURE_NONE, TEXTURE_NONE, TEXTURE_NONE, TEXTURE_EYE1},
    {TEXTURE_TROUSERS, 0, 0, 0},
    {TEXTURE_NONE, TEXTURE_SHIRT, 0, 0}
};

short playerModelFlags[6][4] = {
    {MATERIAL_VTXCOL, 0, 0, 0},
    {MATERIAL_VTXCOL, 0, 0, 0},
    {MATERIAL_VTXCOL, 0, 0, 0},
    {MATERIAL_VTXCOL, MATERIAL_VTXCOL, MATERIAL_VTXCOL, MATERIAL_CUTOUT | MATERIAL_DECAL},
    {0, 0, 0, 0},
    {MATERIAL_VTXCOL, 0, 0, 0}
};

void load_object_model(Object *obj, int objectID) {
    obj->gfx = malloc(sizeof(ObjectGraphics));
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
            } else {
                m->material.flags = MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_LIGHTING | MATERIAL_VTXCOL;
                m->material.textureID = -1;
            }
            m->material.index = NULL;
            m->next = NULL;
            if (i == 0 && j == 0) {
                m->matrixBehaviour = matrixType;
            } else {
                m->matrixBehaviour = 0;
            }
            rspq_block_begin();
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

#ifdef PUPPYPRINT_DEBUG
        gNumModels++;
#endif
    debugf("Loading model [%s]\n", gModelIDs[modelID - 1]);
    list->next = NULL;
    list->timer = 10;
    list->id = modelID;
}

static void set_object_functions(Object *obj, int objectID) {
    if (sObjectOverlays[objectID]) {
        init_object_behaviour(obj, objectID);
    }
    if (gObjectModels[objectID]) {
        load_object_model(obj, objectID);
    }
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

void free_dynamic_shadow(Object *obj) {
    surface_free(&obj->dynamic);
    glDeleteTextures(1, &obj->dynamicTex);
    obj->dynamicStaleTimer = 0;
    obj->dynamicExists = false;
    debugf("Freeing dynamic shadow for [%s] object.\n", sObjectOverlays[obj->objectID]);
}

/**
 * Remove an object from the list, then reconnect the list.
 * Free the list entry, then the object, and any further elements from RAM.
*/
static void free_object(Object *obj) {
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
    if (obj->dynamicExists) {
        free_dynamic_shadow(obj);
    }
    free(obj->entry);
    if (obj->data) {
        free(obj->data);
    }
    if (obj->gfx) {
        check_unused_model(obj);
        free(obj->gfx);
    }
    free(obj);
#ifdef PUPPYPRINT_DEBUG
    gNumObjects--;
#endif
}

static void free_clutter(Clutter *obj) {
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

static void free_particle(Particle *obj) {
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

/**
 * Mark Object for deletion. It will be removed when it's finished updating.
*/
void delete_object(Object *obj) {
    obj->flags |= OBJ_FLAG_DELETE;
}

/**
 * Mark clutter for deletion. It will be removed when it's finished updating.
*/
void delete_clutter(Clutter *clutter) {
    clutter->flags |= OBJ_FLAG_DELETE;
}

/**
 * Remove every existing object.
 * gObjectListHead is updated every time it's removed, so keep removing it until it reads null.
*/
void clear_objects(void) {
    while (gObjectListHead) {
        free_object(gObjectListHead->obj);
    }
    while (gClutterListHead) {
        free_clutter(gClutterListHead->clutter);
    }
    while (gParticleListHead) {
        free_particle(gParticleListHead->particle);
    }
}

Object *find_nearest_object(Object *obj, int objectID, float baseDist) {
    float bestDist = SQR(baseDist);
    ObjectList *objList = gObjectListHead;
    Object *listObj;
    Object *bestObj = NULL;
    float dist;

    while (objList) {
        listObj = objList->obj;
        if (listObj->objectID == objectID) {
            dist = DIST3(obj->pos, listObj->pos);
            if (dist < bestDist) {
                bestObj = listObj;
                bestDist = dist;
            }
        }
        objList = objList->next;
    }

    return bestObj;
}

Object *find_nearest_object_facing(Object *obj, int objectID, float baseDist, int range, int angle) {
    float bestDist = SQR(baseDist);
    ObjectList *objList = gObjectListHead;
    Object *listObj;
    Object *bestObj = NULL;
    float dist;

    while (objList) {
        listObj = objList->obj;
        if (listObj->objectID == objectID) {
            short rot = (((angle) % 0xFFFF - 0x8000) - (atan2s(obj->pos[2] - listObj->pos[2], obj->pos[0] - listObj->pos[0]) + 0x8000) % 0xFFFF - 0x8000);
            if (fabs(rot) < range) {
                dist = DIST3(obj->pos, listObj->pos);
                if (dist < bestDist) {
                    bestObj = listObj;
                    bestDist = dist;
                }
            }
        }
        objList = objList->next;
    }

    return bestObj;
}

/**
 * Loop through every element in the object list and run their loop function.
*/
static void update_objects(int updateRate, float updateRateF) {
    DEBUG_SNAPSHOT_1();
    if (gObjectListHead == NULL) {
        get_time_snapshot(PP_OBJECTS, DEBUG_SNAPSHOT_1_END);
        return;
    }
    ObjectList *objList = gObjectListHead;
    Object *obj;

    DEBUG_GET_TIME_1(PP_PLAYER);
    while (objList) {
        obj = objList->obj;
        if (obj->dynamicStaleTimer > 0 && obj->dynamicExists) {
            obj->dynamicStaleTimer -= updateRate;
            if (obj->dynamicStaleTimer <= 0) {
                free_dynamic_shadow(obj);
            }
        }
        obj->cameraDist = DIST3(obj->pos, gCamera->pos);
        if (obj->cameraDist < obj->viewDist) {
            obj->flags &= ~OBJ_FLAG_INVISIBLE;
        } else {
            obj->flags |= OBJ_FLAG_INVISIBLE;
        }
        if (obj->loopFunc) {
            (objList->obj->loopFunc)(objList->obj, updateRate, updateRateF);
        }
        if (obj->flags & OBJ_FLAG_MOVE) {
            object_move(obj, updateRateF);
        }
        if (obj->flags & OBJ_FLAG_GRAVITY) {
            object_gravity(obj, updateRateF);
        }
        if (obj->flags & OBJ_FLAG_COLLISION) {
            object_collide(obj);
        }
        objList = objList->next;
        if (obj->flags & OBJ_FLAG_DELETE) {
            free_object(obj);
        }
    }
    get_time_snapshot(PP_OBJECTS, DEBUG_SNAPSHOT_1_END);
    add_time_offset(PP_OBJECTS, DEBUG_GET_TIME_1_END(PP_PLAYER));
}

static void update_clutter(int updateRate, float updateRateF) {
    DEBUG_SNAPSHOT_1();
    if (gClutterListHead == NULL) {
        get_time_snapshot(PP_CLUTTER, DEBUG_SNAPSHOT_1_END);
        return;
    }
    ClutterList *clutterList = gClutterListHead;
    Clutter *clutter;

    while (clutterList) {
        clutter = clutterList->clutter;
        clutter->cameraDist = DIST3(clutter->pos, gCamera->pos);
        if (clutter->cameraDist <= clutter->viewDist) {
            clutter->flags &= ~OBJ_FLAG_INVISIBLE;
        } else {
            clutter->flags |= OBJ_FLAG_INVISIBLE;
        }
        clutterList = clutterList->next;
        if (clutter->flags & OBJ_FLAG_DELETE) {
            free_clutter(clutter);
        }
    }
    get_time_snapshot(PP_CLUTTER, DEBUG_SNAPSHOT_1_END);
}

static void update_particles(int updateRate, float updateRateF) {
    DEBUG_SNAPSHOT_1();
    if (gParticleListHead == NULL) {
        get_time_snapshot(PP_PARTICLES, DEBUG_SNAPSHOT_1_END);
        return;
    }
    ParticleList *particleList = gParticleListHead;
    Particle *particle;

    while (particleList) {
        particle = particleList->particle;

        particle->timer -= updateRate;
        if (particle->timer <= 0) {
            particle->flags |= OBJ_FLAG_DELETE;
        } else {
            particle->forwardVel += particle->forwardVelIncrease * updateRateF;
            if (particle->forwardVel < 0.0f) {
                particle->forwardVel = 0.0f;
            }
            particle->yVel += particle->yVelIncrease * updateRateF;
            particle->moveAngleVel += particle->moveAngleVelIncrease * updateRateF;
            particle->moveAngle += particle->moveAngleVel * updateRateF;
            for (int i = 0; i < 3; i++) {
                particle->scale[i] += particle->scaleIncrease[i] * updateRateF;
            }
            float velX = particle->forwardVel * coss(particle->moveAngle);
            float velZ = particle->forwardVel * sins(particle->moveAngle);

            particle->pos[0] += velX * updateRateF;
            particle->pos[2] += velZ * updateRateF;
            particle->pos[1] += particle->yVel * updateRateF;
        }

        particleList = particleList->next;
        if (particle->flags & OBJ_FLAG_DELETE) {
            free_particle(particle);
        }
    }
    get_time_snapshot(PP_PARTICLES, DEBUG_SNAPSHOT_1_END);
}

void update_game_entities(int updateRate, float updateRateF) {
    if (gMenuStatus == MENU_CLOSED) {
        update_objects(updateRate, updateRateF);
        update_clutter(updateRate, updateRateF);
        update_particles(updateRate, updateRateF);
    }
    camera_loop(updateRate, updateRateF);
}