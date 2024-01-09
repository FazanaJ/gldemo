#include <libdragon.h>
#include <malloc.h>

#include "scene.h"
#include "../include/global.h"

#include "object.h"
#include "camera.h"
#include "render.h"
#include "debug.h"
#include "hud.h"

SceneBlock *sCurrentScene;
Environment *gEnvironment;
char gSceneUpdate;

char *sSceneTable[] = {
    "intro",
    "testarea",
};

char sSceneTexIDs[][4] = {
    {TEXTURE_INTROSIGN, TEXTURE_KITCHENTILE, TEXTURE_INTEROSIGN2},
    {TEXTURE_STONE, TEXTURE_GRASS0, TEXTURE_WATER},
};

int sSceneMeshFlags[][4] = {
    {MATERIAL_DEPTH_READ, 
    MATERIAL_DEPTH_READ},

    {MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_VTXCOL, 
    MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_VTXCOL, 
    MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_XLU},
};

void setup_fog(SceneHeader *header) {
    if (gEnvironment == NULL) {
        gEnvironment = malloc(sizeof(Environment));
    }
    gEnvironment->fogColour[0] = ((float) header->fogColour[0]) / 255.0f;
    gEnvironment->fogColour[1] = ((float) header->fogColour[1]) / 255.0f;
    gEnvironment->fogColour[2] = ((float) header->fogColour[2]) / 255.0f;
    gEnvironment->fogColour[3] = 1.0f;
    gEnvironment->skyColourBottom[0] = ((float) header->skyBottom[0]) / 255.0f;
    gEnvironment->skyColourBottom[1] = ((float) header->skyBottom[1]) / 255.0f;
    gEnvironment->skyColourBottom[2] = ((float) header->skyBottom[2]) / 255.0f;
    gEnvironment->skyColourBottom[3] = 1.0f;
    gEnvironment->skyColourTop[0] = ((float) header->skyTop[0]) / 255.0f;
    gEnvironment->skyColourTop[1] = ((float) header->skyTop[1]) / 255.0f;
    gEnvironment->skyColourTop[2] = ((float) header->skyTop[2]) / 255.0f;
    gEnvironment->skyColourTop[3] = 1.0f;
    gEnvironment->flags = header->flags;
    gEnvironment->fogNear = header->fogNear;
    gEnvironment->fogFar = header->fogFar;
    glFogf(GL_FOG_START, gEnvironment->fogNear);
    glFogf(GL_FOG_END, gEnvironment->fogFar);
    glFogfv(GL_FOG_COLOR, gEnvironment->fogColour);
}

static void clear_scene(void) {
    SceneMesh *curMesh = sCurrentScene->meshList;
    clear_objects();
    if (sRenderSkyBlock) {
        rspq_block_free(sRenderSkyBlock);
        sRenderSkyBlock = NULL;
    }
    gPlayer = NULL;
    if (sCurrentScene->model) {
        while (curMesh) {
            SceneMesh *m = curMesh;
            curMesh = curMesh->next;
            free(m->material);
            rspq_block_free(m->renderBlock);
            free(m);
        }
    }
    if (gCamera) {
        free(gCamera);
    }
    if (gEnvironment) {
        //free(gEnvironment);
    }
    if (sCurrentScene->model) {
        model64_free(sCurrentScene->model);
    }
    if (sCurrentScene->overlay) {
        dlclose(sCurrentScene->overlay);
    }
    free(sCurrentScene);
}

void load_scene(int sceneID) {
    DEBUG_SNAPSHOT_1();
    rspq_wait();
    if (gScreenshotStatus == -1) {
        screenshot_clear();
    }
    if (sCurrentScene) {
        clear_scene();
    }
    sCurrentScene = malloc(sizeof(SceneBlock));
    SceneBlock *s = sCurrentScene;
    s->overlay = dlopen(asset_dir(sSceneTable[sceneID], DFS_OVERLAY), RTLD_LOCAL);
    SceneHeader *header = dlsym(s->overlay, "header");
    s->model = NULL;
    if (header->model) {
        s->model = model64_load(asset_dir(header->model, DFS_MODEL64));
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
                m->material->textureID = sSceneTexIDs[sCurrentScene->sceneID][j];
                m->material->flags = 0;
                m->flags = sSceneMeshFlags[sCurrentScene->sceneID][j];
                m->next = NULL;
                rspq_block_begin();
                glPushMatrix();
                glScalef(5.0f, 5.0f, 5.0f);
                model64_draw_primitive(m->mesh);
                glPopMatrix();
                m->renderBlock = rspq_block_end();

                if (s->meshList == NULL) {
                    s->meshList = m;
                } else {
                    tail->next = m;
                }
                tail = m;
            }
        }
    }
    
    if (header->objectMap) {
        int i = 0;
        ObjectMap *o = &header->objectMap[i];

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

            if (header->objectMap[i + 1].objectID == -1) {
                break;
            } else {
                i++;
                o = &header->objectMap[i];
            }
        }
    }
    setup_fog(header);
    camera_init();
    gSceneUpdate = 0;
#ifdef PUPPYPRINT_DEBUG
    debugf("Scene [%s] loaded in %2.3fs.\n", sSceneTable[sceneID], ((float) TIMER_MICROS(DEBUG_SNAPSHOT_1_END)) / 1000000.0f);
#endif
}