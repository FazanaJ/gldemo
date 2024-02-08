#include <libdragon.h>
#include <malloc.h>

#include "../../include/global.h"

#include "../scene.h"
#include "../object.h"

ObjectMap objects[] = {
    {OBJ_PLAYER, MAP_OBJ, /*Yaw*/ 0, /*X*/ 0, /*Y*/ 0, /*Z*/ 0},
    {OBJ_CRATE, MAP_OBJ, /*Yaw*/ 0, /*X*/ 0, /*Y*/ 0, /*Z*/ -30},
    {OBJ_BARREL, MAP_OBJ, /*Yaw*/ 0, /*X*/ 0, /*Y*/ -10, /*Z*/ -60},
    {OBJ_TESTSPHERE, MAP_OBJ, /*Yaw*/ 0, /*X*/ 0, /*Y*/ 0, /*Z*/ -90},

    {-1, MAP_CLU, /*Yaw*/ 0, /*X*/ 75, /*Y*/ -75, /*Z*/ 0},
};

SceneHeader header = {
    /*Model*/           "testarea2",
    /*Objects*/         objects, 
    /*Fog Near*/        150, 
    /*Fog Far*/         400, 
    /*Fog Colour*/      {125, 78, 37},
    /*Light Colour*/    {0xFF, 0xFF, 0xFF},
    /*Light Ambient*/   {0xFF, 0xFF, 0xFF},
    /*Sky Top*/         {108, 60, 24},
    /*Sky Bottom*/      {143, 95, 50},
    /*Flags*/           ENV_FOG,
    /*Sky Texture*/     TEXTURE_SKYBOX1,
};