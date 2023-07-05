#include <libdragon.h>
#include <malloc.h>

#include "object.h"
#include "../include/global.h"

#include "debug.h"
#include "player.h"

ObjectList *gObjectListHead = NULL;
ObjectList *gObjectListTail = NULL;
short gNumObjects = 0;

/**
 * Check if gObjectListHead is null first.
 * If it is, set that as the new list element, otherwise, make a new element and join it onto the tail.
*/
Object *allocate_object(void) {
    Object *newObj = malloc(sizeof(Object));

    if (gObjectListHead == NULL) {
        gObjectListHead = malloc(sizeof(ObjectList));
        gObjectListHead->next = NULL;
        gObjectListHead->prev = NULL;
        gObjectListTail = gObjectListHead;
        gObjectListHead->obj = newObj;
    } else {
        ObjectList *list = malloc(sizeof(ObjectList));
        gObjectListTail->next = list;
        list->prev = gObjectListTail;
        gObjectListTail = list;
        list->next = NULL;
        list->obj = newObj;
    }
    newObj->entry = gObjectListTail;
    newObj->gfx = NULL;
    newObj->data = NULL;

    gNumObjects++;

    return newObj;
}

static void (*gObjectInits[])(struct Object *obj) = {
    NULL,
    player_init,
    0,
};

static void (*gObjectLoops[])(struct Object *obj, int updateRate, float updateRateF) = {
    0,
    player_loop,
    0,
};

static const unsigned short gObjectData[] = {
    0,
    sizeof(PlayerData),
    0,
};

static void set_object_functions(Object *obj, int objectID) {
    void (*initFunc)(struct Object *obj) = gObjectInits[objectID];
    if (gObjectData[objectID]) {
        obj->data = malloc(gObjectData[objectID]);
    }
    if (initFunc != NULL) {
        (*initFunc)(obj);
    }
    obj->loopFunc = gObjectLoops[objectID];
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
    if (objectID != OBJ_NULL) {
        set_object_functions(obj, objectID);
    }
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
    set_object_functions(obj, objectID);
    return obj;
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
        if (obj->entry->prev) {
            obj->entry->next->prev = obj->entry->prev;
        }
        if (obj->entry->next) {
            obj->entry->prev->next = obj->entry->next;
        }
    }
    free(obj->entry);
    if (obj->data) {
        free(obj->data);
    }
    if (obj->gfx) {
        free(obj->gfx);
    }
    free(obj);
    gNumObjects--;
}

/**
 * Remove every existing object.
 * gObjectListHead is updated every time it's removed, so keep removing it until it reads null.
*/
void clear_objects(void) {
    while (gObjectListHead) {
        free_object(gObjectListHead->obj);
    }
}

/**
 * Loop through every element in the object list and run their loop function.
*/
void update_objects(int updateRate, float updateRateF) {
    DEBUG_SNAPSHOT_1();
    if (gObjectListHead == NULL) {
        get_time_snapshot(PP_OBJECTS, DEBUG_SNAPSHOT_1_END);
        return;
    }
    ObjectList *list = gObjectListHead;
    Object *obj;

    while (list) {
        obj = list->obj;
        if (obj->loopFunc) {
            (list->obj->loopFunc)(list->obj, updateRate, updateRateF);
        }
        list = list->next;
    }
    get_time_snapshot(PP_OBJECTS, DEBUG_SNAPSHOT_1_END);
}
