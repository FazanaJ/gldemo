#pragma once

#include "../include/global.h"
#include "assets.h"

#define MAP_OBJ 0
#define MAP_CLU 1

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
    unsigned int primC;
    unsigned char step;
    unsigned char flags;
} SceneMesh;

typedef struct SceneChunk {
    struct SceneChunk *next;
    SceneMesh *meshList;
    short flags;
    short chunkID;
    float bounds[2][3];
    unsigned char *visibility;
} SceneChunk;

typedef struct ObjectMap {
    short objectID;
    char type;
    char yaw;
    short x;
    short y;
    short z;
} ObjectMap;

typedef struct SceneHeader {
    char *model;
    struct ObjectMap *objectMap;
    short fogNear;
    short fogFar;
    unsigned char fogColour[3];
    unsigned char lightColour[3];
    unsigned char lightAmbient[3];
    unsigned char skyTop[3];
    unsigned char skyBottom[3];
    int flags;
    Texture skyTexture;
    unsigned short chunkCount;
} SceneHeader;

typedef struct SceneBlock {
    model64_t *model;
    SceneChunk *chunkList;
    int sceneID;
    void *overlay;
} SceneBlock;

extern SceneBlock *sCurrentScene;
extern Environment *gEnvironment;
extern char gSceneUpdate;
extern char *sSceneTable[SCENE_TOTAL];

void load_scene(int sceneID);
void scene_clear_chunk(SceneChunk *c);
void scene_generate_chunk(SceneMesh *s);