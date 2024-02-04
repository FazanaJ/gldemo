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
#include "scene.h"
#include "assets.h"

ObjectList *gObjectListHead = NULL;
ObjectList *gObjectListTail = NULL;
ClutterList *gClutterListHead = NULL;
ClutterList *gClutterListTail = NULL;
ParticleList *gParticleListHead = NULL;
ParticleList *gParticleListTail = NULL;
Object *gPlayer;
#ifdef PUPPYPRINT_DEBUG
short gNumObjects = 0;
short gNumClutter = 0;
short gNumParticles = 0;
short gNumModels = 0;
short gNumOverlays = 0;
#endif
char gGamePaused = false;

static void object_move(Object *obj, float updateRateF) {
    if (obj->forwardVel != 0.0f) {
        float vel = obj->forwardVel / 20.0f;
        obj->pos[0] += (vel * sins(obj->moveAngle[1])) * updateRateF;
        obj->pos[2] += (vel * coss(obj->moveAngle[1])) * updateRateF;
    }
}

static void object_gravity(Object *obj, float updateRateF) {
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

#define COLLISION_CAP (sizeof(obj->hitbox->collideObj) / sizeof(int *))

static void object_hitbox(Object *obj) {
    DEBUG_SNAPSHOT_1();
    ObjectList *objList = gObjectListHead;
    Object *testObj;
    while (objList) {
        testObj = objList->obj;
        if (obj->hitbox && obj != testObj) {
            Hitbox *h = obj->hitbox;
            Hitbox *h2 = testObj->hitbox;
            for (int i = 0; i < h->numCollisions; i++) {
                if (testObj == h->collideObj[i] || h->numCollisions >= COLLISION_CAP) {
                    goto next;
                }
            }
            if (fabsf(obj->pos[0] - testObj->pos[0]) < h->width + h2->width &&
                fabsf(obj->pos[2] - testObj->pos[2]) < h->length + h2->length) {
                h->collideObj[(int) h->numCollisions++] = testObj;
                if (h2->numCollisions < COLLISION_CAP) {
                    h2->collideObj[(int) h2->numCollisions++] = obj;
                }
                if (testObj->flags & OBJ_FLAG_TANGIBLE) {
                    float maxWeight = MAX(h->weight, h2->weight);
                    float mag = MAX(h->weight - h2->weight, 0.0f);
                    float mag2 = (maxWeight - mag) / maxWeight;
                    mag /= maxWeight;
                    float radiusX = h->width + h2->width;
                    float radiusZ = h->length + h2->length;
                    float dist;
                    float relX;
                    float relZ;
                    //debugf("1: %2.2f, 2: %2.2f\n", mag, mag2);
                    if (mag != 0.0f || mag2 != 0.0f) {
                        relX = (obj->pos[0] - testObj->pos[0]);
                        relZ = (obj->pos[2] - testObj->pos[2]);
                        dist = sqrtf(SQR(relX) + SQR(relZ));
                    }
                    
                    if (mag2 != 0.0f) {
                        obj->pos[0] += ((radiusX - dist) / radiusX * relX) * mag2;
                        obj->pos[2] += ((radiusZ - dist) / radiusZ * relZ) * mag2;
                    }

                    if (mag != 0.0f) {
                        testObj->pos[0] -= ((radiusX - dist) / radiusX * relX) * mag;
                        testObj->pos[2] -= ((radiusZ - dist) / radiusZ * relZ) * mag;
                    }
                    
                }
            }
        }
        next:
        objList = objList->next;
    }
    get_time_snapshot(PP_COLLISION, DEBUG_SNAPSHOT_1_END);
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
    // Run object specific functions first
    while (objList) {
        DEBUG_SNAPSHOT_2();
        obj = objList->obj;
        if (obj->loopFunc && (obj->flags & OBJ_FLAG_DELETE) == false && (obj->flags & OBJ_FLAG_INACTIVE) == false) {
            (objList->obj->loopFunc)(objList->obj, updateRate, updateRateF);
        }
        if (obj->hitbox) {
            obj->hitbox->numCollisions = 0;
        }
        objList = objList->next;
        if ((obj->flags & OBJ_FLAG_DELETE) == false) {
            get_obj_snapshot(obj, DEBUG_SNAPSHOT_2_END, true);
        }
    }
    // Reset the list and run generic functions.
    objList = gObjectListHead;
    while (objList) {
        DEBUG_SNAPSHOT_2();
        obj = objList->obj;
        if (obj->flags & OBJ_FLAG_INACTIVE) {
            goto skipObject;
        }
        if (obj->gfx && obj->gfx->dynamicShadow) {
            DynamicShadow *d = obj->gfx->dynamicShadow;
            if (d->staleTimer > 0) {
                d->staleTimer -= updateRate;
                if (d->staleTimer <= 0) {
                    free_dynamic_shadow(obj);
                }
            }
        }
        obj->cameraDist = DIST3(obj->pos, gCamera->pos);
        if (obj->cameraDist <= obj->viewDist) {
            obj->flags &= ~OBJ_FLAG_INVISIBLE;
        } else {
            obj->flags |= OBJ_FLAG_INVISIBLE;
        }
        if (obj->flags & OBJ_FLAG_MOVE) {
            object_move(obj, updateRateF);
        }
        if (obj->flags & OBJ_FLAG_GRAVITY) {
            object_gravity(obj, updateRateF);
        }
        if (obj->hitbox && obj->flags & OBJ_FLAG_TANGIBLE) {
            object_hitbox(obj);
        }
        if (obj->flags & OBJ_FLAG_COLLISION) {
            object_collide(obj);
        }
        skipObject:
        objList = objList->next;
        if (obj->flags & OBJ_FLAG_DELETE) {
            free_object(obj);
        } else {
            get_obj_snapshot(obj, DEBUG_SNAPSHOT_2_END, false);
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
    if (gSceneUpdate == 0 || gMenuStatus != MENU_CLOSED || gCamera->mode == CAMERA_PHOTO) {
        gSceneUpdate = 1;
    } else {
        update_objects(updateRate, updateRateF);
        update_clutter(updateRate, updateRateF);
        update_particles(updateRate, updateRateF);
    }
    camera_loop(updateRate, updateRateF);
}