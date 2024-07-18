#pragma once

#include <GL/gl.h>
#include <t3d/t3dmodel.h>

typedef struct RenderSettings {
    unsigned cutout : 1;
    unsigned xlu : 1;
    unsigned lighting : 1;
    unsigned fog : 1;
    unsigned envmap : 1;
    unsigned depthRead : 1;
    unsigned texture : 1;
    unsigned vertexColour : 1;
    unsigned decal : 1;
    unsigned inter : 1;
    unsigned backface : 1;
    unsigned frontface : 1;
    unsigned ci : 1;
} RenderSettings;

typedef struct MaterialList {
    struct MaterialList *next;
    struct MaterialList *prev;
    struct Material *material;
    short materialID;
    char refCount;
} MaterialList;

typedef struct SpriteList {
    struct SpriteList *next;
    struct SpriteList *prev;
    sprite_t *sprite;
    short spriteID;
    char refCount;
} SpriteList;

typedef struct Material {
    SpriteList *tex0;
    SpriteList *tex1;
    struct MaterialList *entry;
    rspq_block_t *block;
    unsigned short flags;
    short combiner;
    short collisionFlags;
    short shiftS0;
    short shiftT0;
    short shiftS1;
    short shiftT1;
    unsigned short flipbookFrame0;
    unsigned short flipbookFrame1;
    char tex0Flags;
    char tex1Flags;
    char floorDecID;
} Material;

typedef struct Environment {
    unsigned char fogColour[3];
    unsigned char skyColourTop[3];
    unsigned char skyColourBottom[3];
    GLfloat fogNear;
    GLfloat fogFar;
    int flags;
    sprite_t *skySprite;
    Texture skyboxTextureID;
    unsigned char texGen;
    char skyTimer;
    rspq_block_t *skyInit;
    rspq_block_t *skySegment[32];
    T3DVertPacked *skyVerts;
    T3DMat4FP *skyMtx;
} Environment;

typedef T3DModel Model3D;

typedef struct {
    GLfloat m[4][4];
} Matrix;