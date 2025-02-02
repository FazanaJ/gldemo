#include <libdragon.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>
#include <malloc.h>
#include <t3d/t3d.h>
#include <t3d/rsp/rsp_tiny3d.h>

#include "assets.h"
#include "../include/global.h"
#define INCLUDE_TEX_TABLE
#include "../include/texture_table.h"
#include "../include/material_table.h"
#undef INCLUDE_TEX_TABLE

#include "debug.h"
#include "scene.h"
#include "main.h"
#include "math_util.h"

char *gFontAssetTable[] = {
    "arial.10",
    "mvboli.12"
};

char *sObjectOverlays[OBJ_TOTAL] = {
    NULL,
    "player",
    "projectile",
    "npc",
    "crate",
    "testsphere",
    "barrel",
    "lightsource"
};

char *gModelIDs[] = {
    "humanoid",
    "rock",
    "bottombillboard",
    "crate",
    "testsphere",
    "testcylinder",
    "bottombillboard_mirror",
    "metalpipe",
    "cratelid"
};

short gObjectModels[OBJ_TOTAL] = {
    0,
    1,
    3,
    1,
    4,
    5,
    6,
    2,
};

short gClutterModels[CLUTTER_TOTAL] = {
    0,
    7,
    3,
    2
};

short playerModelTextures[] = {
    MATERIAL_INTROSIGN2, MATERIAL_FLATPRIM, MATERIAL_EYE1, MATERIAL_TROUSERS, MATERIAL_FLATPRIM, 
    MATERIAL_SHIRT, MATERIAL_BODY1, MATERIAL_BODY1, MATERIAL_MOUTH1, MATERIAL_FLATPRIM, MATERIAL_FLATPRIM, MATERIAL_FLATPRIM, MATERIAL_FLATPRIM, MATERIAL_FLATPRIM
};

MaterialList *gMaterialListHead;
MaterialList *gMaterialListTail;
SpriteList *gSpriteListHead;
SpriteList *gSpriteListTail;
ModelList_NEW *gModelListHead;
ModelList_NEW *gModelListTail;
ModelList *gModelIDListHead;
ModelList *gModelIDListTail;
rdpq_font_t *gFonts[FONT_TOTAL];
#ifdef PUPPYPRINT_DEBUG
short gNumMaterials;
short gNumTextureLoads;
#endif

void init_materials(void) {
    bzero(&gRenderSettings, sizeof(RenderSettings));
    gPrevRenderFlags = 0;
    gPrevMaterialID = -1;
    gMaterialListHead = NULL;
    gMaterialListTail = NULL;
#ifdef PUPPYPRINT_DEBUG
    gNumMaterials = 0;
    gNumTextureLoads = 0;
#endif
    rspq_block_begin();
#if OPENGL
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_COLOR_MATERIAL);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
#endif
    gParticleMaterialBlock = rspq_block_end();
}

static char *sFileFormatString[] = {
    "sprite",
    "wav64",
    "t3dm",
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
        rdpq_text_register_font(fontID, gFonts[fontID]);
    }
}

void free_font(int fontID) {
    if (gFonts[fontID] != NULL) {
        rdpq_font_free(gFonts[fontID]);
    }
}

rdpq_texparms_t sTexParams;

rdpq_combiner_t sCombinerTable[CC_TOTAL] = {
    RDPQ_COMBINER_TEX_SHADE,
    RDPQ_COMBINER2((TEX1, TEX0, TEX1, TEX0), (0, 0, 0, ENV), (COMBINED, 0, SHADE, 0), (0, 0, 0, COMBINED)),
    RDPQ_COMBINER_TEX_FLAT,
    RDPQ_COMBINER2((TEX1, TEX0, TEX1, TEX0), (0, 0, 0, ENV), (COMBINED, 0, PRIM, 0), (0, 0, 0, COMBINED)),
    RDPQ_COMBINER2((TEX0, 0, SHADE, 0), (0, 0, 0, ENV), (COMBINED, 0, PRIM, 0), (0, 0, 0, COMBINED)),
    RDPQ_COMBINER2((TEX1, TEX0, TEX1, TEX0), (0, 0, 0, ENV), (COMBINED, PRIM, SHADE, PRIM), (0, 0, 0, COMBINED)),
    RDPQ_COMBINER2((TEX0, PRIM, TEX1, PRIM), (TEX0, 0, TEX1, 0), (TEX1, 0, TEX1, COMBINED), (SHADE, 0, PRIM, COMBINED)),
    RDPQ_COMBINER2((PRIM, 0, SHADE, 0), (0, 0, 0, ENV), (PRIM, 0, SHADE, 0), (0, 0, 0, COMBINED)),
    RDPQ_COMBINER2((TEX0, PRIM, TEX0_ALPHA, PRIM), (0, 0, 0, ENV), (COMBINED, 0, SHADE, 0), (0, 0, 0, ENV)),
};

void material_setup_constants(Material *m) {
    tex_format_t fmt = sprite_get_format(m->tex0->sprite);
    if (fmt == FMT_CI4 || fmt == FMT_CI8) {
        int palletteSize;
        uint16_t *pallette;
        if (fmt == FMT_CI8) {
            palletteSize = 256;
        } else {
            palletteSize = 16;
        }
        pallette = sprite_get_palette(m->tex0->sprite);
        if (pallette == NULL) {
            debugf("Texture [%s] is marked as CI, but is not actually a CI.\n", gTextureIDs[m->tex0->spriteID].file);
        }
        rdpq_tex_upload_tlut(pallette, 0, palletteSize);
    }
    if (gTextureIDs[m->tex0->spriteID].flags & TEX_CLAMP_H) {
        sTexParams.s.repeats = 1;
    } else {
        sTexParams.s.repeats = REPEAT_INFINITE;
    }
    /*if (gTextureIDs[m->tex0->spriteID].flags & TEX_CLAMP_V) {
        sTexParams.t.repeats = 1;
    } else {*/
        sTexParams.t.repeats = REPEAT_INFINITE;
    //}
    if (gTextureIDs[m->tex0->spriteID].flags & TEX_MIRROR_H) {
        sTexParams.s.mirror = MIRROR_REPEAT;
    } else {
        sTexParams.s.mirror = false;
    }
    if (gTextureIDs[m->tex0->spriteID].flags & TEX_MIRROR_V) {
        sTexParams.t.mirror = MIRROR_REPEAT;
    } else {
        sTexParams.t.mirror = false;
    }
    /*sTexParams.s.translate = 0;
    sTexParams.t.translate = 0;
    sTexParams.s.scale_log = gMaterialIDs[m->entry->materialID].shiftS0;
    sTexParams.t.scale_log = gMaterialIDs[m->entry->materialID].shiftT0;*/
}

