#include <libdragon.h>
#include <malloc.h>

#include "../../include/global.h"

#include "../scene.h"
#include "../object.h"

ObjectMap objects[] = {
    {OBJ_PLAYER, MAP_OBJ, /*Yaw*/ 0, /*X*/ 0, /*Y*/ 0, /*Z*/ 0},
    {OBJ_NPC, MAP_OBJ, /*Yaw*/ 0, /*X*/ 50 * 8, /*Y*/ 0, /*Z*/ 0},
    {CLUTTER_FLIPBOOKTEST, MAP_CLU, /*Yaw*/ 0, /*X*/ 20 * 8, /*Y*/ 0, /*Z*/ 40 * 8},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ 10 * 8, /*Y*/ 0, /*Z*/ 45 * 8},
    {CLUTTER_ROCK, MAP_CLU, /*Yaw*/ 0, /*X*/ 35 * 8, /*Y*/ 0, /*Z*/ -100 * 8},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ 75 * 8, /*Y*/ 0, /*Z*/ -25 * 8},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ 60 * 8, /*Y*/ 0, /*Z*/ 40 * 8},
    {CLUTTER_ROCK, MAP_CLU, /*Yaw*/ 0, /*X*/ -25 * 8, /*Y*/ 0, /*Z*/ 45 * 8},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ -95 * 8, /*Y*/ 0, /*Z*/ 40 * 8},
    {CLUTTER_ROCK, MAP_CLU, /*Yaw*/ 0, /*X*/ 45 * 8, /*Y*/ 0, /*Z*/ 0},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ 75 * 8, /*Y*/ 0, /*Z*/ -75 * 8},

    {-1, MAP_CLU, /*Yaw*/ 0, /*X*/ 75, /*Y*/ -75, /*Z*/ 0},
};

SceneMap map = {
    "testarea1.i4",
    1.5f,
    1.9f,
    0,
    0,
};

SceneHeader header = {
    /*Model*/           "testarea",
    /*Objects*/         objects, 
    /*Fog Near*/        450, 
    /*Fog Far*/         900, 
    /*Fog Colour*/      {125, 78, 37},
    /*Light Colour*/    {0xFF, 0xFF, 0xFF},
    /*Light Ambient*/   {0xFF, 0xFF, 0xFF},
    /*Sky Top*/         {108, 60, 24},
    /*Sky Bottom*/      {143, 95, 50},
    /*Flags*/           ENV_FOG,
    /*Sky Texture*/     TEXTURE_SKYBOX1,
    /*Model Count*/     1,
    /*Map*/             &map,
};