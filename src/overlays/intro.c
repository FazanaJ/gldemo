#include <libdragon.h>
#include <malloc.h>

#include "../../include/global.h"

#include "../scene.h"
#include "../object.h"

ObjectMap objects[] = {
    {CLUTTER_ROCK, MAP_CLU, /*Yaw*/ 0, /*X*/ 20, /*Y*/ 0, /*Z*/ 40},

    {-1, MAP_CLU, /*Yaw*/ 0, /*X*/ 75, /*Y*/ -75, /*Z*/ 0},
};

SceneHeader header = {
    /*Model*/           "intro",
    /*Objects*/         objects, 
    /*Fog Near*/        0, 
    /*Fog Far*/         0, 
    /*Fog Colour*/      {0, 0, 0},
    /*Light Colour*/    {0xFF, 0xFF, 0xFF},
    /*Light Ambient*/   {0xFF, 0xFF, 0xFF},
    /*Sky Top*/         {0, 0, 0},
    /*Sky Bottom*/      {0xFF, 0xFF, 0xFF},
    /*Flags*/           ENV_NONE
};