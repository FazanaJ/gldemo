#include <libdragon.h>
#include <malloc.h>

#include "../../include/global.h"

#include "../scene.h"
#include "../object.h"

SceneHeader header = {
    /*Model*/           "intro",
    /*Objects*/         NULL, 
    /*Fog Near*/        0, 
    /*Fog Far*/         0, 
    /*Fog Colour*/      {0, 0, 0},
    /*Light Colour*/    {0xFF, 0xFF, 0xFF},
    /*Light Ambient*/   {0xFF, 0xFF, 0xFF},
    /*Sky Top*/         {0, 0, 0},
    /*Sky Bottom*/      {0xFF, 0xFF, 0xFF},
    /*Flags*/           ENV_NONE,
    /*Sky Texture*/     -1,
    /*Model Count*/     1,
};