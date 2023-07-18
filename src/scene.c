#include <libdragon.h>
#include <malloc.h>

#include "scene.h"
#include "../include/global.h"

#include "object.h"
#include "camera.h"
#include "render.h"

SceneBlock *sCurrentScene;

ObjectMap sTestAreaObjs[] = {
    {OBJ_PLAYER, MAP_OBJ, /*Yaw*/ 0, /*X*/ 0, /*Y*/ 0, /*Z*/ 0},
    {OBJ_NPC, MAP_OBJ, /*Yaw*/ 0, /*X*/ 50, /*Y*/ 0, /*Z*/ 0},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ 20, /*Y*/ 40, /*Z*/ 0},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ 10, /*Y*/ 45, /*Z*/ 0},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ 35, /*Y*/ -100, /*Z*/ 0},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ 75, /*Y*/ -25, /*Z*/ 0},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ 60, /*Y*/ 40, /*Z*/ 0},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ -25, /*Y*/ 45, /*Z*/ 0},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ -95, /*Y*/ 40, /*Z*/ 0},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ 45, /*Y*/ 0, /*Z*/ 0},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ 75, /*Y*/ -75, /*Z*/ 0},

    {-1, MAP_CLU, /*Yaw*/ 0, /*X*/ 75, /*Y*/ -75, /*Z*/ 0},
};

SceneIDs sSceneTable[] = {
    {"intro", NULL, NULL},
    {"testarea", sTestAreaObjs, NULL},
};

char sSceneTexIDs[][4] = {
    {6, 7},
    {4, 0, 5},
};

int sSceneMeshFlags[][4] = {
    {MATERIAL_DEPTH_READ, 
    MATERIAL_DEPTH_READ},

    {MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_VTXCOL, 
    MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_VTXCOL, 
    MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_XLU},
};

static void clear_scene(void) {
    SceneMesh *curMesh = sCurrentScene->meshList;
    clear_objects();
    gPlayer = NULL;
    while (curMesh) {
        SceneMesh *m = curMesh;
        curMesh = curMesh->next;
        free(m->material);
        rspq_block_free(m->renderBlock);
        free(m);
    }
    if (gCamera) {
        free(gCamera);
    }
    if (gEnvironment) {
        //free(gEnvironment);
    }
    free(sCurrentScene->model);
    free(sCurrentScene);
}

void load_scene(int sceneID) {
    if (sCurrentScene) {
        clear_scene();
    }
    SceneIDs *t = &sSceneTable[sceneID];
    sCurrentScene = malloc(sizeof(SceneBlock));
    SceneBlock *s = sCurrentScene;
    s->model = model64_load(asset_dir(t->model, DFS_MODEL64));
    s->meshList = NULL;
    s->sceneID = sceneID;
    int numMeshes = model64_get_mesh_count(s->model);
    SceneMesh *tail = NULL;
    for (int i = 0; i < numMeshes; i++) {
        SceneMesh *m = malloc(sizeof(SceneMesh));
        m->mesh = model64_get_mesh(s->model, i);
        m->material = malloc(sizeof(Material));
        m->material->index = NULL;
        m->material->textureID = sSceneTexIDs[sCurrentScene->sceneID][i];
        m->material->flags = 0;
        m->flags = sSceneMeshFlags[sCurrentScene->sceneID][i];
        m->next = NULL;
        rspq_block_begin();
        glPushMatrix();
        glScalef(5.0f, 5.0f, 5.0f);
        model64_draw_mesh(m->mesh);
        glPopMatrix();
        m->renderBlock = rspq_block_end();

        if (s->meshList == NULL) {
            s->meshList = m;
            tail = m;
        } else {
            tail->next = m;
            tail = m;
        }
    }
    
    if (t->objectMap) {
        int i = 0;
        ObjectMap *o = &t->objectMap[i];

        while (1) {
            if (o->type == MAP_OBJ) {
                Object *obj = spawn_object_pos(o->objectID, o->x, o->y, o->z);
                obj->faceAngle[2] = o->yaw;
                if (o->objectID == OBJ_PLAYER) {
                    gPlayer = obj;
                }
            } else {
                spawn_clutter(o->objectID, o->x, o->y, o->z, 0, o->yaw, 0);
            }

            if (t->objectMap[i + 1].objectID == -1) {
                break;
            } else {
                i++;
                o = &t->objectMap[i];
            }
        }
    }
    camera_init();
}