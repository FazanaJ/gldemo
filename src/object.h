#pragma once

#include "object_data.h"
#include "assets.h"
#include "render.h"

#define OBJ_DIST(x) (x >> 4)

enum ObjectFlags {
	OBJ_FLAG_NONE,
	OBJ_FLAG_DELETE = 			(1 << 0),
	OBJ_FLAG_INVISIBLE = 		(1 << 1),
	OBJ_FLAG_MOVE = 			(1 << 2),
	OBJ_FLAG_GRAVITY = 			(1 << 3),
	OBJ_FLAG_COLLISION = 		(1 << 4),
	OBJ_FLAG_SHADOW = 			(1 << 5),
	OBJ_FLAG_SHADOW_DYNAMIC =	(1 << 6),
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
	CLUTTER_ROCK,
};

enum MatrixTypes {
	MTX_NONE,
	MTX_TRANSLATE,
	MTX_ROTATE,
	MTX_TRANSLATE_ROTATE_SCALE,
	MTX_BILLBOARD,
	MTX_BILLBOARD_SCALE,

	MTX_PUSH = 0xE0,
	MTX_POP = 0xF0
};

typedef struct DynamicShadow {
	surface_t surface;
	unsigned short texW;
	unsigned short texH;
	float planeW;
	float planeH;
	float offset;
	unsigned char texCount;
	signed char staleTimer;
	u_uint16_t angle[3];
	GLuint tex[9];
} DynamicShadow;

typedef struct ObjectModel {
	Material material;
	struct ObjectModel *next;
	void (*func)(struct Object *obj, int updateRate, float updateRateF);
	rspq_block_t *block;
	char matrixBehaviour;
	char pad[3];
} ObjectModel;

typedef struct ObjectGraphics {
	struct ModelList *listEntry;
	char envColour[3];
	char primColour[3];
	char opacity;
	short modelID;
	DynamicShadow *dynamicShadow;
} ObjectGraphics;

typedef struct Object {
	ObjectGraphics *gfx;
	struct VoidList *overlay;
	void (*loopFunc)(struct Object *obj, int updateRate, float updateRateF);
	struct ObjectList *entry;
	struct Object *parent;
	float cameraDist;
	float viewDist;
	float pos[3];
	float yVel;
	float yVelMax;
	float weight;
	float scale[3];
	float forwardVel;
	u_uint32_t flags;
	uint16_t faceAngle[3];
	uint16_t moveAngle[3];
	void *data;
	short objectID;
	short floorHeight;
	unsigned char animID;
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
	float yVel;
	float yVelIncrease;
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

typedef struct VoidList {
	void *addr;
	struct VoidList *next;
	struct VoidList *prev;
	short id;
	short timer;
} VoidList;

typedef struct ModelList {
	void *model64;
	ObjectModel *entry;
	struct ModelList *next;
	struct ModelList *prev;
	short id;
	short timer;
} ModelList;

typedef struct ObjectEntry {
    void (*initFunc)(Object *obj);
    void (*loopFunc)(Object *obj, int updateRate, float updateRateF);
    unsigned short data;
    unsigned short flags;
	unsigned char viewDist;
	unsigned char viewWidth;
	unsigned char viewHeight;
	unsigned char pad;
} ObjectEntry;

extern char *sObjectOverlays[];
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
extern short gNumModels;
extern short gNumOverlays;

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
void object_move(Object *obj, float updateRateF);
void clear_objects(void);
void free_dynamic_shadow(Object *obj);
