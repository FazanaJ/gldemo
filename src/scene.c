#include <libdragon.h>
#include <malloc.h>

#include "scene.h"
#include "../include/global.h"

#include "object.h"
#include "camera.h"
#include "render.h"

SceneBlock *sCurrentScene;
Environment *gEnvironment;

ObjectMap sTestAreaObjs[] = {
    {OBJ_PLAYER, MAP_OBJ, /*Yaw*/ 0, /*X*/ 0, /*Y*/ 0, /*Z*/ 0},
    {OBJ_NPC, MAP_OBJ, /*Yaw*/ 0, /*X*/ 50, /*Y*/ 0, /*Z*/ 0},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ 20, /*Y*/ 0, /*Z*/ 40},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ 10, /*Y*/ 0, /*Z*/ 45},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ 35, /*Y*/ 0, /*Z*/ -100},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ 75, /*Y*/ 0, /*Z*/ -25},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ 60, /*Y*/ 0, /*Z*/ 40},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ -25, /*Y*/ 0, /*Z*/ 45},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ -95, /*Y*/ 0, /*Z*/ 40},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ 45, /*Y*/ 0, /*Z*/ 0},
    {CLUTTER_BUSH, MAP_CLU, /*Yaw*/ 0, /*X*/ 75, /*Y*/ 0, /*Z*/ -75},

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

light_t sEnvironmentLight = {
    color: { 0.51f, 0.81f, 0.665f, 0.5f},
    diffuse: {1.0f, 1.0f, 1.0f, 1.0f},
    direction: {0.0f, -60.0f, 0.0f},
    position: {1.0f, 0.0f, 0.0f, 0.0f},
    radius: 10.0f,
};

void setup_fog(light_t light) {
    if (gEnvironment == NULL) {
        gEnvironment = malloc(sizeof(Environment));
    }

    gEnvironment->fogColour[0] = light.color[0];
    gEnvironment->fogColour[1] = light.color[1];
    gEnvironment->fogColour[2] = light.color[2];
    gEnvironment->skyColourBottom[0] = light.color[0] * 0.66f;
    gEnvironment->skyColourBottom[1] = light.color[1] * 0.66f;
    gEnvironment->skyColourBottom[2] = light.color[2] * 0.66f;
    gEnvironment->skyColourTop[0] = light.color[0] * 1.33f;
    if (gEnvironment->skyColourTop[0] > 1.0f) {
        gEnvironment->skyColourTop[0] = 1.0f;
    }
    gEnvironment->skyColourTop[1] = light.color[1] * 1.33f;
    if (gEnvironment->skyColourTop[1] > 1.0f) {
        gEnvironment->skyColourTop[1] = 1.0f;
    }
    gEnvironment->skyColourTop[2] = light.color[2] * 1.33f;
    if (gEnvironment->skyColourTop[2] > 1.0f) {
        gEnvironment->skyColourTop[2] = 1.0f;
    }
    gEnvironment->flags = ENV_FOG;
    gEnvironment->fogNear = 150.0f;
    gEnvironment->fogFar = 400.0f;
    glFogf(GL_FOG_START, gEnvironment->fogNear);
    glFogf(GL_FOG_END, gEnvironment->fogFar);
    glFogfv(GL_FOG_COLOR, gEnvironment->fogColour);
}

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
        mesh_t *mesh = model64_get_mesh(s->model, i);
        int primCount = model64_get_primitive_count(mesh);
        for (int j = 0; j < primCount; j++) {
            SceneMesh *m = malloc(sizeof(SceneMesh));
            m->mesh = model64_get_primitive(mesh, j);
            m->material = malloc(sizeof(Material));
            m->material->index = NULL;
            m->material->textureID = sSceneTexIDs[sCurrentScene->sceneID][i];
            m->material->flags = 0;
            m->flags = sSceneMeshFlags[sCurrentScene->sceneID][i];
            m->next = NULL;
            rspq_block_begin();
            glPushMatrix();
            glScalef(5.0f, 5.0f, 5.0f);
            model64_draw_primitive(m->mesh);
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
    }
    
    if (t->objectMap) {
        int i = 0;
        ObjectMap *o = &t->objectMap[i];

        while (1) {
            if (o->type == MAP_OBJ) {
                Object *obj = spawn_object_pos(o->objectID, o->x, o->y, o->z);
                obj->faceAngle[1] = o->yaw;
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
    setup_fog(sEnvironmentLight);
    camera_init();
}