void material_run_tile0(Material *m) {
    m->shiftS0 += gMaterialIDs[m->entry->materialID].moveS0;
    int capS = 1024 >> gMaterialIDs[m->entry->materialID].shiftS0;
    if (m->shiftS0 > capS) {
        m->shiftS0 -= capS;
    }
    m->shiftT0 += gMaterialIDs[m->entry->materialID].moveT0;
    int capT = 1024 >> gMaterialIDs[m->entry->materialID].shiftT0;
    if (m->shiftT0 > capT) {
        m->shiftT0 -= capT;
    }
    sTexParams.s.translate = (float) m->shiftS0 * 0.125f;
    sTexParams.t.translate = (float) m->shiftT0 * 0.125f;
    int size = m->tex0->sprite->width;
    int scaleS = 0;
    while (size > 32) {
        size >>= 1;
        scaleS--;
    }
    int flipbook = gTextureIDs[m->tex0->spriteID].flipbook + 1;
    size = m->tex0->sprite->height / flipbook;
    int scaleT = 0;
    while (size > 32) {
        size >>= 1;
        scaleT--;
    }
    sTexParams.s.scale_log = gMaterialIDs[m->entry->materialID].shiftS0 + scaleS;
    sTexParams.t.scale_log = gMaterialIDs[m->entry->materialID].shiftT0 + scaleT;
    if (m->tex1) {
        rdpq_tex_multi_begin();
    }
}

void material_run_tile1(Material *m) {
    m->shiftS1 += gMaterialIDs[m->entry->materialID].moveS1;
    int capS = 1024 >> gMaterialIDs[m->entry->materialID].shiftS1;
    if (m->shiftS1 > capS) {
        m->shiftS1 -= capS;
    }
    m->shiftT1 += gMaterialIDs[m->entry->materialID].moveT1;
    int capT = 1024 >> gMaterialIDs[m->entry->materialID].shiftT1;
    if (m->shiftT1 > capT) {
        m->shiftT1 -= capT;
    }
    sTexParams.s.translate = (float) m->shiftS1 * 0.125f;
    sTexParams.t.translate = (float) m->shiftT1 * 0.125f;
    int size = m->tex1->sprite->width;
    int scaleS = 0;
    while (size > 32) {
        size >>= 1;
        scaleS--;
    }
    int flipbook = gTextureIDs[m->tex0->spriteID].flipbook + 1;
    size = m->tex1->sprite->height / flipbook;
    int scaleT = 0;
    while (size > 32) {
        size >>= 1;
        scaleT--;
    }
    sTexParams.s.scale_log = gMaterialIDs[m->entry->materialID].shiftS1 + scaleS;
    sTexParams.t.scale_log = gMaterialIDs[m->entry->materialID].shiftT1 + scaleT;
}

void material_run_partial(Material *m) {
    material_run_tile0(m);
    if (gTextureIDs[m->tex0->spriteID].flipbook == 0) {
        surface_t surf = sprite_get_pixels(m->tex0->sprite);
        rdpq_tex_upload(0, &surf, &sTexParams);
    }
    if (m->entry->material->tex1) {
        if (gTextureIDs[m->tex1->spriteID].flipbook == 0) {
            material_run_tile1(m);
            surface_t surf = sprite_get_pixels(m->tex1->sprite);
            rdpq_tex_upload(1, &surf, &sTexParams);
            rdpq_tex_multi_end();
        }
    }
}

void material_run_flipbook(Material *m) {
    material_run_tile0(m);
    sTexParams.t.translate = 0;
    int height = m->tex0->sprite->height / gTextureIDs[m->tex0->spriteID].flipbook;
    int y = (height * (m->flipbookFrame0 >> 2 ));
    surface_t surf = sprite_get_pixels(m->tex0->sprite);
    rdpq_tex_upload_sub(TILE0, &surf, &sTexParams, 0, y, m->tex0->sprite->width, y + height);
    m->flipbookFrame0 += 1;
    if ((m->flipbookFrame0 >> 2) >= gTextureIDs[m->tex0->spriteID].flipbook) {
        m->flipbookFrame0 = 0;
    }
}

rspq_block_t *material_generate_dl(Material *m) {
    rspq_block_begin();
    if (m->tex0) {
        material_setup_constants(m);
        if (gMaterialIDs[m->entry->materialID].moveS0 == 0 && gMaterialIDs[m->entry->materialID].moveT0 == 0 &&
            gMaterialIDs[m->entry->materialID].moveS1 == 0 && gMaterialIDs[m->entry->materialID].moveT1 == 0) {
            material_run_partial(m);
        }
    }
    rdpq_mode_combiner(sCombinerTable[m->combiner]);
    return rspq_block_end();
}

static ModelList_NEW *model_try_load(int modelID) {
    ModelList_NEW *list = gModelListHead;
    // Check if the texture is already loaded, and bind it to the index.
    while (list) {
        if (list->modelID == modelID) {
            goto end;
        }
        list = list->next;
    }
    // It doesn't, so load the texture into RAM and create a new entry.
    list = malloc(sizeof(ModelList_NEW));
    if (list == NULL) {
        return NULL;
    }
    if (gModelListHead == NULL) {
        gModelListHead = list;
        gModelListHead->prev = NULL;
        gModelListTail = gModelListHead;
    } else {
        gModelListTail->next = list;
        gModelListTail->next->prev = gModelListTail;
        gModelListTail = gModelListTail->next;
    }
    list->next = NULL;
    list->modelID = modelID;
    list->model = t3d_model_load(asset_dir(gModelIDs[modelID], DFS_MODEL64));
    list->refCount = 1;
    int partCount = list->model->chunkCount;
    list->partCount = partCount;
    list->nodes = malloc(sizeof(ModelDraw) * partCount);
    T3DModelIter it = t3d_model_iter_create(list->model, T3D_CHUNK_TYPE_OBJECT);
    for (int i = 0; t3d_model_iter_next(&it); i++) {
        list->nodes[i].obj = it.object;
        list->nodes[i].func = NULL;
        list->nodes[i].materialID = -1;
        list->nodes[i].block = NULL;
        list->nodes[i].material = NULL;
        // temp starts here
        list->nodes[i].colour = RGBA32(255, 255, 255, 255);
        if (modelID == 1) {
            list->nodes[i].materialID = playerModelTextures[i];
            if (list->nodes[i].materialID == MATERIAL_FLATPRIM || list->nodes[i].materialID == MATERIAL_EYE1 || 
            list->nodes[i].materialID == MATERIAL_MOUTH1 || list->nodes[i].materialID == MATERIAL_EYEBROW1 || list->nodes[i].materialID == MATERIAL_BODY1) {
                list->nodes[i].colour = RGBA32(255, 126, 0, 255);
            }
            if (i == 1) {
                list->nodes[i].colour = RGBA32(127, 48, 0, 255);
            }
        } else if (modelID == 4 || modelID == 5 || modelID == 6) {
            list->nodes[i].materialID = MATERIAL_CRATE;
        } else if (modelID == 7) {
            list->nodes[i].materialID = MATERIAL_PLANT1;
        } else if (modelID == 3) {
            list->nodes[i].materialID = MATERIAL_FLIPBOOKTEST;
        } else {
            list->nodes[i].materialID = 0;
        }
        // temp ends here
    }
    int animCount = t3d_model_get_animation_count(list->model);
    if (animCount != 0) {
        list->animData = malloc(sizeof(void *) * animCount);
        t3d_model_get_animations(list->model, list->animData);
    } else {
        list->animData = NULL;
    }

    end:
    list->refCount++;
    return list;
}

