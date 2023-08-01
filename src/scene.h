#pragma once

#include "assets.h"

#define MAP_OBJ 0
#define MAP_CLU 1

enum EnvironmentFlags {
    ENV_FOG = (1 << 0),
};

typedef struct SceneIDs {
    char *model;
    struct ObjectMap *objectMap;
    char *header;
} SceneIDs;

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

typedef struct SceneBlock {
    model64_t *model;
    SceneMesh *meshList;
    int sceneID;
} SceneBlock;

typedef struct Environment {
    GLfloat fogColour[4];
    GLfloat skyColourTop[4];
    GLfloat skyColourBottom[4];
    GLfloat fogNear;
    GLfloat fogFar;
    char flags;
} Environment;

extern SceneBlock *sCurrentScene;
extern Environment *gEnvironment;

void load_scene(int sceneID);