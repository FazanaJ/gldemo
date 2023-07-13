#pragma once

#include "object_data.h"
#include "assets.h"

enum ObjectFlags {
	OBJ_FLAG_NONE,
	OBJ_FLAG_DELETE = 		(1 << 0),
	OBJ_FLAG_INVISIBLE = 	(1 << 1),
};

enum ObjectIDs {
	OBJ_NULL,
	OBJ_PLAYER,
	OBJ_PROJECTILE,
	OBJ_NPC,
};

enum ClutterIDs {
	CLUTTER_NULL,
	CLUTTER_BUSH,
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
	ObjectModel model;
	char envColour[3];
	char primColour[3];
	char opacity;
} ObjectGraphics;

typedef struct Object {
	ObjectGraphics *gfx;
	void (*loopFunc)(struct Object *obj, int updateRate, float updateRateF);
	struct ObjectList *entry;
	struct Object *parent;
	float cameraDist;
	float viewDist;
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

typedef struct Clutter {
	ObjectGraphics *gfx;
	struct ClutterList *entry;
	float cameraDist;
	float viewDist;
	float pos[3];
	float scale[3];
	u_uint32_t flags;
	uint16_t faceAngle[3];
	short objectID;
} Clutter;

typedef struct ClutterList {
	Clutter *clutter;
	struct ClutterList *next;
	struct ClutterList *prev;
} ClutterList;

typedef struct ParticleInfo {
	short timer;
	float forwardVel;
	float forwardVelIncrease;
	float zVel;
	float zVelIncrease;
	short moveAngle;
	short moveAngleVel;
	short moveAngleVelIncrease;
	float scale[3];
	float scaleIncrease[3];
	unsigned char opacity;

	float posRandom[3];
	float forwardVelRandom;
	float zVelRandom;
	float scaleRandom[3];
	short angleRandom;
	short timerRandom;
	char opacityRandom;
} ParticleInfo;

typedef struct Particle {
	Material *material;
	struct ParticleList *entry;
	float pos[3];
	u_uint32_t flags;

	short timer;
	float forwardVel;
	float forwardVelIncrease;
	float zVel;
	float zVelIncrease;
	short moveAngle;
	short moveAngleVel;
	short moveAngleVelIncrease;
	float scale[3];
	float scaleIncrease[3];
	unsigned char opacity;
} Particle;

typedef struct ParticleList {
	Particle *particle;
	struct ParticleList *next;
	struct ParticleList *prev;
} ParticleList;

extern Object *gPlayer;
extern ObjectList *gObjectListHead;
extern ObjectList *gObjectListTail;
extern ClutterList *gClutterListHead;
extern ClutterList *gClutterListTail;
extern ParticleList *gParticleListHead;
extern ParticleList *gParticleListTail;
extern short gNumObjects;
extern short gNumClutter;
extern short gNumParticles;

Object *allocate_object(void);
Clutter *allocate_clutter(void);
Object *spawn_object_pos(int objectID, float x, float y, float z);
Object *spawn_object_pos_angle(int objectID, float x, float y, float z, short pitch, short roll, short yaw);
Object *spawn_object_pos_angle_scale(int objectID, float x, float y, float z, short pitch, short roll, short yaw, float scaleX, float scaleY, float scaleZ);
Object *spawn_object_pos_scale(int objectID, float x, float y, float z, float scaleX, float scaleY, float scaleZ);
Clutter *spawn_clutter(int objectID, float x, float y, float z, short pitch, short roll, short yaw);
Particle *spawn_particle(int particleID, float x, float y, float z);
void delete_object(Object *obj);
void delete_clutter(Clutter *clutter);
void update_game_entities(int updateRate, float updateRateF);
Object *find_nearest_object(Object *obj, int objectID, float baseDist);
Object *find_nearest_object_facing(Object *obj, int objectID, float baseDist, int range, int angle);