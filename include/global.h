#pragma once

#define TEXTURE_NONE -1
#define PACKED __attribute__((__packed__))

#define SCREEN_WIDTH 320
#define SCREEN_WIDTH_16_10 384
#define SCREEN_WIDTH_16_9 424
#define SCREEN_HEIGHT 240

typedef short Texture;

#include "config.h"
#include "enums.h"
#include "types.h"

#if !defined(OPENGL) && !defined(TINY3D)
    #define TINY3D 1
#endif
