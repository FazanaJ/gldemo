#include <libdragon.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>
#include <malloc.h>

#include "assets.h"
#include "../include/global.h"
#define INCLUDE_TEX_TABLE
#include "../include/texture_table.h"
//#include "../include/talksprite_table.h"
#undef INCLUDE_TEX_TABLE

#include "debug.h"
#include "scene.h"
#include "main.h"

char *gFontAssetTable[] = {
    "arial.10",
    "mvboli.12"
};

RenderSettings sRenderSettings;
int sPrevRenderFlags;
int sPrevTextureID;
int sPrevCombiner;
MaterialList *gMaterialListHead;
MaterialList *gMaterialListTail;
static rspq_block_t *sParticleMaterialBlock;
rdpq_font_t *gFonts[FONT_TOTAL];
#ifdef PUPPYPRINT_DEBUG
short gNumTextures;
short gNumTextureLoads;
#endif

void init_materials(void) {
    bzero(&sRenderSettings, sizeof(RenderSettings));
    sPrevRenderFlags = 0;
    sPrevTextureID = 0;
    gMaterialListHead = NULL;
    gMaterialListTail = NULL;
#ifdef PUPPYPRINT_DEBUG
    gNumTextures = 0;
    gNumTextureLoads = 0;
#endif
    rspq_block_begin();
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_COLOR_MATERIAL);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    sParticleMaterialBlock = rspq_block_end();
}

