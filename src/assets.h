#pragma once

#include <libdragon.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>

enum MaterialFlags {
    MATERIAL_NULL,
    MATERIAL_CUTOUT =       (1 << 0),
    MATERIAL_XLU =          (1 << 1),
    MATERIAL_LIGHTING =     (1 << 2),
    MATERIAL_FOG =          (1 << 3),
    MATERIAL_ENVMAP =       (1 << 4),
    MATERIAL_DEPTH_WRITE =  (1 << 5),
};

typedef struct RenderSettings {
    unsigned cutout : 1;
    unsigned xlu : 1;
    unsigned lighting : 1;
    unsigned fog : 1;
    unsigned envmap : 1;
    unsigned depthWrite : 1;
    unsigned texture : 1;
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
} Material;

extern short gNumTextures;
extern short gNumTextureLoads;

void setup_textures(GLuint textures[], sprite_t *sprites[], const char *texture_path[], int texture_number);
void set_material(Material *material);
void cycle_textures(int updateRate);