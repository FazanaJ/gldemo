#include <libdragon.h>
#include <malloc.h>

#include "../../include/global.h"

#include "../scene.h"
#include "../object.h"

ObjectMap objects[] = {
    {OBJ_PLAYER, MAP_OBJ, /*Yaw*/ 0, /*X*/ 0, /*Y*/ 0, /*Z*/ 0},
    {OBJ_NPC, MAP_OBJ, /*Yaw*/ 0, /*X*/ 50, /*Y*/ 0, /*Z*/ 0},
    {CLUTTER_ROCK, MAP_CLU, /*Yaw*/ 0, /*X*/ 20, /*Y*/ 0, /*Z*/ 40},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ 10, /*Y*/ 0, /*Z*/ 45},
    {CLUTTER_ROCK, MAP_CLU, /*Yaw*/ 0, /*X*/ 35, /*Y*/ 0, /*Z*/ -100},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ 75, /*Y*/ 0, /*Z*/ -25},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ 60, /*Y*/ 0, /*Z*/ 40},
    {CLUTTER_ROCK, MAP_CLU, /*Yaw*/ 0, /*X*/ -25, /*Y*/ 0, /*Z*/ 45},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ -95, /*Y*/ 0, /*Z*/ 40},
    {CLUTTER_ROCK, MAP_CLU, /*Yaw*/ 0, /*X*/ 45, /*Y*/ 0, /*Z*/ 0},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ 75, /*Y*/ 0, /*Z*/ -75},

    {-1, MAP_CLU, /*Yaw*/ 0, /*X*/ 75, /*Y*/ -75, /*Z*/ 0},
};

SceneHeader header = {
    /*Model*/           "testarea",
    /*Objects*/         objects, 
    /*Fog Near*/        150, 
    /*Fog Far*/         400, 
    /*Fog Colour*/      {127, 220, 160},
    /*Light Colour*/    {0xFF, 0xFF, 0xFF},
    /*Light Ambient*/   {0xFF, 0xFF, 0xFF},
    /*Sky Top*/         {127, 220, 160},
    /*Sky Bottom*/      {0xFF, 0xFF, 0xFF},
    /*Flags*/           ENV_NONE
};