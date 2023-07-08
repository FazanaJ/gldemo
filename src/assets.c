#include <libdragon.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>
#include <malloc.h>

#include "assets.h"
#include "../include/global.h"

#include "debug.h"
#include "render.h"
#include "main.h"

const char *sTextureIDs[] = {
    "rom:/grass0.ci4.sprite",
    "rom:/health.i8.sprite",
    "rom:/plant1.ia8.sprite",
};

RenderSettings sRenderSettings;
int sPrevRenderFlags;
MaterialList *gMaterialListHead;
MaterialList *gMaterialListTail;
static Material *sCurrentMaterial;
short gNumTextures;
short gNumTextureLoads;

void setup_textures(GLuint textures[], sprite_t *sprites[], const char *texture_path[], int texture_number) {

    for (uint32_t i = 0; i < texture_number; i++){
        if (sprites[i] == NULL) {
            sprites[i] = sprite_load(texture_path[i]);
        }
    }

    glGenTextures(texture_number, textures);

    for (uint32_t i = 0; i < texture_number; i++){
        int repeat;

        if (i == 2 || i == 3) {
            repeat = false;
        } else {
            repeat = REPEAT_INFINITE;
        }

        glBindTexture(GL_TEXTURE_2D, textures[i]);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glSpriteTextureN64(GL_TEXTURE_2D, sprites[i], &(rdpq_texparms_t){.s.repeats = repeat, .t.repeats = repeat});
    }
}


void init_materials(void) {
    bzero(&sRenderSettings, sizeof(RenderSettings));
    sPrevRenderFlags = 0;
    gMaterialListHead = NULL;
    gMaterialListTail = NULL;
    gNumTextures = 0;
    gNumTextureLoads = 0;
    sCurrentMaterial = NULL;
}

void bind_new_texture(MaterialList *material) {
    int repeat;

        if (material->textureID == 2 || material->textureID == 3) {
            repeat = false;
        } else {
            repeat = REPEAT_INFINITE;
        }
    glBindTexture(GL_TEXTURE_2D, material->texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glSpriteTextureN64(GL_TEXTURE_2D, material->sprite, &(rdpq_texparms_t){.s.repeats = repeat, .t.repeats = repeat});
}

int load_texture(Material *material) {
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
    list->sprite = sprite_load(sTextureIDs[material->textureID]);
    glGenTextures(1, &list->texture);
    bind_new_texture(list);
    list->textureID = material->textureID;
    list->loadTimer = 120;
    material->index = list;
    gNumTextures++;
    return 0;
}

void free_material(MaterialList *material) {
    if (material == gMaterialListHead) {
        if (gMaterialListHead->next) {
            gMaterialListHead = gMaterialListHead->next;
            gMaterialListHead->prev = NULL;
            gMaterialListTail = gMaterialListHead;
        } else {
            gMaterialListHead = NULL;
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
    gNumTextures--;
}

void cycle_textures(int updateRate) {
    DEBUG_SNAPSHOT_1();
    if (gMaterialListHead == NULL) {
        get_time_snapshot(PP_MATERIALS, DEBUG_SNAPSHOT_1_END);
        return;
    }
    MaterialList *list = gMaterialListHead;

    while (list) {
        MaterialList *curList = list;
        list->loadTimer -= updateRate;
        list = list->next;
        if (curList->loadTimer <= 0) {
            free_material(curList);
            break;
        }
    }
    gNumTextureLoads = 0;
    sPrevRenderFlags = 0;
    bzero(&sRenderSettings, sizeof(RenderSettings));
    get_time_snapshot(PP_MATERIALS, DEBUG_SNAPSHOT_1_END);
}

void set_render_settings(int flags) {
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
            sRenderSettings.xlu = true;
        }
    } else {
        if (sRenderSettings.xlu) {
            glDisable(GL_BLEND);
            sRenderSettings.xlu = false;
        }
    }
    if (flags & MATERIAL_DEPTH_WRITE) {
        if (!sRenderSettings.depthWrite) {
            glEnable(GL_DEPTH_TEST);
            sRenderSettings.depthWrite = true;
        }
    } else {
        if (sRenderSettings.depthWrite) {
            glDisable(GL_DEPTH_TEST);
            sRenderSettings.depthWrite = false;
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
}

void set_material(Material *material) {
    DEBUG_SNAPSHOT_1();
    if (sCurrentMaterial == material) {
        get_time_snapshot(PP_MATERIALS, DEBUG_SNAPSHOT_1_END);
        return;
    }
    if (material->textureID != -1) {
        //if (material->index == NULL) {
            if (load_texture(material) == -1) {
                get_time_snapshot(PP_MATERIALS, DEBUG_SNAPSHOT_1_END);
                return;
            }
        //}
        
        if (!sRenderSettings.texture) {
            glEnable(GL_TEXTURE_2D);
            sRenderSettings.texture = true;
        }
        material->index->loadTimer = 120;
        glBindTexture(GL_TEXTURE_2D, material->index->texture);
    } else {
        if (sRenderSettings.texture) {
            glDisable(GL_TEXTURE_2D);
            sRenderSettings.texture = false;
        }
    }

    gNumTextureLoads++;
    if (sPrevRenderFlags != material->flags) {
        set_render_settings(material->flags);
    }

    sCurrentMaterial = material;
    sPrevRenderFlags = material->flags;
    get_time_snapshot(PP_MATERIALS, DEBUG_SNAPSHOT_1_END);
}