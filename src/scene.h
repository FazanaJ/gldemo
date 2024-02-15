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

typedef struct SceneMesh {
    struct SceneMesh *next;
    primitive_t *mesh;
    Material *material;
    rspq_block_t *renderBlock;
    int flags;
} SceneMesh;

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
} SceneHeader;

typedef struct SceneBlock {
    model64_t *model;
    SceneMesh *meshList;
    int sceneID;
    void *overlay;
} SceneBlock;

extern SceneBlock *sCurrentScene;
extern Environment *gEnvironment;
extern char gSceneUpdate;
extern char *sSceneTable[SCENE_TOTAL];

void load_scene(int sceneID);