tstatic void model_try_free(ModelList_NEW *list) {
    list->refCount--;
    if (list->refCount > 0) {
        return;
    }

    for (int i = 0; i < list->partCount; i++) {
        if (list->nodes[i].block) {
            rspq_block_free(list->nodes[i].block);
        }
        if (list->nodes[i].material) {
            material_try_free(list->nodes[i].material->entry);
        }
    }
        
    if (list == gModelListHead) {
        if (gModelListHead->next) {
            gModelListHead = gModelListHead->next;
            gModelListHead->prev = NULL;
        } else {
            gModelListHead = NULL;
            if (list == gModelListTail) {
                gModelListTail = NULL;
            }
        }
    } else {
        if (list == gModelListTail) {
            gModelListTail = gModelListTail->prev;
        }
        list->prev->next = list->next;
        if (list->next) {
            list->next->prev = list->prev;
        }
    }
    if (list->animData) {
        free(list->animData);
    }
    free(list);
}

static SpriteList *sprite_try_load(int spriteID) {
    SpriteList *list = gSpriteListHead;
    // Check if the texture is already loaded, and bind it to the index.
    while (list) {
        if (list->spriteID == spriteID) {
            goto end;
        }
        list = list->next;
    }
    // It doesn't, so load the texture into RAM and create a new entry.
    list = malloc(sizeof(SpriteList));
    if (list == NULL) {
        return NULL;
    }
    if (gSpriteListHead == NULL) {
        gSpriteListHead = list;
        gSpriteListHead->prev = NULL;
        gSpriteListTail = gSpriteListHead;
    } else {
        gSpriteListTail->next = list;
        gSpriteListTail->next->prev = gSpriteListTail;
        gSpriteListTail = gSpriteListTail->next;
    }
    list->next = NULL;
    list->spriteID = spriteID;
    list->sprite = sprite_load(asset_dir(gTextureIDs[spriteID].file, DFS_SPRITE));
    list->refCount = 0;
    end:
    list->refCount++;
    return list;
}

tstatic void sprite_try_free(SpriteList *sprite) {
    sprite->refCount--;
    if (sprite->refCount > 0) {
        return;
    }
    sprite_free(sprite->sprite);
        
    if (sprite == gSpriteListHead) {
        if (gSpriteListHead->next) {
            gSpriteListHead = gSpriteListHead->next;
            gSpriteListHead->prev = NULL;
        } else {
            gSpriteListHead = NULL;
            if (sprite == gSpriteListTail) {
                gSpriteListTail = NULL;
            }
        }
    } else {
        if (sprite == gSpriteListTail) {
            gSpriteListTail = gSpriteListTail->prev;
        }
        sprite->prev->next = sprite->next;
        if (sprite->next) {
            sprite->next->prev = sprite->prev;
        }
    }
    free(sprite);
}

Material *material_init(int materialID) {
    DEBUG_SNAPSHOT_1();
    MaterialList *list = gMaterialListHead;
    // Check if the texture is already loaded, and bind it to the index.
    while (list) {
        if (list->materialID == materialID) {
            list->refCount++;
            return list->material;
        }
        list = list->next;
    }
    debugf("Loading material: %d.", materialID);
    list = malloc(sizeof(MaterialList));
    if (gMaterialListHead == NULL) {
        gMaterialListHead = list;
        gMaterialListHead->prev = NULL;
        gMaterialListTail = gMaterialListHead;
    } else {
        gMaterialListTail->next = list;
        gMaterialListTail->next->prev = gMaterialListTail;
        gMaterialListTail = gMaterialListTail->next;
    }
    list->refCount = 1;
    list->materialID = materialID;
    list->next = NULL;
    list->material = malloc(sizeof(Material));
    list->material->collisionFlags = gMaterialIDs[materialID].collisionFlags;
    list->material->combiner = gMaterialIDs[materialID].combiner;
    list->material->flags = gMaterialIDs[materialID].flags;
    list->material->entry = list;
    list->material->shiftS0 = 0;
    list->material->shiftS1 = 0;
    list->material->flipbookFrame0 = 0;
    list->material->flipbookFrame1 = 0;
    if (gMaterialIDs[materialID].tex0 != TEXTURE_NONE) {
        list->material->tex0 = sprite_try_load(gMaterialIDs[materialID].tex0);
        list->material->tex0Flags = gTextureIDs[gMaterialIDs[materialID].tex0].flags;
        list->material->shiftT0 = list->material->tex0->sprite->height * 16;
    } else {
        list->material->tex0 = NULL;
        list->material->shiftT0 = 0;
    }
    if (gMaterialIDs[materialID].tex1 != TEXTURE_NONE) {
        list->material->tex1 = sprite_try_load(gMaterialIDs[materialID].tex1);
        list->material->tex1Flags = gTextureIDs[gMaterialIDs[materialID].tex1].flags;
        list->material->shiftT1 = list->material->tex1->sprite->height * 16;
    } else {
        list->material->tex1 = NULL;
        list->material->shiftT1 = 0;
    }
    if ((list->material->flags & MAT_INVISIBLE) == false) {
        list->material->block = material_generate_dl(list->material);
    } else {
        list->material->block = NULL;
    }
    debugf(" Time: %2.3fs.\n", (double) (TIMER_MICROS(DEBUG_SNAPSHOT_1_END) / 1000000.0f));
#ifdef PUPPYPRINT_DEBUG
    gNumMaterials++;
#endif
    return list->material;
}

void material_try_free(MaterialList *material) {
    material->refCount--;
    if (material->refCount > 0) {
        return;
    }
    
    debugf("Freeing material: %d.\n", material->materialID);
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
    rspq_block_free(material->material->block);
    if (material->material->tex0) {
        sprite_try_free(material->material->tex0);
    }
    if (material->material->tex1) {
        sprite_try_free(material->material->tex1);
    }
    free(material->material);
    free(material);
#ifdef PUPPYPRINT_DEBUG
    gNumMaterials--;
#endif
}

