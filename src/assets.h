#pragma once

#include <libdragon.h>
#include <GL/gl.h>
#include "../include/global.h"
#include "scene.h"
#include "object.h"

#define ANIM_NONE 65535
#define MOD_CLUTTER 0
#define MOD_OBJECT 1

enum Languages {
    LANG_ENGLISH,
    LANG_PLACEHOLDER,

    LANG_TOTAL
};


enum CombinerNames {
    CC_TEX_SHADE,
    CC_MULTITEX_SHADE,
    CC_TEX_PRIM,
    CC_MULTITEX_PRIM,
    CC_TEX_SHADE_PRIM,
    CC_MULTITEX_SHADE_PRIM,
    CC_MULTITEX_WATER,
    CC_SHADE_PRIM,
    CC_DECAL_SHADE_PRIM,

    CC_TOTAL
};

enum MaterialFlags {
    MAT_NULL,
    MAT_CUTOUT =       (1 << 0), // Enables 1 bit alpha.
    MAT_XLU =          (1 << 1), // Enables semitransparency.
    MAT_LIGHTING =     (1 << 2), // Enables light calculation.
    MAT_FOG =          (1 << 3), // Enables fog.
    MAT_ENVMAP =       (1 << 4), // Enables environment mapping.
    MAT_DEPTH_READ =   (1 << 5), // Enables depth write.
    MAT_VTXCOL =       (1 << 6), // Enables vertex colours
    MAT_DECAL =        (1 << 7), // Enables surface decal projection.
    MAT_INTER =        (1 << 8), // Enables interpenetrating surfaces.
    MAT_BACKFACE =     (1 << 9), // Enables backfaces
    MAT_INVISIBLE =    (1 << 10), // Don't render.
    MAT_FRONTFACE =    (1 << 11), // Enables frontfaces
    MAT_CI =           (1 << 12), // Use colour index
};

enum TextureFlags {
    TEX_NULL,
    TEX_CLAMP_H =   (1 << 0),
    TEX_CLAMP_V =   (1 << 1),
    TEX_MIRROR_H =  (1 << 2),
    TEX_MIRROR_V =  (1 << 3),
};

enum FloorDecFlags {
    FLOORDEC_NONE,
    FLOORDEC_GRASS =    (1 << 0),
    FLOORDEC_PEBBLES =  (1 << 1),
    FLOORDEC_STICKS =   (1 << 2),
    FLOORDEC_LEAVES =   (1 << 3),
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

#define COLFLAG_NULL            0x0000
#define COLFLAG_SOUND_DIRT      0x0001
#define COLFLAG_SOUND_GRASS     0x0002
#define COLFLAG_SOUND_STONE     0x0003
#define COLFLAG_SOUND_GRAVEL    0x0004
#define COLFLAG_SOUND_TILE      0x0005
#define COLFLAG_SOUND_WOOD      0x0006
#define COLFLAG_SOUND_GLASS     0x0007
#define COLFLAG_SOUND_WATER     0x0008
#define COLFLAG_SOUND_MESH      0x0009
#define COLFLAG_SOUND_SAND      0x000A
#define COLFLAG_SOUND_SNOW      0x000B
#define COLFLAG_SOUND_METAL     0x000C
#define COLFLAG_SOUND_CARPET    0x000D
#define COLFLAG_GRIP(x)         ((x & 0xF) << 4)
#define COLFLAG_INTANGIBLE      0x0100
#define COLFLAG_CAM_ONLY        0x0200
#define COLFLAG_NO_CAM          0x0400
#define COLFLAG_SHADOW          0x0800

#define COL_GET_SOUND(x) ((x) & 0xF)
#define COL_GET_GRIP(x) (((x) & 0xF0) >> 4)
#define COL_GET_FLAG(x) (((x) & 0xF00) >> 8)

typedef struct TextureInfo {
    char *file;
    unsigned flags : 4;
    unsigned flipbook : 5;
    unsigned flipBookSpeed : 3;
} __attribute__((__packed__)) TextureInfo;

typedef struct SpriteInfo {
    char *file;
    unsigned char frames;
} __attribute__((__packed__)) SpriteInfo;

typedef struct MaterialInfo {
    short tex0;
    short tex1;
    unsigned short flags;
    char combiner;
    short collisionFlags;
    char floorDecID;
    char shiftS0;
    char shiftT0;
    char shiftS1;
    char shiftT1;
    char moveS0;
    char moveT0;
    char moveS1;
    char moveT1;
} __attribute__((__packed__)) MaterialInfo;

extern short gNumMaterials;
extern short gNumTextureLoads;
extern const TextureInfo gTextureIDs[];
extern const MaterialInfo gMaterialIDs[];
extern rdpq_font_t *gFonts[FONT_TOTAL];

void setup_textures(GLuint textures[], sprite_t *sprites[], const char *texture_path[], int texture_number);
void asset_cycle(int updateRate);
void init_materials(void);
char *asset_dir(char *dir, int format);
void load_font(int fontID);
void free_font(int fontID);
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
void obj_overlay_init(struct Object *obj, int objectID);
Material *material_init(int materialID);
void material_try_free(MaterialList *material);
rspq_block_t *material_generate_dl(Material *m);
void material_run(Material *m);
void material_run_partial(Material *m);
void material_run_flipbook(Material *m);
void clutter_model_generate(struct Clutter *obj);