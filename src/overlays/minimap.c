#include <libdragon.h>
#include <malloc.h>

#include "../../include/global.h"

#include "../assets.h"
#include "../object.h"
#include "../main.h"
#include "../math_util.h"
#include "../scene.h"
#include "../camera.h"

sprite_t *sMinimapArrow;
sprite_t *sMinimapSprite;
sprite_t *sMinimapSight;
SceneMap *sMiniMap;
float sMapOpacity;
float sMapOpacityTarget;
float sMapSizeX;
float sMapSizeY;
float sMapDrawSizeX;
float sMapDrawSizeY;
float sMapBoundsX;
float sMapBoundsZ;

void loop(int updateRate, float updateRateF) {
    if (sMiniMap == NULL) {
        return;
    }
    int width = display_get_width();
    int height = display_get_height();
    float charPosX = (gPlayer->pos[0] / sMapBoundsX) * ((float) sMapSizeX);
    float charPosY = (gPlayer->pos[2] / sMapBoundsZ) * ((float) sMapSizeY);
    float charAngle = SHORT_TO_RADIANS(gPlayer->faceAngle[1] + 0x8000);
    float sightAngle = SHORT_TO_RADIANS(gCamera->yaw);
    sMapOpacity = lerpf(sMapOpacity, sMapOpacityTarget, 0.1f * updateRateF);
    rdpq_set_mode_standard();
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_combiner(RDPQ_COMBINER_TEX_FLAT);
    rdpq_set_prim_color(RGBA32(255, 255, 255, 192 * sMapOpacity));
    rdpq_sprite_blit(sMinimapSprite, width - (sMapDrawSizeX * 0.75f), height - (sMapDrawSizeY * 0.75f), 
        &(rdpq_blitparms_t) {.scale_x = 1.0f, .scale_y = 1.0f, .cx = sMapDrawSizeX / 2, .cy = sMapDrawSizeY / 2});
    
    int sineCol = 64 + (32 * sins(gGameTimer * 0x800));
    rdpq_set_prim_color(RGBA32(255, 255, sineCol, 96 * sMapOpacity));
    rdpq_sprite_blit(sMinimapSight, width - (sMapDrawSizeX * 0.75f) + charPosX + sMiniMap->offsetX, height - (sMapDrawSizeY * 0.75f) + charPosY + sMiniMap->offsetY, 
        &(rdpq_blitparms_t) {.cx = 7, .cy = 14, .theta = sightAngle});

    rdpq_set_prim_color(RGBA32(255, 0, 0, 255 * sMapOpacity));
    rdpq_sprite_blit(sMinimapArrow, width - (sMapDrawSizeX * 0.75f) + charPosX + sMiniMap->offsetX, height - (sMapDrawSizeY * 0.75f) + charPosY + sMiniMap->offsetY, 
        &(rdpq_blitparms_t) {.cx = 4, .cy = 4, .theta = charAngle});
}

void init(void) {
    debugf("Loading overlay: [minimap]\n");
    SceneHeader *header = dlsym(gCurrentScene->overlay, "header");
    sMiniMap = header->map;
    if (sMiniMap == NULL) {
        return;
    }
    sMinimapArrow = sprite_load(asset_dir("maparrow.ia4", DFS_SPRITE));
    sMinimapSight = sprite_load(asset_dir("sight.i4", DFS_SPRITE));
    sMinimapSprite = sprite_load(asset_dir(sMiniMap->texture, DFS_SPRITE));
    surface_t surf = sprite_get_pixels(sMinimapSprite);
    sMapDrawSizeX = surf.width;
    sMapDrawSizeY = surf.height;
    sMapSizeX = surf.width * sMiniMap->scaleX;
    sMapSizeY = surf.height * sMiniMap->scaleY;
    sMapBoundsX = gCurrentScene->bounds[1][0] - gCurrentScene->bounds[0][0];
    sMapBoundsZ = gCurrentScene->bounds[1][2] - gCurrentScene->bounds[0][2];
    sMapOpacityTarget = 1.0f;
    sMapOpacity = 0.0f;
}

void destroy(void) {
    debugf("Closing overlay: [minimap]\n");
    if (sMinimapArrow) {
        sprite_free(sMinimapArrow);
    }
    if (sMinimapSprite) {
        sprite_free(sMinimapSprite);
    }
    if (sMinimapSight) {
        sprite_free(sMinimapSight);
    }
}