void shadow_generate(Object *obj) {
    DEBUG_SNAPSHOT_1();
    debugf("Allocating dynamic shadow for [%s] object.", sObjectOverlays[obj->objectID]);
    obj->gfx->dynamicShadow = malloc(sizeof(DynamicShadow));
    DynamicShadow *d = obj->gfx->dynamicShadow;
    d->mtx[0] = malloc_uncached(sizeof(T3DMat4FP));
    d->mtx[1] = malloc_uncached(sizeof(T3DMat4FP));
    d->texW = obj->header->dynamicShadow->texW;
    d->texH = obj->header->dynamicShadow->texH;
    d->planeW = obj->header->dynamicShadow->planeW;
    d->planeH = obj->header->dynamicShadow->planeH;
    d->offset = obj->header->dynamicShadow->offset;
    d->camPos[0] = obj->header->dynamicShadow->camPos[0];
    d->camPos[1] = obj->header->dynamicShadow->camPos[1];
    d->camPos[2] = obj->header->dynamicShadow->camPos[2];
    d->camFocus[0] = obj->header->dynamicShadow->camFocus[0];
    d->camFocus[1] = obj->header->dynamicShadow->camFocus[1];
    d->camFocus[2] = obj->header->dynamicShadow->camFocus[2];
    d->angle[0] = 0;
    d->angle[1] = 0;
    d->angle[2] = 0;
    d->staleTimer = 10;
    d->texCount = 0;
    d->surface = surface_alloc(FMT_I8, d->texW, d->texH);
    int w = 0;
    int h = 0;
    float frac = (float) d->texW / 64.0f;
    if (frac <= 1.0f) {
        w = 1;
    } else {
        float newFrac = frac - (int) frac;
        if (newFrac > 0.0f) {
            frac = (int) frac + 1;
        }
        w = frac;
    }
    frac = (float) d->texH / 64.0f;
    if (frac <= 1.0f) {
        h = 1;
    } else {
        float newFrac = frac - (int) frac;
        if (newFrac > 0.0f) {
            frac = (int) frac + 1;
        }
        h = frac;
    }
    d->acrossX = w;
    d->acrossY = h;
    d->texCount = w * h;
    int x = 0;
    int y = 0;
    int texLoads = d->texCount - 1;
    int stepW = MIN(MIN(d->texW, 64), d->texW / w);
    int stepH = MIN(MIN(d->texH, 64), d->texH / h);
    rspq_block_begin();
    d->verts = malloc_uncached(sizeof(T3DVertPacked) * (d->texCount * 2));
    rdpq_sync_pipe();
    rdpq_set_prim_color(RGBA32(0, 0, 0, 127));
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_combiner(RDPQ_COMBINER_TEX_FLAT);
    t3d_state_set_drawflags(T3D_FLAG_TEXTURED | T3D_FLAG_DEPTH | T3D_FLAG_CULL_FRONT | T3D_FLAG_SHADED);
    x = -32;
    int z = ((w - 1) * 64) + 16 + d->offset;
    int uvX = 0;
    int uvY = 0;
    for (int i = 0, j = 0; i < 6; i++, j += 2) {
        d->verts[i] = (T3DVertPacked){
            .posA = {x, 0, z}, .stA[0] = uvX, .stA[1] = uvY,
            .posB = {x, 0, z - 32}, .stB[0] = uvX, .stB[1] = uvY + 2048,
        };
        x += 32;
        if (x > 32) {
            x = -32;
            z -= 64;
            uvX = 0;
            uvY += 4096;
        } else {
            uvX += 2048;
        }
    }
    
    x = 0;
    z = 0;
    t3d_matrix_push(t3d_segment_placeholder(SEGMENT_MATRIX));
    t3d_vert_load(d->verts, 0, d->texCount * 2);
    t3d_matrix_pop(1);
    int i0 = 0;
    int i1 = 1;
    int i2 = 2;
    int pattern = 0;
    int texX = 0;
    int texY = 0;//d->texH;
    rdpq_texparms_t parms;
    bzero(&parms, sizeof(rdpq_texparms_t));
    parms.s.repeats = 0;
    parms.t.repeats = REPEAT_INFINITE;
    parms.t.mirror = true;
    for (int i = 0; i < 12; i++) {
        if ((i % 2) == 0) {
            t3d_tri_sync();
            rdpq_tex_upload_sub(TILE0, &d->surface, &parms, texX, texY, texX + stepW, texY + stepH);
            texX += stepW;
            if (texX >= d->texW) {
                texX = 0;
                texY += stepH;
            }
        }
        t3d_tri_draw(i0, i1, i2);
        x += 16;
        if (x >= 64) {
            x = 0;
            if (pattern < 2) {
                pattern = 2;
                i0 -= 3;
                i1 += 3;
            } else {
                pattern = 0;
                i0 += 1;
                i1 -= 1;
            }
            i2 -= 2;
        } else {
            switch (pattern) {
            case 0:
                i0 += 2;
                i2 += 1;
                break;
            case 1:
                i1 += 2;
                i2 += 1;
                break;
            case 2:
                i0 += 2;
                i2 += 5;
                break;
            case 3:
                i1 += 2;
                i2 -= 3;
                break;
            }
            if (pattern == 0) {
                pattern = 1;
            } else if (pattern == 1) {
                pattern = 0;
            } else if (pattern == 2) {
                pattern = 3;
            } else if (pattern == 3) {
                pattern = 2;
            }
        }
    }
    t3d_tri_sync();
    d->block = rspq_block_end();

    debugf(" Texture count: %d.", d->texCount);
    debugf(" Time: %2.3fs.\n", (double) (TIMER_MICROS(DEBUG_SNAPSHOT_1_END) / 1000000.0f));
}

tstatic void object_model_clear(ModelList *entry) {
    ObjectModel *m = entry->entry;
    while (m) {
        if (m->material) {
            material_try_free(m->material->entry);
            m->material = NULL;
        }
        m->material = NULL;
        if (m->block) {
            rspq_block_free(m->block);
            m->block = NULL;
        }
        m = m->next;
    }
    entry->active = false;
}

void sky_free(Environment *e) {
    debugf("Freeing texture: %s.\n", gTextureIDs[gEnvironment->skyboxTextureID].file);
    if (gEnvironment->skyInit) {
        rspq_block_free(gEnvironment->skyInit);
        gEnvironment->skyInit = NULL;
    }
    for (int i = 0; i < 32; i++) {
        if (gEnvironment->skySegment[i]) {
            rspq_block_free(gEnvironment->skySegment[i]);
            gEnvironment->skySegment[i] = NULL;
        }
        gNumMaterials--;
    }
    if (gEnvironment->skyVerts) {
        free_uncached(gEnvironment->skyVerts);
    }
    if (gEnvironment->skyMtx) {
        free_uncached(gEnvironment->skyMtx);
    }
    sprite_free(gEnvironment->skySprite);
    gEnvironment->texGen = false;
}

void asset_cycle(int updateRate) {
    DEBUG_SNAPSHOT_1();
    if (gMaterialListHead == NULL) {
        get_time_snapshot(PP_MATERIALS, DEBUG_SNAPSHOT_1_END);
        return;
    }
#ifdef PUPPYPRINT_DEBUG
    gNumTextureLoads = 0;
#endif
    gPrevMaterialID = -1;
    gPrevRenderFlags = 0;
    bzero(&gRenderSettings, sizeof(RenderSettings));
    if (gEnvironment->texGen) {
        gEnvironment->skyTimer -= updateRate;
        if (gEnvironment->skyTimer <= 0) {
            sky_free(gEnvironment);
        }
    }
    get_time_snapshot(PP_MATERIALS, DEBUG_SNAPSHOT_1_END);

    ModelList *modelList = gModelIDListHead;

    while (modelList) {
        modelList->timer -= updateRate;
        if (modelList->timer <= 0) {
            object_model_clear(modelList);
            //break;
        }
        modelList = modelList->next;
    }

    ObjectList *objList = gObjectListHead;
    Object *obj;

    while (objList) {
        obj = objList->obj;
        if (obj->overlay) {
            obj->overlayTimer -= updateRate;
            if (obj->overlayTimer <= 0) {
                dlclose(obj->overlay);
                obj->overlay = NULL;
            }
        }
        objList = objList->next;
    }

    SceneChunk *sceneList = gCurrentScene->chunkList;
    while (sceneList) {
        if (sceneList->collisionTimer > 0) {
            sceneList->collisionTimer -= updateRate;
            if (sceneList->collisionTimer <= 0) {
                free(sceneList->collision);
                free(sceneList->collisionData);
                sceneList->collision = NULL;
            }
        }
        sceneList = sceneList->next;
    }
}

