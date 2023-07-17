#pragma once

#include "assets.h"

typedef struct SceneIDs {
    char *model;
    struct ObjectMap *objectMap;
    char *header;
} SceneIDs;

typedef struct SceneMesh {
    struct SceneMesh *next;
    mesh_t *mesh;
    Material *material;
    rspq_block_t *renderBlock;
    int flags;
} SceneMesh;

#define MAP_OBJ 0
#define MAP_CLU 1

typedef struct ObjectMap {
    short objectID;
    char type;
    char yaw;
    short x;
    short y;
    short z;
} ObjectMap;

typedef struct SceneBlock {
    model64_t *model;
    SceneMesh *meshList;
    int sceneID;
} SceneBlock;

extern SceneBlock *sCurrentScene;

void load_scene(int sceneID);