#pragma once

#include <GL/gl.h>
//#include <t3d/t3d.h>

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
} RenderSettings;

typedef struct MaterialList {
    struct MaterialList *next;
    struct MaterialList *prev;
    sprite_t *sprite;
#ifdef OPENGL
    GLuint texture;
#endif
    short loadTimer;
    short textureID;
} MaterialList;

typedef struct Material {
    MaterialList *index;
    short textureID;
    short flags;
    int combiner;
} Material;

typedef struct Environment {
#ifdef OPENGL
    GLfloat fogColour[3];
    GLfloat skyColourTop[3];
    GLfloat skyColourBottom[3];
    GLfloat fogNear;
    GLfloat fogFar;
#elif defined(TINY3D)
    unsigned char fogColour[3];
    unsigned char skyColourTop[3];
    unsigned char skyColourBottom[3];
    unsigned short fogNear;
    unsigned short fogFar;
#endif
    int flags;
    sprite_t *skySprite;
    Texture skyboxTextureID;
    unsigned char texGen;
    char skyTimer;
#ifdef OPENGL
    GLuint textureSegments[32];
#elif defined(TINY3D)
    T3DVertPacked *skyVerts;
#endif
} Environment;

#ifdef OPENGL
typedef model64_t Model3D;
#elif defined(TINY3D)
typedef T3DModel Model3D;
#endif