void sky_texture_generate(Environment *e) {
    DEBUG_SNAPSHOT_1();
    debugf("Loading texture: %s.", gTextureIDs[e->skyboxTextureID].file);
    e->skySprite = sprite_load(asset_dir(gTextureIDs[e->skyboxTextureID].file, DFS_SPRITE));
    surface_t surf = sprite_get_pixels(e->skySprite);
    tex_format_t fmt = sprite_get_format(e->skySprite);
    e->skyVerts = malloc_uncached(sizeof(T3DVertPacked) * 64);
    e->skyMtx = malloc_uncached(sizeof(T3DMat4FP));
    rspq_block_begin();
    t3d_matrix_push(t3d_segment_placeholder(SEGMENT_MATRIX));
    rdpq_set_mode_standard();
    rdpq_mode_persp(true);
    rdpq_mode_antialias(AA_NONE);
    t3d_state_set_drawflags(T3D_FLAG_TEXTURED | T3D_FLAG_CULL_BACK);
    __rdpq_mode_change_som(0x300000000001, 0x200000000000);
    rdpq_mode_zbuf(false, false);
    rdpq_mode_blender(0);
    rdpq_mode_filter(FILTER_BILINEAR);
    rdpq_mode_dithering(DITHER_SQUARE_SQUARE);
    rdpq_mode_combiner(RDPQ_COMBINER_TEX);
    if (fmt == FMT_CI4 || fmt == FMT_CI8) {
        int colours;
        if (fmt == FMT_CI4) {
            colours = 16;
        } else {
            colours = 256;
        }
        rdpq_mode_tlut(TLUT_RGBA16);
        rdpq_tex_upload_tlut(sprite_get_palette(e->skySprite), 0, colours);
    }
    e->skyInit = rspq_block_end();
    rdpq_texparms_t parms = {
        .s.translate = 0,
        .t.translate = 0,
        .s.scale_log = 0,
        .t.scale_log = 0,
    };
    
    for (int i = 0, j = 0, x = 0; i < 16; i++, j += 2) {
        gNumMaterials++;
        rspq_block_begin();
        rdpq_set_prim_color(RGBA32(i * 127, i * 64, i * 32, 255));
        rdpq_tex_upload_sub(TILE0, &surf, &parms, x, 0, x + 32, 64);
        float pX = 500.0f * sins((0x10000 / 16) * i);
        float pZ = 500.0f * coss((0x10000 / 16) * i);
        e->skyVerts[j + 0] = (T3DVertPacked){
            .posA = {pX * 0.66f, 375, pZ * 0.66f}, .stA[0] = (i * 1024), .stA[1] = 0,
            .posB = {pX, 0, pZ}, .stB[0] = (i * 1024), .stB[1] = 2048,
        };
        pX = 500.0f * sins((0x10000 / 16) * (i + 1));
        pZ = 500.0f * coss((0x10000 / 16) * (i + 1));
        e->skyVerts[j + 1] = (T3DVertPacked){
            .posA = {pX, 0, pZ}, .stA[0] = 1024 + (i * 1024), .stA[1] = 2048,
            .posB = {pX * 0.66, 375, pZ * 0.66f}, .stB[0] = 1024 + (i * 1024), .stB[1] = 0,
        };
        
        data_cache_hit_writeback(&e->skyVerts[j], sizeof(T3DVertPacked) * 2);
        t3d_vert_load(&e->skyVerts[j], 0, 4);
        t3d_tri_draw(2, 1, 0);
        t3d_tri_draw(3, 2, 0);
        t3d_tri_sync();
        e->skySegment[i] = rspq_block_end();
        x += 32;
    }
    parms.t.repeats = REPEAT_INFINITE;
    for (int i = 0, j = 0, x = 0; i < 16; i++, j += 2) {
        gNumMaterials++;
        rspq_block_begin();
        rdpq_tex_upload_sub(TILE0, &surf, &parms, x, 64, x + 32, 128);
        float pX = 500.0f * sins((0x10000 / 16) * i);
        float pZ = 500.0f * coss((0x10000 / 16) * i);
        e->skyVerts[j + 32 + 0] = (T3DVertPacked){
            .posA = {pX, 0, pZ}, .stB[0] = (i * 1024), .stB[1] = 2048,
            .posB = {pX * 0.66f, -375, pZ * 0.66f}, .stA[0] = (i * 1024), .stA[1] = 0,
        };
        pX = 500.0f * sins((0x10000 / 16) * (i + 1));
        pZ = 500.0f * coss((0x10000 / 16) * (i + 1));
        e->skyVerts[j + 32 + 1] = (T3DVertPacked){
            .posA = {pX * 0.66, -375, pZ * 0.66f}, .stB[0] = 1024 + (i * 1024), .stB[1] = 0,
            .posB = {pX, 0, pZ}, .stA[0] = 1024 + (i * 1024), .stA[1] = 2048,
        };
        
        data_cache_hit_writeback(&e->skyVerts[j + 32], sizeof(T3DVertPacked) * 2);
        t3d_vert_load(&e->skyVerts[j + 32], 0, 4);
        t3d_tri_draw(2, 1, 0);
        t3d_tri_draw(3, 2, 0);
        t3d_tri_sync();
        e->skySegment[16 + i] = rspq_block_end();
        x += 32;
    }
    debugf(" Time: %2.3fs.\n", (double) (TIMER_MICROS(DEBUG_SNAPSHOT_1_END) / 1000000.0f));
}

rspq_block_t *sky_gradient_generate(Environment *e) {
    float width = display_get_width();
    float height = display_get_height();
    rspq_block_begin();
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_SHADE);
    rdpq_mode_blender(0);
    rdpq_mode_dithering(DITHER_SQUARE_SQUARE);
    float col[2][3];
    col[0][0] = e->skyColourTop[0] / 255.0f;
    col[0][1] = e->skyColourTop[1] / 255.0f;
    col[0][2] = e->skyColourTop[2] / 255.0f;
    col[1][0] = e->skyColourBottom[0] / 255.0f;
    col[1][1] = e->skyColourBottom[1] / 255.0f;
    col[1][2] = e->skyColourBottom[2] / 255.0f;
    rdpq_triangle(&TRIFMT_SHADE,
        (float[]){0.0f, 0.0f, col[0][0], col[0][1], col[0][2], 1.0f},
        (float[]){0.0f, height, col[1][0], col[1][1], col[1][2], 1.0f},
        (float[]){width, height, col[1][0], col[1][1], col[1][2], 1.0f}
    );
    rdpq_triangle(&TRIFMT_SHADE,
        (float[]){width, height, col[1][0], col[1][1], col[1][2], 1.0f},
        (float[]){width, 0.0f, col[0][0], col[0][1], col[0][2], 1.0f},
        (float[]){0.0f, 0.0f, col[0][0], col[0][1], col[0][2], 1.0f}
    );
    return rspq_block_end();
}

