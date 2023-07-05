#pragma once

#include <libdragon.h>
#include <math.h>

#include "math.h"
#include "time.h"

enum ObjectIDs {
	OBJ_NULL,
	OBJ_PLAYER,
	OBJ_BUSH,
};

typedef struct Mesh {
	char b;
} Mesh;

typedef struct ObjectModel {
	struct ObjectModel *next;
	sprite_t *texture;
	Mesh *mesh;
	void *matrix;
} ObjectModel;

typedef struct ObjectGraphics {
	char envColour[3];
	char primColour[3];
	char opacity;
} ObjectGraphics;

typedef struct PlayerData {
	short health;
	short healthBase;
	short healthMax;
} PlayerData;

typedef struct Object {
	ObjectGraphics *gfx;
	void (*loopFunc)(struct Object *obj, int updateRate, float updateRateF);
	struct ObjectList *entry;
	float pos[3];
	float vel[3];
	float scale[3];
	float forwardVel;
	u_uint32_t flags;
	uint16_t faceAngle[3];
	uint16_t moveAngle[3];
	void *data;
	short objectID;
} Object;

typedef struct ObjectList {
	Object *obj;
	struct ObjectList *next;
	struct ObjectList *prev;
} ObjectList;

extern Object *gPlayer;
extern ObjectList *gObjectListHead;
extern ObjectList *gObjectListTail;
extern short gNumObjects;

Object *allocate_object(void);
Object *spawn_object_pos(int objectID, float x, float y, float z);
Object *spawn_object_pos_angle(int objectID, float x, float y, float z, short pitch, short roll, short yaw);
Object *spawn_object_pos_angle_scale(int objectID, float x, float y, float z, short pitch, short roll, short yaw, float scaleX, float scaleY, float scaleZ);
Object *spawn_object_pos_scale(int objectID, float x, float y, float z, float scaleX, float scaleY, float scaleZ);
void free_object(Object *obj);
void update_objects(int updateRate, float updateRateF);
