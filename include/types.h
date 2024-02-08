#pragma once

#include <GL/gl.h>

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
    GLuint texture;
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
    GLfloat fogColour[4];
    GLfloat skyColourTop[4];
    GLfloat skyColourBottom[4];
    GLfloat fogNear;
    GLfloat fogFar;
    int flags;
    sprite_t *skySprite;
    Texture skyboxTextureID;
    unsigned char texGen;
    char skyTimer;
    GLuint textureSegments[32];
} Environment;