void obj_overlay_init(Object *obj, int objectID) {
    DEBUG_SNAPSHOT_1();
    debugf("Loading overlay [%s].", sObjectOverlays[objectID]);
    void *addr = dlopen(asset_dir(sObjectOverlays[objectID], DFS_OVERLAY), RTLD_LOCAL);
#ifdef PUPPYPRINT_DEBUG
    gNumOverlays++;
#endif
    debugf(" Time: %2.3fs.\n", (double) (TIMER_MICROS(DEBUG_SNAPSHOT_1_END) / 1000000.0f));
    ObjectEntry *entry = dlsym(addr, "entry");
    obj->header = entry;
    obj->loopFunc = entry->loopFunc;
    obj->flags = entry->flags;
    obj->overlay = addr;
    obj->hitbox = entry->hitbox;
    obj->overlayTimer = 10;
}

tstatic void init_object_behaviour(Object *obj, int objectID) {
    obj_overlay_init(obj, objectID);
    if (obj->header->viewDist) {
        obj->viewDist = obj->header->viewDist << 4;
    } else {
        obj->viewDist = 500.0f;
    }
    obj->viewDist *= obj->viewDist;
    if (obj->header->data) {
        obj->data = malloc(obj->header->data);
        bzero(obj->data, obj->header->data);
    }
    if (obj->flags & OBJ_FLAG_COLLISION) {
        obj->collision = malloc(sizeof(ObjectCollision));
        bzero(obj->collision, sizeof(ObjectCollision));
    }
    if (obj->flags & OBJ_FLAG_MOVE) {
        obj->movement = malloc(sizeof(ObjectMovement));
        bzero(obj->movement, sizeof(ObjectMovement));
    }
    if (obj->header->initFunc) {
        (*obj->header->initFunc)(obj);
    }
}

void check_unused_model(Object *obj) {
    ObjectList *objList = gObjectListHead;
    Object *listObj;

    while (objList) {
        listObj = objList->obj;
        if (listObj->gfx->modelID == obj->gfx->modelID && listObj != obj) {
            return;
        }
        objList = objList->next;
    }

    ClutterList *cluList = gClutterListHead;
    Clutter *listClu;

    while (cluList) {
        listClu = cluList->clutter;
        if (listClu->gfx->modelID == obj->gfx->modelID && listClu != (Clutter *) obj) {
            return;
        }
        cluList = cluList->next;
    }
    
    debugf("Freeing model [%s]\n", gModelIDs[obj->gfx->modelID - 1]);
    if (obj->gfx->listEntry == gModelIDListHead) {
        if (gModelIDListHead->next) {
            gModelIDListHead = gModelIDListHead->next;
            gModelIDListHead->prev = NULL;
        } else {
            gModelIDListHead = NULL;
        }
    } else {
        if (obj->gfx->listEntry == gModelIDListTail) {
            gModelIDListTail = gModelIDListTail->prev;
        }
        obj->gfx->listEntry->prev->next = obj->gfx->listEntry->next;
        if (obj->gfx->listEntry->next) {
            obj->gfx->listEntry->next->prev = obj->gfx->listEntry->prev;
        }
    }

    ObjectModel *curMesh = obj->gfx->listEntry->entry;
    while (curMesh) {
        ObjectModel *m = curMesh;
        curMesh = curMesh->next;
        if (m->material) {
            material_try_free(m->material->entry);
        }
        if (m->block) {
            rspq_block_free(m->block);
        }
        free(m);
    }
    t3d_model_free(obj->gfx->listEntry->model64);
    if (obj->gfx->listEntry->animData) {
        free(obj->gfx->listEntry->animData);
    }
    free(obj->gfx->listEntry);
#ifdef PUPPYPRINT_DEBUG
        gNumModels--;
#endif
}

void check_unused_overlay(Object *obj) {
    if (obj->overlay) {
        dlclose(obj->overlay);
    }
#ifdef PUPPYPRINT_DEBUG
    gNumOverlays--;
#endif
}

/**
 * Check if gObjectListHead is null first.
 * If it is, set that as the new list element, otherwise, make a new element and join it onto the tail.
*/
Object *allocate_object(void) {
    Object *newObj = malloc(sizeof(Object));
    bzero(newObj, sizeof(Object));
    newObj->entry = malloc(sizeof(ObjectList));

    if (gObjectListHead == NULL) {
        gObjectListHead = newObj->entry;
        gObjectListHead->next = NULL;
        gObjectListHead->prev = NULL;
        gObjectListTail = gObjectListHead;
        gObjectListHead->obj = newObj;
    } else {
        ObjectList *list = newObj->entry;
        gObjectListTail->next = list;
        list->prev = gObjectListTail;
        list->next = NULL;
        list->obj = newObj;
        gObjectListTail = list;
    }
    newObj->loopFunc = NULL;
    newObj->gfx = NULL;
    newObj->data = NULL;
    newObj->flags = OBJ_FLAG_NONE;
    newObj->viewDist = SQR(300.0f);
    newObj->overlay = NULL;
#ifdef PUPPYPRINT_DEBUG
    gNumObjects++;
#endif
    return newObj;
}

Clutter *allocate_clutter(void) {
    Clutter *newClutter = malloc(sizeof(Clutter));
    bzero(newClutter, sizeof(Clutter));
    newClutter->entry = malloc(sizeof(ClutterList));

    if (gClutterListHead == NULL) {
        gClutterListHead = newClutter->entry;
        gClutterListHead->next = NULL;
        gClutterListHead->prev = NULL;
        gClutterListTail = gClutterListHead;
        gClutterListHead->clutter = newClutter;
    } else {
        ClutterList *list = newClutter->entry;
        gClutterListTail->next = list;
        list->prev = gClutterListTail;
        list->next = NULL;
        list->clutter = newClutter;
        gClutterListTail = list;
    }
    newClutter->gfx = NULL;
    newClutter->flags = OBJ_FLAG_NONE;
    newClutter->viewDist = SQR(1500.0f);
    newClutter->matrix = malloc_uncached(sizeof(T3DMat4FP));
#ifdef PUPPYPRINT_DEBUG
    gNumClutter++;
#endif
    return newClutter;
}

Particle *allocate_particle(void) {
    Particle *newParticle = malloc(sizeof(Particle));
    bzero(newParticle, sizeof(Particle));
    newParticle->entry = malloc(sizeof(ParticleList));

    if (gParticleListHead == NULL) {
        gParticleListHead = newParticle->entry;
        gParticleListHead->next = NULL;
        gParticleListHead->prev = NULL;
        gParticleListTail = gParticleListHead;
        gParticleListHead->particle = newParticle;
    } else {
        ParticleList *list = newParticle->entry;
        gParticleListTail->next = list;
        list->prev = gParticleListTail;
        list->next = NULL;
        list->particle = newParticle;
        gParticleListTail = list;
    }
    //newParticle->material = NULL;
    newParticle->flags = OBJ_FLAG_NONE;
#ifdef PUPPYPRINT_DEBUG
    gNumParticles++;
#endif
    return newParticle;
}

static int temp_matrix_grabber(int modelID) {
    switch (modelID) {
    case 1:
        return MTX_TRANSLATE_ROTATE_SCALE;
    case 2:
        return MTX_TRANSLATE_ROTATE_SCALE;
    case 3:
        return MTX_BILLBOARD;
    case 7:
        return MTX_BILLBOARD;
    }
    return MTX_TRANSLATE_ROTATE_SCALE;
}

