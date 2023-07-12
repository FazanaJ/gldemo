#pragma once

#include <libdragon.h>
#include <GL/gl.h>

enum MaterialFlags {
    MATERIAL_NULL,
    MATERIAL_CUTOUT =       (1 << 0), // Enables 1 bit alpha.
    MATERIAL_XLU =          (1 << 1), // Enables semitransparency.
    MATERIAL_LIGHTING =     (1 << 2), // Enables light calculation.
    MATERIAL_FOG =          (1 << 3), // Enables fog.
    MATERIAL_ENVMAP =       (1 << 4), // Enables environment mapping.
    MATERIAL_DEPTH_READ =  (1 << 5), // Enables depth write.
    MATERIAL_VTXCOL =       (1 << 6), // Enables vertex colours
    MATERIAL_DECAL =        (1 << 7), // Enables surface decal projection.
    MATERIAL_INTER =        (1 << 8), // Enables interpenetrating surfaces.
    MATERIAL_BACKFACE =     (1 << 9), // Enables backfaces
    MATERIAL_INVISIBLE =    (1 << 10), // Don't render.
    MATERIAL_INTANGIBLE =   (1 << 11), // Don't collide with anything.
    MATERIAL_CAM_ONLY =     (1 << 12), // Only collide with the camera.
    MATERIAL_NO_CAM =       (1 << 13), // Don't collide with the camera.
};

enum TextureFlags {
    TEX_NULL,
    TEX_MIRROR_H =  (1 << 0),
    TEX_MIRROR_V =  (1 << 1),
    TEX_CLAMP_H =   (1 << 2),
    TEX_CLAMP_V =   (1 << 3),
};

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
} Material;

typedef struct TextureInfo {
    char *file;
    unsigned char flags;
    char pallettes;
} TextureInfo;

extern short gNumTextures;
extern short gNumTextureLoads;

void setup_textures(GLuint textures[], sprite_t *sprites[], const char *texture_path[], int texture_number);
void set_material(Material *material, int flags);
void cycle_textures(int updateRate);
void init_materials(void);
void set_particle_render_settings(void);