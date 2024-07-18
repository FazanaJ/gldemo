#pragma once

#include "../include/global.h"
#include "assets.h"
#include "collision.h"

#define MAP_OBJ 0
#define MAP_CLU 1

#define WORLD_SCALE 0.5328f

enum SceneNames {
    SCENE_INTRO,
    SCENE_TESTAREA,
    SCENE_TESTAREA2,
    SCENE_TESTAREA3,

    SCENE_TOTAL
};

enum EnvironmentFlags {
    ENV_NONE,
    ENV_FOG = (1 << 0),
    ENV_SKYBOX_TOP = (1 << 1),
    ENV_SKYBOX_BOTTOM = (1 << 2),
};

enum MeshFlags {
    MESH_INTANGIBLE = (1 << 0),
    MESH_INVISIBLE = (1 << 1),
    MESH_CAMERA_ONLY = (1 << 2),
    MESH_CAMERA_BLOCK = (1 << 3),
};

typedef struct SceneMesh {
    struct SceneMesh *next;
    primitive_t *mesh;
    Material *material;
    rspq_block_t *renderBlock;
    T3DMat4FP mtx;
    char *second;
    color_t primC;
    short materialID;
    unsigned char step;
    unsigned char flags;
} SceneMesh;

typedef struct SceneChunk {
    struct SceneChunk *next;
    struct CollisionCell *collision;
    float bounds[2][3];
    SceneMesh *meshList;
    short flags;
    short chunkID;
    short collisionTriCount;
    unsigned char *visibility;
    unsigned char collisionTimer;
} SceneChunk;

typedef struct ObjectMap {
    short objectID;
    char type;
    char yaw;
    short x;
    short y;
    short z;
} ObjectMap;

typedef struct SceneMap {
    char *texture;
    float scaleX;
    float scaleY;
    char offsetX;
    char offsetY;
} SceneMap;

typedef struct SceneHeader {
    char *model;
    struct ObjectMap *objectMap;
    unsigned short fogNear;
    unsigned short fogFar;
    unsigned char fogColour[3];
    unsigned char lightColour[3];
    unsigned char lightAmbient[3];
    unsigned char skyTop[3];
    unsigned char skyBottom[3];
    int flags;
    Texture skyTexture;
    unsigned short chunkCount;
    SceneMap *map;
} SceneHeader;

typedef struct SceneBlock {
    Model3D *model;
    SceneChunk *chunkList;
    int sceneID;
    void *overlay;
    float bounds[2][3];
} SceneBlock;

extern SceneBlock *gCurrentScene;
extern Environment *gEnvironment;
extern char gSceneUpdate;
extern char *sSceneTable[SCENE_TOTAL];

void load_scene(int sceneID, int updateRate, float updateRateF);
void scene_clear_chunk(SceneChunk *c);
void scene_generate_chunk(SceneMesh *s);
void scene_generate_collision(SceneChunk *c);