void clutter_model_generate(Clutter *obj) {
    ObjectModel *m = obj->gfx->listEntry->entry;
    m->material = material_init(m->materialID);
    rspq_block_begin();
    t3d_matrix_push(t3d_segment_placeholder(SEGMENT_MATRIX));
    t3d_model_draw((T3DModel *) obj->gfx->listEntry->model64);
    t3d_matrix_pop(1);
    m->block = rspq_block_end();
    obj->gfx->listEntry->active = true;
}

void object_model_generate(Object *obj) {
    ObjectModel *m = obj->gfx->listEntry->entry;
    while (m) {
        T3DMat4FP *mat;
        m->material = material_init(m->materialID);
        rspq_block_begin();
        if (obj->animation) {
            mat = t3d_segment_placeholder(SEGMENT_BONES);
        } else {
            mat = NULL;
        }
        t3d_matrix_push(t3d_segment_placeholder(SEGMENT_MATRIX));
        t3d_model_draw_object((T3DObject *) m->prim, mat);
        t3d_matrix_pop(1);
        m->block = rspq_block_end();
        m = m->next;
    }
    obj->gfx->listEntry->active = true;
}

tstatic void load_object_model(Object *obj, int objectID, int tableID) {
    DEBUG_SNAPSHOT_1();
    obj->gfx = malloc(sizeof(ObjectGraphics));
    bzero(obj->gfx, sizeof(ObjectGraphics));
    obj->gfx->envColour[0] = 0xFF;
    obj->gfx->envColour[1] = 0xFF;
    obj->gfx->envColour[2] = 0xFF;
    obj->gfx->primColour[0] = 0xFF;
    obj->gfx->primColour[1] = 0xFF;
    obj->gfx->primColour[2] = 0xFF;
    obj->gfx->opacity = 0xFF;
    int modelID;
    if (tableID == MOD_CLUTTER) {
        modelID = gClutterModels[objectID];
    } else {
        modelID = gObjectModels[objectID];
    }
    obj->gfx->modelID = modelID;
    int matrixType = temp_matrix_grabber(modelID);

    if (gModelIDListHead) {
        ModelList *modelList = gModelIDListHead;
        while (modelList) {
            if (modelList->id == modelID) {
                obj->gfx->listEntry = modelList;
                return;
            }
            modelList = modelList->next;
        }
    }

    ModelList *list = malloc(sizeof(ModelList));
    list->entry = NULL;
    obj->gfx->listEntry = list;

    if (gModelIDListHead == NULL) {
        gModelIDListHead = list;
        list->prev = NULL;
    } else {
        gModelIDListTail->next = list;
        list->prev = gModelIDListTail;
    }
    gModelIDListTail = list;
    debugf("Loading model [%s].", gModelIDs[modelID - 1]);
    list->entry = NULL;
    list->active = false;
    list->model64 = t3d_model_load(asset_dir(gModelIDs[modelID - 1], DFS_MODEL64));
        ObjectModel *tail = NULL;
        T3DModel *mesh = list->model64;
        int primCount = mesh->chunkCount;
        for (int j = 0; j < primCount; j++) {
            ObjectModel *m;
            if (mesh->chunkOffsets[j].type == 'O') {
                m = malloc(sizeof(ObjectModel));
                uint32_t offset = mesh->chunkOffsets[j].offset & 0x00FFFFFF;
                T3DObject *obj = (T3DObject *) ((char *) mesh + offset);
                m->prim = (primitive_t *) obj;
            } else {
                continue;
            }
            m->material = NULL;
            m->colour = RGBA32(255, 255, 255, 255);
            if (modelID == 1) {
                m->materialID = playerModelTextures[j];
                if (m->materialID == MATERIAL_FLATPRIM || m->materialID == MATERIAL_EYE1 || 
                m->materialID == MATERIAL_MOUTH1 || m->materialID == MATERIAL_EYEBROW1 || m->materialID == MATERIAL_BODY1) {
                    m->colour = RGBA32(255, 126, 0, 255);
                }
                if (j == 1) {
                    m->colour = RGBA32(127, 48, 0, 255);
                }
            } else if (modelID == 4 || modelID == 5 || modelID == 6) {
                m->materialID = MATERIAL_CRATE;
            } else if (modelID == 7) {
                m->materialID = MATERIAL_PLANT1;
            } else if (modelID == 3) {
                m->materialID = MATERIAL_FLIPBOOKTEST;
            } else if (objectID == OBJ_LIGHTSOURCE) {
                m->materialID = MATERIAL_REFLECTION;
            } else {
                m->materialID = 0;
            }
            m->next = NULL;
            if (j == 0) {
                m->matrixBehaviour = matrixType;
            } else {
                m->matrixBehaviour = 0;
            }
            m->block = NULL;

            if (list->entry == NULL) {
                list->entry = m;
            } else {
                tail->next = m;
            }
            tail = m;
        }

    int animCount = t3d_model_get_animation_count(list->model64);

    if (animCount != 0) {
        list->animData = malloc(sizeof(void *) * animCount);
        t3d_model_get_animations(obj->gfx->listEntry->model64, list->animData);
    } else {
        list->animData = NULL;
    }

    debugf(" Time: %2.3fs.\n", (double) (TIMER_MICROS(DEBUG_SNAPSHOT_1_END) / 1000000.0f));

#ifdef PUPPYPRINT_DEBUG
    gNumModels++;
#endif
    list->next = NULL;
    list->timer = 10;
    list->id = modelID;
}

void obj_animation_init(Object *obj) {
    obj->animation = malloc(sizeof(ObjectAnimation));
    obj->animation->id[0] = ANIM_NONE;
    obj->animation->id[1] = ANIM_NONE;
    obj->animation->idPrev[0] = ANIM_NONE;
    obj->animation->idPrev[1] = ANIM_NONE;
    obj->animation->skeleton[0] = t3d_skeleton_create(obj->gfx->listEntry->model64);
    obj->animation->skeleton[1] = t3d_skeleton_clone(&obj->animation->skeleton[0], false);
    obj->animation->speed[0] = 0.05f;
    obj->animation->speed[1] = 0.05f;
}

void obj_animation_close(Object *obj) {
    for (int i = 0; i < 2; i++) {
        if (obj->animation->idPrev[i] != ANIM_NONE) {
            t3d_anim_destroy(&obj->animation->inst[i]);
        }
        t3d_skeleton_destroy(&obj->animation->skeleton[i]);
    }
    free(obj->animation);
}

void set_object_functions(Object *obj, int objectID) {
    if (gObjectModels[objectID]) {
        load_object_model(obj, objectID, MOD_OBJECT);
    }
    if (t3d_model_get_animation_count(obj->gfx->listEntry->model64)) {
        obj_animation_init(obj);
    }
    if (sObjectOverlays[objectID]) {
        init_object_behaviour(obj, objectID);
    }
}

