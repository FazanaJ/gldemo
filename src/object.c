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
#include "audio.h"

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

static void object_platform_displacement(Object *obj) {
    Object *p = obj->platform;
    if (obj->pos[1] <= obj->hitboxHeight + 0.5f) {
        float diff[3];
        diff[0] = p->pos[0] - p->prevPos[0];
        diff[1] = p->pos[1] - p->prevPos[1];
        diff[2] = p->pos[2] - p->prevPos[2];
        obj->pos[0] += diff[0];
        obj->pos[1] += diff[1];
        obj->pos[2] += diff[2];
    }
}

static void object_gravity(Object *obj, float updateRateF) {
    //return;
    float weightMax = -(obj->weight * 10.0f);
    if (obj->yVel > weightMax) {
        obj->yVel -= (obj->weight / 10.0f) * updateRateF;
        if (obj->yVel < weightMax) {
            obj->yVel = weightMax;
        }
    }
    if (obj->yVel != 0.0f) {
        float height = MAX(obj->floorHeight, obj->hitboxHeight);
        obj->pos[1] += (obj->yVel / 10.0f) * updateRateF;
        if (obj->yVel < 0.0f && obj->pos[1] - height < 0.0f) {
            obj->pos[1] = height;
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

float gHitboxSize[2][3];
Object *sPrevPlatform;
#define COLLISION_CAP (sizeof(obj->hitbox->collideObj) / sizeof(int *))

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


static int object_hit_platform_flat(Object *obj, Object *testObj) {
    Hitbox *h = obj->hitbox;
    Hitbox *h2 = testObj->hitbox;
    float heightScale = h->offsetY * obj->scale[1];
    float heightScale2 = h2->offsetY * testObj->scale[1];
    if (h2->solid) {
        if (obj->pos[1] + heightScale >= testObj->pos[1] + (heightScale2 + gHitboxSize[1][1]) || 
            (sPrevPlatform == testObj && obj->pos[1] + heightScale >= testObj->pos[1] + heightScale2)) {
            obj->hitboxHeight = testObj->pos[1] + gHitboxSize[1][1] + heightScale2;
            obj->platform = testObj;
            return true;
        }
    }
    // Above or below it, so return.
    if (obj->pos[1] + heightScale + gHitboxSize[0][1] <= testObj->pos[1] + heightScale2 || 
        obj->pos[1] + heightScale >= testObj->pos[1] + heightScale2 + gHitboxSize[1][1]) {
        return true;
    }
    return false;
}

static int object_hit_platform_round(Object *obj, Object *testObj) {
    Hitbox *h = obj->hitbox;
    Hitbox *h2 = testObj->hitbox;
    float heightScale = h->offsetY * obj->scale[1];
    float heightScale2 = h2->offsetY * testObj->scale[1];
    if (h2->solid) {
        float midPoint = (gHitboxSize[1][1] / 2.0f);
        if (obj->pos[1] + heightScale >= testObj->pos[1] + heightScale2 + midPoint || 
            (sPrevPlatform == testObj && obj->pos[1] + heightScale >= testObj->pos[1] + heightScale2)) {
            float relX = fabsf((obj->pos[0]) - (testObj->pos[0]));
            float relZ = fabsf((obj->pos[2]) - (testObj->pos[2]));
            float dist = (SQR(relX) + SQR(relZ)) / ((gHitboxSize[0][0] + gHitboxSize[1][0]) * (gHitboxSize[0][2] + gHitboxSize[1][2]));
            float height = testObj->pos[1] + heightScale2 + (midPoint * (2.0f - dist));
            obj->hitboxHeight = height;
            obj->platform = testObj;
            return true;
        }
    }
    // Above or below it, so return.
    if (obj->pos[1] + heightScale + gHitboxSize[0][1] <= testObj->pos[1] + heightScale2 || 
        obj->pos[1] + heightScale >= testObj->pos[1] + heightScale2 + gHitboxSize[1][1]) {
        return true;
    }
    return false;
}

static void object_hit_block_block(Object *obj, Object *testObj) {
    if (fabsf(obj->pos[0] - testObj->pos[0]) < gHitboxSize[0][0] + gHitboxSize[1][0] &&
        fabsf(obj->pos[2] - testObj->pos[2]) < gHitboxSize[0][2] + gHitboxSize[1][2]) {
        Hitbox *h = obj->hitbox;
        Hitbox *h2 = testObj->hitbox;
        if (object_hit_platform_flat(obj, testObj) == true) {
            return;
        }
    }
}


static void object_hit_block_cylinder(Object *obj, Object *testObj) {
    if (fabsf(obj->pos[0] - testObj->pos[0]) < gHitboxSize[0][0] + gHitboxSize[1][0] &&
        fabsf(obj->pos[2] - testObj->pos[2]) < gHitboxSize[0][2] + gHitboxSize[1][2]) {
        Hitbox *h = obj->hitbox;
        Hitbox *h2 = testObj->hitbox;
        h->collideObj[(int) h->numCollisions++] = testObj;
        if (h2->numCollisions < COLLISION_CAP) {
            h2->collideObj[(int) h2->numCollisions++] = obj;
        }
        if (testObj->flags & OBJ_FLAG_TANGIBLE) {
            if (object_hit_platform_flat(obj, testObj) == true) {
                return;
            }
            float pointX;
            float pointZ;
            if (obj->pos[0] < testObj->pos[0]) { // Left
                pointX = testObj->pos[0] - gHitboxSize[1][0];
            } else { // Right
                pointX = testObj->pos[0] + gHitboxSize[1][0];
            }
            if (obj->pos[2] < testObj->pos[2]) { // Up
                pointZ = testObj->pos[2] - gHitboxSize[1][2];
            } else { // Down
                pointZ = testObj->pos[2] + gHitboxSize[1][2];
            }
            float dist = SQR(obj->pos[0] - pointX) + SQR(obj->pos[2] - pointZ);
            if (dist < h->width * h->length) {

            }
        }
    }
}

static void object_hit_block_sphere(Object *obj, Object *testObj) {
    
}

static void object_hit_cylinder_cylinder(Object *obj, Object *testObj) {
        float relX = (obj->pos[0] - testObj->pos[0]);
        float relZ = (obj->pos[2] - testObj->pos[2]);
        float radiusX = gHitboxSize[0][0] + gHitboxSize[1][0];
        float radiusZ = gHitboxSize[0][2] + gHitboxSize[1][2];
    if (fabsf(relX) < radiusX && fabsf(relZ) < radiusZ) {
        Hitbox *h = obj->hitbox;
        Hitbox *h2 = testObj->hitbox;
        float dist = SQR(relX) + SQR(relZ);
        if (dist > (gHitboxSize[0][0] + gHitboxSize[1][0]) * (gHitboxSize[0][2] + gHitboxSize[1][2])) {
            return;
        }
        dist = sqrtf(dist);
        h->collideObj[(int) h->numCollisions++] = testObj;
        if (h2->numCollisions < COLLISION_CAP) {
            h2->collideObj[(int) h2->numCollisions++] = obj;
        }
        if (testObj->flags & OBJ_FLAG_TANGIBLE) {
            if (object_hit_platform_flat(obj, testObj) == true) {
                return;
            }
            float maxWeight = MAX(h->weight, h2->weight);
            float mag = MAX(h->weight - h2->weight, 0.0f);
            float mag2 = (maxWeight - mag) / maxWeight;
            mag /= maxWeight;
            
            obj->pos[0] += ((radiusX - dist) / radiusX * relX) * mag2;
            obj->pos[2] += ((radiusZ - dist) / radiusZ * relZ) * mag2;

            if (mag != 0.0f) {
                float oldPos[3] = {testObj->pos[0], testObj->pos[1], testObj->pos[2]};
                testObj->pos[0] -= ((radiusX - dist) / radiusX * relX) * mag;
                testObj->pos[2] -= ((radiusZ - dist) / radiusZ * relZ) * mag;
                if (h2->moveSound) {
                    float moveDist = DIST3(oldPos, testObj->pos);
                    moveDist = sqrtf(moveDist);
                    if (moveDist != 0.0f) {
                        play_sound_spatial_pitch(h2->moveSound, testObj->pos, 0.5f + moveDist);
                    }
                }
            }
        }
    }
}

static void object_hit_cylinder_sphere(Object *obj, Object *testObj) {
        float relX = (obj->pos[0] - testObj->pos[0]);
        float relZ = (obj->pos[2] - testObj->pos[2]);
        float radiusX = gHitboxSize[0][0] + gHitboxSize[1][0];
        float radiusY = gHitboxSize[0][1] + gHitboxSize[1][1];
        float radiusZ = gHitboxSize[0][2] + gHitboxSize[1][2];
    if (fabsf(relX) < radiusX && fabsf(relZ) < radiusZ) {
        Hitbox *h = obj->hitbox;
        Hitbox *h2 = testObj->hitbox;
        float dist = SQR(relX) + SQR(relZ);
        if (dist > radiusX * radiusZ) {
            return;
        }
        dist = sqrtf(dist);
        h->collideObj[(int) h->numCollisions++] = testObj;
        if (h2->numCollisions < COLLISION_CAP) {
            h2->collideObj[(int) h2->numCollisions++] = obj;
        }
        if (testObj->flags & OBJ_FLAG_TANGIBLE) {
            if (object_hit_platform_round(obj, testObj) == true) {
                return;
            }
            float maxWeight = MAX(h->weight, h2->weight);
            float mag = MAX(h->weight - h2->weight, 0.0f);
            float midPoint = gHitboxSize[1][1] / 2.0f;
            float relY = ((obj->pos[1] + gHitboxSize[0][1] + (h->offsetY * obj->scale[1])) - testObj->pos[1] + (h2->offsetY * testObj->scale[1]));
            float distV = MIN(relY / (gHitboxSize[0][1] + gHitboxSize[1][1]), 1.0f);
            float mag2 = ((maxWeight - mag) / maxWeight) * distV;
            mag /= maxWeight;
            
            obj->pos[0] += ((radiusX - dist) / radiusX * relX) * mag2;
            obj->pos[2] += ((radiusZ - dist) / radiusZ * relZ) * mag2;

            if (mag != 0.0f) {
                float oldPos[3] = {testObj->pos[0], testObj->pos[1], testObj->pos[2]};
                testObj->pos[0] -= ((radiusX - dist) / radiusX * relX) * mag;
                testObj->pos[2] -= ((radiusZ - dist) / radiusZ * relZ) * mag;
                if (h2->moveSound) {
                    float moveDist = DIST3(oldPos, testObj->pos);
                    moveDist = sqrtf(moveDist);
                    if (moveDist != 0.0f) {
                        play_sound_spatial_pitch(h2->moveSound, testObj->pos, 0.5f + moveDist);
                    }
                }
            }
        }
    }
}

static void object_hit_sphere_sphere(Object *obj, Object *testObj) {
    
}

static void object_hit_cylinder(Object *obj, Object *testObj) {
    switch (testObj->hitbox->type) {
    case HITBOX_BLOCK:
        object_hit_block_cylinder(obj, testObj);
        break;
    case HITBOX_CYLINDER:
        object_hit_cylinder_cylinder(obj, testObj);
        break;
    case HITBOX_SPHERE:
        object_hit_cylinder_sphere(obj, testObj);
        break;
    }
}

static void object_hit_block(Object *obj, Object *testObj) {
    switch (testObj->hitbox->type) {
    case HITBOX_BLOCK:
        object_hit_block_block(obj, testObj);
        break;
    case HITBOX_CYLINDER:
        object_hit_block_cylinder(obj, testObj);
        break;
    case HITBOX_SPHERE:
        object_hit_block_sphere(obj, testObj);
        break;
    }
}

static void object_hit_sphere(Object *obj, Object *testObj) {
    switch (testObj->hitbox->type) {
    case HITBOX_BLOCK:
        object_hit_block_sphere(obj, testObj);
        break;
    case HITBOX_CYLINDER:
        object_hit_cylinder_sphere(obj, testObj);
        break;
    case HITBOX_SPHERE:
        object_hit_sphere_sphere(obj, testObj);
        break;
    }
}

static void (*sObjectHitFuncs[])(Object *, Object *) = {
    object_hit_block,
    object_hit_sphere,
    object_hit_cylinder,
};

static void object_hitbox(Object *obj) {
    DEBUG_SNAPSHOT_1();
    if ((obj->hitbox) == NULL) {
        return;
    }
    ObjectList *objList = gObjectListHead;
    Object *testObj;
    sPrevPlatform = obj->platform;
    obj->hitboxHeight = -30000;
    obj->platform = NULL;
    gHitboxSize[0][0] = obj->hitbox->width * obj->scale[0];
    gHitboxSize[0][1] = obj->hitbox->height * obj->scale[1];
    gHitboxSize[0][2] = obj->hitbox->length * obj->scale[2];
    while (objList) {
        testObj = objList->obj;
        if (testObj->hitbox && obj != testObj) {
            for (int i = 0; i < obj->hitbox->numCollisions; i++) {
                if (testObj == obj->hitbox->collideObj[i] || obj->hitbox->numCollisions >= COLLISION_CAP) {
                    goto next;
                }
            }
            gHitboxSize[1][0] = testObj->hitbox->width * testObj->scale[0];
            gHitboxSize[1][1] = testObj->hitbox->height * testObj->scale[1];
            gHitboxSize[1][2] = testObj->hitbox->length * testObj->scale[2];
            sObjectHitFuncs[(int) obj->hitbox->type](obj, testObj);
        }
        next:
        objList = objList->next;
    }
    get_time_snapshot(PP_HITBOXES, DEBUG_SNAPSHOT_1_END);
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
        obj->prevPos[0] = obj->pos[0];
        obj->prevPos[1] = obj->pos[1];
        obj->prevPos[2] = obj->pos[2];
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
    // Reset the list and run platform displacement
    objList = gObjectListHead;
    while (objList) {
        DEBUG_SNAPSHOT_2();
        obj = objList->obj;
        if (obj->platform) {
            object_platform_displacement(obj);
        }
        objList = objList->next;
        if ((obj->flags & OBJ_FLAG_DELETE) == false) {
            get_obj_snapshot(obj, DEBUG_SNAPSHOT_2_END, true);
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