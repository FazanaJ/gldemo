#include <libdragon.h>
#include <malloc.h>

#include "../../include/global.h"

#include "../scene.h"
#include "../object.h"

ObjectMap objects[] = {
    {OBJ_PLAYER, MAP_OBJ, /*Yaw*/ 0, /*X*/ 0, /*Y*/ 0, /*Z*/ 0},
    {OBJ_LIGHTSOURCE, MAP_OBJ, /*Yaw*/ 0, /*X*/ 30, /*Y*/ 0, /*Z*/ -30 * 8},
    {OBJ_CRATE, MAP_OBJ, /*Yaw*/ 0, /*X*/ 0, /*Y*/ 0, /*Z*/ -30 * 8},
    {OBJ_BARREL, MAP_OBJ, /*Yaw*/ 0, /*X*/ 0, /*Y*/ -10 * 8, /*Z*/ -60 * 8},
    {OBJ_TESTSPHERE, MAP_OBJ, /*Yaw*/ 0, /*X*/ 0, /*Y*/ 0, /*Z*/ -90 * 8},

    {-1, MAP_CLU, /*Yaw*/ 0, /*X*/ 75, /*Y*/ -75, /*Z*/ 0},
};

SceneMap map = {
    "testarea2.i4",
    1.0f,
    1.0f,
    15,
    22,
};

SceneHeader header = {
    /*Model*/           "testarea2",
    /*Objects*/         objects, 
    /*Fog Near*/        500, 
    /*Fog Far*/         1000, 
    /*Fog Colour*/      {125, 78, 37},
    /*Light Colour*/    {0xFF, 0xFF, 0xFF},
    /*Light Ambient*/   {0xFF, 0xFF, 0xFF},
    /*Sky Top*/         {108, 60, 24},
    /*Sky Bottom*/      {143, 95, 50},
    /*Flags*/           ENV_FOG,
    /*Sky Texture*/     -1,
    /*Model Count*/     1,
    /*Map*/             &map,
};