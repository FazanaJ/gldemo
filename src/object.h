#pragma once

#include <t3d/t3danim.h>
#include <t3d/t3dskeleton.h>

#include "../include/global.h"
#include "object_data.h"
#include "render.h"

#define OBJ_DIST(x) (x >> 4)
#define FLOOR_MARGIN 12.0f

enum ObjectFlags {
	OBJ_FLAG_NONE,
	OBJ_FLAG_DELETE = 			(1 << 0),
	OBJ_FLAG_INVISIBLE = 		(1 << 1),
	OBJ_FLAG_MOVE = 			(1 << 2),
	OBJ_FLAG_GRAVITY = 			(1 << 3),
	OBJ_FLAG_COLLISION = 		(1 << 4),
	OBJ_FLAG_SHADOW = 			(1 << 5),
	OBJ_FLAG_SHADOW_DYNAMIC =	(1 << 6),
	OBJ_FLAG_IN_VIEW =			(1 << 7),
	OBJ_FLAG_TANGIBLE =			(1 << 8),
	OBJ_FLAG_INACTIVE =			(1 << 9),
	OBJ_FLAG_OUTLINE =			(1 << 10),
};

enum ObjectIDs {
	OBJ_NULL,
	OBJ_PLAYER,
	OBJ_PROJECTILE,
	OBJ_NPC,
	OBJ_CRATE,
	OBJ_BARREL,
	OBJ_TESTSPHERE,
	OBJ_LIGHTSOURCE,

	OBJ_TOTAL
};

enum ClutterIDs {
	CLUTTER_NULL,
	CLUTTER_BUSH,
	CLUTTER_FLIPBOOKTEST,
	CLUTTER_ROCK,

	CLUTTER_TOTAL
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

enum HitboxTypes {
	HITBOX_BLOCK,
	HITBOX_SPHERE,
	HITBOX_CYLINDER
};

typedef struct Hitbox {
	unsigned type : 4;
	unsigned numNear : 4;
	unsigned numCollisions : 4;
	unsigned solid : 4;
	unsigned short moveSound;
	float offsetX;
	float offsetY;
	float offsetZ;
	float width;
	float length;
	float height;
	float weight;
	struct Object *collideObj[4]; // Objects actively collided with
	struct Object *nearObj[8]; // Objects reasonably near
} Hitbox;

typedef struct DynamicShadowData {
	unsigned short texW;
	unsigned short texH;
	float planeW;
	float planeH;
	float offset;
	float camPos[3];
	float camFocus[3];
} DynamicShadowData;

typedef struct DynamicShadow {
	surface_t surface;
	unsigned short texW;
	unsigned short texH;
	float planeW;
	float planeH;
	float offset;
	unsigned char acrossX;
	unsigned char acrossY;
	unsigned char texCount;
	signed char staleTimer;
	unsigned short angle[3];
	float camPos[3];
	float camFocus[3];
    T3DMat4FP *mtx[2];
    T3DVertPacked *verts;
	rspq_block_t *block;
} DynamicShadow;

typedef struct ObjectModel {
	Material *material;
	struct ObjectModel *next;
	void (*func)(struct Object *obj, int updateRate, float updateRateF);
	rspq_block_t *block;
	primitive_t *prim;
	color_t colour;
	short materialID;
	char matrixBehaviour;
} ObjectModel;

typedef struct ObjectGraphics {
	struct ModelList *listEntry;
	char envColour[3];
	char primColour[3];
	char opacity;
	char pad;
	short modelID;
	short pad2;
	DynamicShadow *dynamicShadow;
	float width;
	float height;
	float yOffset;
} ObjectGraphics;

typedef struct ObjectCollision {
	struct Object *platform;
	float hitboxHeight;
	float floorHeight;
	float floorNorm;
	short floorFlags;
	char grounded;
} ObjectCollision;

typedef struct ObjectMovement {
	float vel[3];
	float yVelMax;
	float forwardVel;
	float weight;
	unsigned short moveAngle[3];
	// pad: 0x02
} ObjectMovement;

typedef struct ObjectAnimation {
	T3DAnim *inst;
	T3DSkeleton skeleton;
	T3DSkeleton skelBlend;
	char *name[2];
	float animBlend;
	float speed[2];
	float framePrev[2];
	unsigned short id[2];
	unsigned short idPrev[2];
	char stage[2];
} ObjectAnimation;

typedef struct Object {
	ObjectGraphics *gfx;
	float pos[3];
	Hitbox *hitbox;
	struct ObjectList *entry;
	unsigned short faceAngle[3];
	short objectID;
	struct ObjectEntry *header;
	void *overlay;
	void (*loopFunc)(struct Object *obj, int updateRate, float updateRateF);
	struct Object *parent;
	float viewDist;
	float cameraDist;
	float prevPos[3];
	float scale[3];
	unsigned int flags;
	void *data;
	ObjectCollision *collision;
	ObjectMovement *movement;
	ObjectAnimation *animation;
	
	char overlayTimer;
} Object;

typedef struct ObjectList {
	Object *obj;
	struct ObjectList *next;
	struct ObjectList *prev;
} ObjectList;

typedef struct Clutter {
	ObjectGraphics *gfx;
	float pos[3];
	Hitbox *hitbox;
	struct ClutterList *entry;
	unsigned short faceAngle[3];
	short objectID;
	float cameraDist;
	float viewDist;
	float scale[3];
	unsigned int flags;
	T3DMat4FP matrix;
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
	//Material *material;
	struct ParticleList *entry;
	float pos[3];
	unsigned int flags;

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

typedef struct ModelList {
	T3DModel *model64;
	ObjectModel *entry;
	struct ModelList *next;
	struct ModelList *prev;
	T3DChunkAnim **animData;
	short id;
	char timer;
	char active;
} ModelList;

typedef struct ObjectEntry {
    void (*initFunc)(Object *obj);
    void (*loopFunc)(Object *obj, int updateRate, float updateRateF);
#ifdef PUPPYPRINT_DEBUG
	char *name;
#endif
    unsigned short data;
    unsigned short flags;
	unsigned short viewDist;
	unsigned short viewWidth;
	unsigned short viewHeight;
	unsigned char pad;
	DynamicShadowData *dynamicShadow;
	Hitbox *hitbox;
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

void delete_object(Object *obj);
void delete_clutter(Clutter *clutter);
void update_game_entities(int updateRate, float updateRateF);
Object *find_nearest_object(Object *obj, int objectID, float baseDist);
Object *find_nearest_object_facing(Object *obj, int objectID, float baseDist, int range, int angle);
void clear_objects(void);
void free_dynamic_shadow(Object *obj);
void object_footsteps(int stepID, float pos[3]);