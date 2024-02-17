#pragma once

#include <libdragon.h>
#include <GL/gl.h>
#include "../include/global.h"
#include "scene.h"
#include "object.h"

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
    TEX_CLAMP_H =   (1 << 0),
    TEX_CLAMP_V =   (1 << 1),
    TEX_MIRROR_H =  (1 << 2),
    TEX_MIRROR_V =  (1 << 3),
};

enum FileFormat {
    DFS_SPRITE,
    DFS_WAV64,
    DFS_MODEL64,
    DFS_XM64,
    DFS_FONT64,
    DFS_OVERLAY,

    DFS_TOTAL
};

enum FontList {
    FONT_INTERNAL,
    FONT_ARIAL,
    FONT_MVBOLI,

    FONT_TOTAL
};

 typedef struct TextureInfo {
    char *file;
    unsigned char flags;
} __attribute__((__packed__)) TextureInfo;

 typedef struct SpriteInfo {
    char *file;
    unsigned char frames;
} __attribute__((__packed__)) SpriteInfo;

extern short gNumTextures;
extern short gNumTextureLoads;
extern const TextureInfo gTextureIDs[];
extern rdpq_font_t *gFonts[FONT_TOTAL];

void setup_textures(GLuint textures[], sprite_t *sprites[], const char *texture_path[], int texture_number);
void asset_cycle(int updateRate);
void init_materials(void);
char *asset_dir(char *dir, int format);
void load_font(int fontID);
void free_font(int fontID);
int load_texture(Material *material);
void shadow_generate(struct Object *obj);
void sky_texture_generate(Environment *e);
rspq_block_t *sky_gradient_generate(Environment *e);
struct Object *allocate_object(void);
struct Clutter *allocate_clutter(void);
struct Particle *allocate_particle(void);
void set_object_functions(struct Object *obj, int objectID);
void free_object(struct Object *obj);
void free_clutter(struct Clutter *clu);
void free_particle(struct Particle *particle);
struct Object *spawn_object_pos(int objectID, float x, float y, float z);
struct Object *spawn_object_pos_angle(int objectID, float x, float y, float z, short pitch, short roll, short yaw);
struct Object *spawn_object_pos_angle_scale(int objectID, float x, float y, float z, short pitch, short roll, short yaw, float scaleX, float scaleY, float scaleZ);
struct Object *spawn_object_pos_scale(int objectID, float x, float y, float z, float scaleX, float scaleY, float scaleZ);
struct Clutter *spawn_clutter(int objectID, float x, float y, float z, short pitch, short roll, short yaw);
struct Particle *spawn_particle(int particleID, float x, float y, float z);
void object_model_generate(struct Object *obj);