void bind_new_texture(MaterialList *material) {
    int repeatH;
    int repeatV;
    int mirrorH;
    int mirrorV;
    int texID = material->textureID;

    if (sTextureIDs[texID].flags & TEX_CLAMP_H) {
        repeatH = false;
    } else {
        repeatH = REPEAT_INFINITE;
    }
    if (sTextureIDs[texID].flags & TEX_CLAMP_V) {
        repeatV = false;
    } else {
        repeatV = REPEAT_INFINITE;
    }
    if (sTextureIDs[texID].flags & TEX_MIRROR_H) {
        mirrorH = MIRROR_REPEAT;
    } else {
        mirrorH = MIRROR_DISABLED;
    }
    if (sTextureIDs[texID].flags & TEX_MIRROR_V) {
        mirrorV = MIRROR_REPEAT;
    } else {
        mirrorV = MIRROR_DISABLED;
    }
    glBindTexture(GL_TEXTURE_2D, material->texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glSpriteTextureN64(GL_TEXTURE_2D, material->sprite, &(rdpq_texparms_t){.s.repeats = repeatH, .t.repeats = repeatV, .s.mirror = mirrorH, .t.mirror = mirrorV});
}

static char *sFileFormatString[] = {
    "sprite",
    "wav64",
    "model64",
    "xm64",
    "font64",
    "dso"
};

static char sFileFormatName[40];

char *asset_dir(char *dir, int format) {
    sprintf(sFileFormatName, "rom:/%s.%s", dir, sFileFormatString[format]);
    return sFileFormatName;
}

void load_font(int fontID) {
    if (gFonts[fontID] == NULL) {
        gFonts[fontID] = rdpq_font_load(asset_dir(gFontAssetTable[fontID - 1], DFS_FONT64));
        rdpq_font_style(gFonts[fontID], 0, &(rdpq_fontstyle_t) { .color = RGBA32(255, 255, 255, 255),});
        rdpq_text_register_font(fontID, gFonts[fontID]);
    }
}

void free_font(int fontID) {
    if (gFonts[fontID] != NULL) {
        rdpq_font_free(gFonts[fontID]);
    }
}

static int load_texture(Material *material) {
    MaterialList *list = gMaterialListHead;
    // Check if the texture is already loaded, and bind it to the index.
    while (list) {
        if (list->textureID == material->textureID) {
            material->index = list;
            return 0;
        }
        list = list->next;
    }
    // It doesn't, so load the texture into RAM and create a new entry.
    list = malloc(sizeof(MaterialList));
    if (list == NULL) {
        return -1;
    }
    if (gMaterialListHead == NULL) {
        gMaterialListHead = list;
        gMaterialListHead->prev = NULL;
        gMaterialListTail = gMaterialListHead;
    } else {
        gMaterialListTail->next = list;
        gMaterialListTail->next->prev = gMaterialListTail;
        gMaterialListTail = gMaterialListTail->next;
    }
    list->next = NULL;
    list->sprite = sprite_load(asset_dir(sTextureIDs[material->textureID].file, DFS_SPRITE));
    list->textureID = material->textureID;
    glGenTextures(1, &list->texture);
    bind_new_texture(list);
    list->loadTimer = 10;
    material->index = list;
#ifdef PUPPYPRINT_DEBUG
    gNumTextures++;
#endif
    return 0;
}

static void free_material(MaterialList *material) {
    if (material == gMaterialListHead) {
        if (gMaterialListHead->next) {
            gMaterialListHead = gMaterialListHead->next;
            gMaterialListHead->prev = NULL;
        } else {
            gMaterialListHead = NULL;
            if (material == gMaterialListTail) {
                gMaterialListTail = NULL;
            }
        }
    } else {
        if (material == gMaterialListTail) {
            gMaterialListTail = gMaterialListTail->prev;
        }
        material->prev->next = material->next;
        if (material->next) {
            material->next->prev = material->prev;
        }
    }
    sprite_free(material->sprite);
    glDeleteTextures(1, &material->texture);
    free(material);
#ifdef PUPPYPRINT_DEBUG
    gNumTextures--;
#endif
}

void asset_cycle(int updateRate) {
    DEBUG_SNAPSHOT_1();
    if (gMaterialListHead == NULL) {
        get_time_snapshot(PP_MATERIALS, DEBUG_SNAPSHOT_1_END);
        return;
    }
    MaterialList *matList = gMaterialListHead;
    while (matList) {
        MaterialList *curList = matList;
        matList->loadTimer -= updateRate;
        matList = matList->next;
        if (curList->loadTimer <= 0) {
            free_material(curList);
            break;
        }
    }
#ifdef PUPPYPRINT_DEBUG
    gNumTextureLoads = 0;
#endif
    sPrevTextureID = 0;
    sPrevRenderFlags = 0;
    bzero(&sRenderSettings, sizeof(RenderSettings));
    if (gEnvironment->texGen) {
        gEnvironment->skyTimer -= updateRate;
        if (gEnvironment->skyTimer <= 0) {
            for (int i = 0; i < 32; i++) {
                glDeleteTextures(1, &gEnvironment->textureSegments[i]);
            }
            sprite_free(gEnvironment->skySprite);
            gEnvironment->texGen = false;
        }
    }
    get_time_snapshot(PP_MATERIALS, DEBUG_SNAPSHOT_1_END);
}

void set_particle_render_settings(void) {
    rspq_block_run(sParticleMaterialBlock);
    sRenderSettings.cutout = false;
    sRenderSettings.xlu = true;
    sRenderSettings.depthRead = true;
    sRenderSettings.vertexColour = false;
    sRenderSettings.inter = false;
    sRenderSettings.decal = false;
    sRenderSettings.backface = false;
    sRenderSettings.texture = true;
    if (gEnvironment->flags & ENV_FOG) {
        if (!sRenderSettings.fog) {
            glEnable(GL_FOG);
            sRenderSettings.fog = true;
        }
    } else {
        if (sRenderSettings.fog) {
            glDisable(GL_FOG);
            sRenderSettings.fog = false;
        }
    }
}

static void material_mode(int flags) {
    if (flags & MATERIAL_CUTOUT) {
        if (!sRenderSettings.cutout) {
            glEnable(GL_ALPHA_TEST);
            sRenderSettings.cutout = true;
        }
    } else {
        if (sRenderSettings.cutout) {
            glDisable(GL_ALPHA_TEST);
            sRenderSettings.cutout = false;
        }
    }
    if (flags & MATERIAL_XLU) {
        if (!sRenderSettings.xlu) {
            glEnable(GL_BLEND);
            glDepthMask(GL_FALSE);
            sRenderSettings.xlu = true;
        }
    } else {
        if (sRenderSettings.xlu) {
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
            sRenderSettings.xlu = false;
        }
    }
    if (flags & MATERIAL_DEPTH_READ) {
        if (!sRenderSettings.depthRead) {
            glEnable(GL_DEPTH_TEST);
            sRenderSettings.depthRead = true;
        }
    } else {
        if (sRenderSettings.depthRead) {
            glDisable(GL_DEPTH_TEST);
            sRenderSettings.depthRead = false;
        }
    }
    if (flags & MATERIAL_LIGHTING) {
        if (!sRenderSettings.lighting) {
            glEnable(GL_LIGHTING);
            sRenderSettings.lighting = true;
        }
    } else {
        if (sRenderSettings.lighting) {
            glDisable(GL_LIGHTING);
            sRenderSettings.lighting = false;
        }
    }
    if (flags & MATERIAL_FOG && gEnvironment->flags & ENV_FOG) {
        if (!sRenderSettings.fog) {
            glEnable(GL_FOG);
            sRenderSettings.fog = true;
        }
    } else {
        if (sRenderSettings.fog) {
            glDisable(GL_FOG);
            sRenderSettings.fog = false;
        }
    }
    if (flags & MATERIAL_VTXCOL) {
        if (!sRenderSettings.vertexColour) {
            glEnable(GL_COLOR_MATERIAL);
            sRenderSettings.vertexColour = true;
        }
    } else {
        if (sRenderSettings.vertexColour) {
            glDisable(GL_COLOR_MATERIAL);
            sRenderSettings.vertexColour = false;
        }
    }
    if (flags & MATERIAL_BACKFACE) {
        if (!sRenderSettings.backface) {
            glDisable(GL_CULL_FACE);
            sRenderSettings.backface = true;
        }
    } else {
        if (sRenderSettings.backface) {
            glEnable(GL_CULL_FACE);
            sRenderSettings.backface = false;
        }
    }
    if (flags & MATERIAL_DECAL) {
        if (!sRenderSettings.decal) {
            glDepthFunc(GL_EQUAL);
            sRenderSettings.decal = true;
            sRenderSettings.inter = false;
        }
    } else if (flags & MATERIAL_INTER) {
        if (!sRenderSettings.inter) {
            glDepthFunc(GL_LESS_INTERPENETRATING_N64);
            sRenderSettings.decal = false;
            sRenderSettings.inter = true;
        }
    } else {
        if (sRenderSettings.inter || sRenderSettings.decal) {
            glDepthFunc(GL_LESS);
            sRenderSettings.inter = false;
            sRenderSettings.decal = false;
        }
    }
    sPrevRenderFlags = flags;
}

static void material_texture(Material *m) {
    if (m->textureID != -1) {
        if (load_texture(m) == -1) {
            return;
        }
            
        if (!sRenderSettings.texture) {
            glEnable(GL_TEXTURE_2D);
            sRenderSettings.texture = true;
        }
        m->index->loadTimer = 10;
        glBindTexture(GL_TEXTURE_2D, m->index->texture);
        sPrevTextureID = m->textureID;
#ifdef PUPPYPRINT_DEBUG
        gNumTextureLoads++;
#endif
    } else {
        if (sRenderSettings.texture) {
            glDisable(GL_TEXTURE_2D);
            sRenderSettings.texture = false;
        }
    }
}

static void material_combiner(int combiner) {
    rdpq_set_combiner_raw(combiner);
    sPrevCombiner = combiner;
}

void material_set(Material *material, int flags, int combiner) {
    DEBUG_SNAPSHOT_1();
    flags |= material->flags;
    combiner |= material->combiner;
    if (sPrevTextureID != material->textureID) {
        material_texture(material);
    }
    if (sPrevRenderFlags != flags) {
        material_mode(flags);
    }
    if (sPrevCombiner != combiner) {
        material_combiner(combiner);
    }
    get_time_snapshot(PP_MATERIALS, DEBUG_SNAPSHOT_1_END);
}
