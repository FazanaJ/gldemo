#pragma once

#include "global.h"

#include "../src/assets.h"

const MaterialInfo gMaterialIDs[] = {
    {TEXTURE_GRASS0, TEXTURE_GRASS1, 1664, CC_MULTITEX_SHADE, COLFLAG_SOUND_DIRT, 0, 0, 3, 3, 0, 0, 0, 0},
    {TEXTURE_HEALTH, -1, 35, CC_TEX_SHADE, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {TEXTURE_PLANT1, -1, 1174 | MAT_DEPTH_READ, CC_TEX_SHADE, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {TEXTURE_SHADOW, -1, MAT_XLU | MAT_DEPTH_READ, CC_TEX_SHADE, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {TEXTURE_STONE, -1, 1664 | MAT_CI, CC_TEX_SHADE, COLFLAG_SOUND_STONE, 0, 0, 0, 0, 0, 0, 0, 0},
    {TEXTURE_WATER, TEXTURE_WATER, 12960, CC_MULTITEX_WATER, 0, 0, 1, 1, 0, 2, 0, 2, 2},
    {TEXTURE_KITCHENTILE, -1, 1664, CC_TEX_SHADE, COLFLAG_SOUND_TILE, 0, 0, 0, 0, 0, 0, 0, 0},
    {TEXTURE_INTROSIGN, -1, 1664, CC_TEX_SHADE, COLFLAG_SOUND_WOOD, 0, 0, 0, 0, 0, 0, 0, 0},
    {TEXTURE_INTROSIGN2, -1, 1664, CC_TEX_SHADE, COLFLAG_SOUND_WOOD, 0, 0, 0, 0, 0, 0, 0, 0},
    {TEXTURE_EYE1, -1, 726 | MAT_LIGHTING, CC_TEX_SHADE, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {TEXTURE_EYEBROW1, -1, 710 | MAT_LIGHTING, CC_TEX_SHADE, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {TEXTURE_MOUTH1, -1, 2774 | MAT_LIGHTING, CC_TEX_SHADE, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {TEXTURE_SHIRT, -1, 704 | MAT_CI | MAT_LIGHTING, CC_TEX_SHADE, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {TEXTURE_TROUSERS, -1, 704 | MAT_CI | MAT_LIGHTING, CC_TEX_SHADE, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {TEXTURE_ROCKSURFACE4, -1, 1664 | MAT_CI, CC_TEX_SHADE, COLFLAG_SOUND_STONE, 0, 0, 0, 0, 0, 0, 0, 0},
    {TEXTURE_MOUNTAINSIDEBOTTOM, -1, 1664 | MAT_CI, CC_TEX_SHADE, COLFLAG_SOUND_STONE, 0, 0, 0, 0, 0, 0, 0, 0},
    {-1, -1, 704, CC_SHADE_PRIM, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {TEXTURE_LOGWALL, -1, 1664, CC_TEX_SHADE, COLFLAG_SOUND_WOOD, 0, 0, 0, 0, 0, 0, 0, 0},
    {TEXTURE_RAILING, -1, 1680, CC_TEX_SHADE, COLFLAG_SOUND_METAL, 0, 0, 0, 0, 0, 0, 0, 0},
    {TEXTURE_CRATE, -1, 1740, CC_TEX_SHADE, COLFLAG_SOUND_WOOD, 0, 0, 0, 0, 0, 0, 0, 0},
    {-1, -1, MAT_DEPTH_READ | MAT_XLU | MAT_VTXCOL, CC_SHADE_PRIM, COLFLAG_SOUND_GLASS, 0, 0, 0, 0, 0, 0, 0, 0},
    {TEXTURE_GRASS0, TEXTURE_GRASS1, MAT_VTXCOL, CC_MULTITEX_SHADE, COLFLAG_SOUND_DIRT, 0, 0, 3, 3, 0, 0, 0, 0},
};