void free_dynamic_shadow(Object *obj) {
    DynamicShadow *d = obj->gfx->dynamicShadow;
    debugf("Freeing dynamic shadow for [%s] object.\n", sObjectOverlays[obj->objectID]);
    surface_free(&d->surface);
    rspq_block_free(obj->gfx->dynamicShadow->block);
    free(obj->gfx->dynamicShadow);
    free_uncached(obj->gfx->dynamicShadow->mtx[0]);
    free_uncached(obj->gfx->dynamicShadow->mtx[1]);
    free_uncached(obj->gfx->dynamicShadow->verts);
    obj->gfx->dynamicShadow = NULL;
}

/**
 * Remove an object from the list, then reconnect the list.
 * Free the list entry, then the object, and any further elements from RAM.
*/
void free_object(Object *obj) {
    if (obj->entry == gObjectListHead) {
        if (gObjectListHead->next) {
            gObjectListHead = gObjectListHead->next;
            gObjectListHead->prev = NULL;
        } else {
            gObjectListHead = NULL;
        }
    } else {
        if (obj->entry == gObjectListTail) {
            gObjectListTail = gObjectListTail->prev;
        }
        obj->entry->prev->next = obj->entry->next;
        if (obj->entry->next) {
            obj->entry->next->prev = obj->entry->prev;
        }
    }
    if (obj->overlay) {
        check_unused_overlay(obj);
    }
    free(obj->entry);
    if (obj->data) {
        free(obj->data);
    }
    if (obj->collision) {
        free(obj->collision);
    }
    if (obj->movement) {
        free(obj->movement);
    }
    if (obj->animation) {
        obj_animation_close(obj);
    }
    if (obj->gfx) {
        if (obj->gfx->dynamicShadow) {
            free_dynamic_shadow(obj);
        }
        check_unused_model(obj);
        free(obj->gfx);
    }
    free(obj);
#ifdef PUPPYPRINT_DEBUG
    gNumObjects--;
#endif
}

void free_clutter(Clutter *obj) {
    if (obj->entry == gClutterListHead) {
        if (gClutterListHead->next) {
            gClutterListHead = gClutterListHead->next;
            gClutterListHead->prev = NULL;
        } else {
            gClutterListHead = NULL;
        }
    } else {
        if (obj->entry == gClutterListTail) {
            gClutterListTail = gClutterListTail->prev;
        }
        obj->entry->prev->next = obj->entry->next;
        if (obj->entry->next) {
            obj->entry->next->prev = obj->entry->prev;
        }
    }
    free(obj->entry);
    free_uncached(obj->matrix);
    if (obj->gfx) {
        check_unused_model((Object *) obj);
        free(obj->gfx);
    }
    free(obj);
#ifdef PUPPYPRINT_DEBUG
    gNumClutter--;
#endif
}

void free_particle(Particle *obj) {
    if (obj->entry == gParticleListHead) {
        if (gParticleListHead->next) {
            gParticleListHead = gParticleListHead->next;
            gParticleListHead->prev = NULL;
        } else {
            gParticleListHead = NULL;
        }
    } else {
        if (obj->entry == gParticleListTail) {
            gParticleListTail = gParticleListTail->prev;
        }
        obj->entry->prev->next = obj->entry->next;
        if (obj->entry->next) {
            obj->entry->next->prev = obj->entry->prev;
        }
    }
    free(obj->entry);
    /*if (obj->material) {
        free(obj->material);
    }*/
    free(obj);
#ifdef PUPPYPRINT_DEBUG
    gNumParticles--;
#endif
}

Object *spawn_object_pos(int objectID, float x, float y, float z) {
    Object *obj = allocate_object();
    if (obj == NULL) {
        return NULL;
    }
    obj->pos[0] = x;
    obj->pos[1] = y;
    obj->pos[2] = z;
    obj->faceAngle[0] = 0;
    obj->faceAngle[1] = 0;
    obj->faceAngle[2] = 0;
    obj->scale[0] = 1.0f;
    obj->scale[1] = 1.0f;
    obj->scale[2] = 1.0f;
    obj->objectID = objectID;
    set_object_functions(obj, objectID);
    return obj;
}

Object *spawn_object_pos_angle(int objectID, float x, float y, float z, short pitch, short roll, short yaw) {
    Object *obj = spawn_object_pos(OBJ_NULL, x, y, z);
    if (obj == NULL) {
        return NULL;
    }
    obj->faceAngle[0] = pitch;
    obj->faceAngle[1] = roll;
    obj->faceAngle[2] = yaw;
    obj->objectID = objectID;
    set_object_functions(obj, objectID);
    return obj;
}

Object *spawn_object_pos_angle_scale(int objectID, float x, float y, float z, short pitch, short roll, short yaw, float scaleX, float scaleY, float scaleZ) {
    Object *obj = spawn_object_pos(OBJ_NULL, x, y, z);
    if (obj == NULL) {
        return NULL;
    }
    obj->faceAngle[0] = pitch;
    obj->faceAngle[1] = roll;
    obj->faceAngle[2] = yaw;
    obj->scale[0] = scaleX;
    obj->scale[1] = scaleY;
    obj->scale[2] = scaleZ;
    obj->objectID = objectID;
    set_object_functions(obj, objectID);
    return obj;
}

Object *spawn_object_pos_scale(int objectID, float x, float y, float z, float scaleX, float scaleY, float scaleZ) {
    Object *obj = spawn_object_pos(OBJ_NULL, x, y, z);
    if (obj == NULL) {
        return NULL;
    }
    obj->scale[0] = scaleX;
    obj->scale[1] = scaleY;
    obj->scale[2] = scaleZ;
    obj->objectID = objectID;
    set_object_functions(obj, objectID);
    return obj;
}

Clutter *spawn_clutter(int objectID, float x, float y, float z, short pitch, short roll, short yaw) {
    Clutter *clutter = allocate_clutter();
    if (clutter == NULL) {
        return NULL;
    }
    clutter->pos[0] = x;
    clutter->pos[1] = y;
    clutter->pos[2] = z;
    clutter->faceAngle[0] = pitch;
    clutter->faceAngle[1] = roll;
    clutter->faceAngle[2] = yaw;
    clutter->scale[0] = 1.0f;
    clutter->scale[1] = 1.0f;
    clutter->scale[2] = 1.0f;
    clutter->objectID = objectID;
    Matrix mtx;
    if (gClutterModels[objectID]) {
        load_object_model((Object *) clutter, objectID, MOD_CLUTTER);
        clutter->gfx->width = 32.0f;
        clutter->gfx->height = 80.0f;
        clutter->gfx->yOffset = 0;
    }
    clutter_matrix(&mtx, clutter->gfx->listEntry->entry->matrixBehaviour, clutter->pos, clutter->faceAngle, clutter->scale);
    t3d_mat4_to_fixed_3x4(clutter->matrix, (T3DMat4  *) &mtx);
    return clutter;
}

Particle *spawn_particle(int particleID, float x, float y, float z) {
    Particle *particle = allocate_particle();
    if (particle == NULL) {
        return NULL;
    }
    particle->pos[0] = x;
    particle->pos[1] = y;
    particle->pos[2] = z;
    particle->scale[0] = 1.0f;
    particle->scale[1] = 1.0f;
    particle->scale[2] = 1.0f;
    //particle->material = NULL;
    return particle;
}