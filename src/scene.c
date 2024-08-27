#include <libdragon.h>
#include <malloc.h>

#include "scene.h"
#include "../include/global.h"

#include "object.h"
#include "camera.h"
#include "render.h"
#include "debug.h"
#include "hud.h"
#include "screenshot.h"
#include "math_util.h"
#include "main.h"

SceneBlock *gCurrentScene;
Environment *gEnvironment;
char gSceneUpdate;

char *sSceneTable[SCENE_TOTAL] = {
    "intro",
    "testarea",
    "testarea2",
    "testarea3",
    "testarea4"
};

char sSceneTexIDs[SCENE_TOTAL][20] = {
    {MATERIAL_INTROSIGN2, MATERIAL_KITCHENTILE, MATERIAL_INTROSIGN},
    {MATERIAL_WATER, MATERIAL_GRASS0, MATERIAL_STONE, MATERIAL_WATER},
    {MATERIAL_WATER, MATERIAL_RAILING, MATERIAL_KITCHENTILE, MATERIAL_HEALTH, MATERIAL_INTROSIGN, MATERIAL_LOGWALL, MATERIAL_FLATPRIM_XLU},
    {MATERIAL_GRASS0, MATERIAL_KITCHENTILE, MATERIAL_LOGWALL, MATERIAL_LOGWALL, MATERIAL_STONE},
    {MATERIAL_DUSTMAT0, MATERIAL_DUSTMAT1, MATERIAL_DUSTMAT2, MATERIAL_DUSTMAT5, MATERIAL_DUSTMAT6, MATERIAL_DUSTMAT7, MATERIAL_DUSTMAT8, MATERIAL_DUSTMAT9, MATERIAL_DUSTMAT11, MATERIAL_DUSTMAT12, MATERIAL_DUSTMAT16, MATERIAL_DUSTMAT17, 0, 0, 0, 0, 0, MATERIAL_DUSTMAT13, MATERIAL_DUSTMAT18, MATERIAL_DUSTMAT24}
};

tstatic void setup_fog(SceneHeader *header) {
    if (gEnvironment == NULL) {
        gEnvironment = malloc(sizeof(Environment));
    }
    bzero(gEnvironment, sizeof(Environment));
    gEnvironment->fogColour[0] = header->fogColour[0];
    gEnvironment->fogColour[1] = header->fogColour[1];
    gEnvironment->fogColour[2] = header->fogColour[2];
    gEnvironment->skyColourBottom[0] = header->skyBottom[0];
    gEnvironment->skyColourBottom[1] = header->skyBottom[1];
    gEnvironment->skyColourBottom[2] = header->skyBottom[2];
    gEnvironment->skyColourTop[0] = header->skyTop[0];
    gEnvironment->skyColourTop[1] = header->skyTop[1];
    gEnvironment->skyColourTop[2] = header->skyTop[2];
    gEnvironment->flags = header->flags;
    gEnvironment->fogNear = header->fogNear;
    gEnvironment->fogFar = header->fogFar;
    gEnvironment->skyboxTextureID = header->skyTexture;
    rdpq_mode_fog(RDPQ_FOG_STANDARD);
    rdpq_set_fog_color((color_t){gEnvironment->fogColour[0], gEnvironment->fogColour[1], gEnvironment->fogColour[2], 0xFF});
    t3d_fog_set_range(gEnvironment->fogNear, gEnvironment->fogFar);
}

tstatic void clear_scene(void) {
    SceneChunk *curChunk = gCurrentScene->chunkList;
    clear_objects();
    if (sRenderSkyBlock) {
        rspq_block_free(sRenderSkyBlock);
        sRenderSkyBlock = NULL;
    }
    gPlayer = NULL;
    if (gCurrentScene->model) {
        while (curChunk) {
            if (curChunk->collision) {
                free(curChunk->collision);
                free(curChunk->collisionData);
            }
            SceneMesh *curMesh = curChunk->meshList;
            while (curMesh) {
                SceneMesh *m = curMesh;
                curMesh = curMesh->next;
                if (m->material) {
                    material_try_free(m->material->entry);
                }
                if (m->renderBlock) {
                    rspq_block_free(m->renderBlock);
                }
                free(m);
            }
            SceneChunk *c = curChunk;
            curChunk = curChunk->next;
            free(c);
        }
    }
    if (gCamera) {
        free(gCamera);
    }
    if (gEnvironment) {
        if (gEnvironment->texGen) {
            sky_free(gEnvironment);
        }
        //free(gEnvironment);
    }
    if (gCurrentScene->model) {
        t3d_model_free(gCurrentScene->model);
    }
    if (gCurrentScene->overlay) {
        dlclose(gCurrentScene->overlay);
    }
    free(gCurrentScene);
}

void scene_clear_chunk(SceneChunk *c) {
    SceneMesh *m = c->meshList;
    while (m) {
        if (m->material) {
            material_try_free(m->material->entry);
            m->material = NULL;
        }
        if (m->renderBlock) {
            rspq_block_free(m->renderBlock);
            m->renderBlock = NULL;
        }
        m = m->next;
    }
    c->flags &= ~CHUNK_HAS_MODEL;
}

void scene_generate_collision(SceneChunk *c) {
    int triCount = c->collisionTriCount;
    if (triCount == 0) {
        return;
    }
    int cellCount = 1;
    if (triCount > 400) {
        cellCount *= 4;
    }
#if OPENGL
    int triCount = c->collisionTriCount;
    if (triCount == 0) {
        return;
    }
    int cellCount = 1;
    triCount -= 400;
    while (triCount > 0) {
        triCount -= 400;
        cellCount *= 4;
    }
    unsigned int allocationSize = sizeof(CollisionCell);
    c->collision = malloc(allocationSize);
    c->collision->cellCount = cellCount;
    c->collision->data = malloc((sizeof(CollisionData *) * cellCount) + (sizeof(CollisionData) * c->collisionTriCount));
    float offsetX = c->bounds[0][0];
    float offsetZ = c->bounds[0][2];
    for (int i = 0; i < cellCount; i++) {
        //(*c->collision->data)[i] = (void *) ((unsigned int )(*c->collision->data)[0] + (sizeof(CollisionData *) * i));
    }
    SceneMesh *m = c->meshList;

    int triOffset = 0;

    while (m) {
        if (m->material->collisionFlags & COLFLAG_INTANGIBLE) {
            m = m->next;
            continue;
        }
        ModelPrim *prim = (ModelPrim *) m->mesh;
        attribute_t *attr = &prim->position;
        int numTris = prim->num_indices;
        for (int i = 0; i < numTris; i += 3) {
            unsigned short *indices = (unsigned short *) prim->indices;
            float mulFactorF = (int) (1 << (prim->vertex_precision));
            u_int16_t *v1 = (u_int16_t *) (attr->pointer + attr->stride * indices[i + 0]);
            u_int16_t *v2 = (u_int16_t *) (attr->pointer + attr->stride * indices[i + 1]);
            u_int16_t *v3 = (u_int16_t *) (attr->pointer + attr->stride * indices[i + 2]);
            
            c->collision->data[triOffset].pos[0][0] = v1[0] - offsetX;
            c->collision->data[triOffset].pos[0][1] = v1[1];
            c->collision->data[triOffset].pos[0][2] = v1[2] - offsetZ;
            c->collision->data[triOffset].pos[1][0] = v2[0] - offsetX;
            c->collision->data[triOffset].pos[1][1] = v2[1];
            c->collision->data[triOffset].pos[1][2] = v2[2] - offsetZ;
            c->collision->data[triOffset].pos[2][0] = v3[0] - offsetX;
            c->collision->data[triOffset].pos[2][1] = v3[1];
            c->collision->data[triOffset].pos[2][2] = v3[2] - offsetZ;
            int upperY = MAX(MAX(v1[1], v2[1]), v3[1]);
            c->collision->data[triOffset].upperY = upperY;
            int lowerY = MIN(MIN(v1[1], v2[1]), v3[1]);
            c->collision->data[triOffset].upperY = lowerY;
            float normals[3];
            collision_normals(v1, v2, v3, normals, false);
            c->collision->data[triOffset].normalY = normals[1] * 32767.0f;
            triOffset++;
        }
        m = m->next;
    }
#endif
}

void scene_generate_chunk(SceneMesh *s) {
    s->material = material_init(s->materialID);
    rspq_block_begin();
    //t3d_mat4fp_from_srt_euler(&s->mtx, (float[3]){WORLD_SCALE, WORLD_SCALE, WORLD_SCALE}, (float[3]){0, 0, 0}, (float[3]){0, 0, 0});
    //data_cache_hit_writeback(&s->mtx, sizeof(Matrix));
    //t3d_matrix_push(&s->mtx);
    t3d_model_draw_object((T3DObject *) s->mesh, NULL);
    //t3d_matrix_pop(1);
    s->renderBlock = rspq_block_end();
}

void load_scene(int sceneID, int updateRate, float updateRateF) {
    DEBUG_SNAPSHOT_1();
    deltatime_snapshot(updateRate, updateRateF);
    rspq_wait();
    if (gScreenshotStatus == -1) {
        screenshot_clear();
    }
    if (gCurrentScene) {
        clear_scene();
    }
    gCurrentScene = malloc(sizeof(SceneBlock));
    SceneBlock *s = gCurrentScene;
    s->chunkList = NULL;
    s->overlay = dlopen(asset_dir(sSceneTable[sceneID], DFS_OVERLAY), RTLD_LOCAL);
    SceneHeader *header = dlsym(s->overlay, "header");
    SceneChunk *tailC = NULL;
    int chunkCount = 0;
    s->model = NULL;
    if (header->model) {
        int lowPos[3] = {9999999, 9999999, 9999999};
        int highPos[3] = {-9999999, -9999999, -9999999};
        s->model = t3d_model_load(asset_dir(header->model, DFS_MODEL64));
        s->sceneID = sceneID;
        T3DModel *mesh = (T3DModel *) s->model;
        int primCount = mesh->chunkCount;
        T3DObject *curObj = NULL;
        SceneMesh *tailM = NULL;
        char *usedModels[256];
        int usedNum = 0;
        bzero(usedModels, 256 * 4);
        for (int i = 0; i < primCount; i++) {
            if (mesh->chunkOffsets[i].type == 'O') {
                uint32_t offset = mesh->chunkOffsets[i].offset & 0x00FFFFFF;
                curObj = (T3DObject *) ((char *) mesh + offset);
            } else {
                continue;
            }
            for (int j = 0; j < usedNum; j++) {
                //debugf("%s vs %s\n", curObj->name, usedModels[j]);
                if (curObj->name == usedModels[j]) {
                    //debugf("Model seen before, skipping\n");
                    goto next;
                }
            }
            usedModels[usedNum] = curObj->name;
            usedNum++;
            SceneChunk *c = malloc(sizeof(SceneChunk));
            c->meshList = NULL;
            c->flags = 0;
            c->chunkID = i;
            c->collisionTriCount = 0;
            c->collisionTimer = 0;
            c->collision = NULL;
            chunkCount++;
            c->next = NULL;
            c->bounds[0][0] = 9999999.0f;
            c->bounds[0][1] = 9999999.0f;
            c->bounds[0][2] = 9999999.0f;
            c->bounds[1][0] = -9999999.0f;
            c->bounds[1][1] = -9999999.0f;
            c->bounds[1][2] = -9999999.0f;
            c->visibility = NULL;
            for (int j = 0; j < primCount; j++) {
                if (mesh->chunkOffsets[j].type != 'O') {
                    continue;
                }
                
                uint32_t offset = mesh->chunkOffsets[j].offset & 0x00FFFFFF;
                T3DObject *obj = (T3DObject *) ((char *) mesh + offset);
                if (obj->name != curObj->name) {
                    continue;
                }
                //debugf("%s   %s\n", obj->name, obj->material->name);
                SceneMesh *m = malloc(sizeof(SceneMesh));
                m->mesh = (primitive_t *) obj;
                int sceneTex = j % 8;
                if (sceneID == SCENE_TESTAREA4) {
                    int matID = strtol(obj->material->name, 0, 10);
                    if (matID < 0) {
                        matID = 0;
                    }
                    if (matID > MATERIAL_DUSTMAT33) {
                        matID = MATERIAL_DUSTMAT33;
                    }
                    m->materialID = matID;
                } else {
                    m->materialID = sSceneTexIDs[gCurrentScene->sceneID][sceneTex];
                }
                if ((gMaterialIDs[m->materialID].collisionFlags & COLFLAG_INTANGIBLE) == false) {
                    for (int k = 0; k < obj->numParts; k++) {
                        const T3DObjectPart *part = &obj->parts[k];
                        c->collisionTriCount += part->numIndices;
                    }
                }
                m->material = NULL;
                if (gMaterialIDs[m->materialID].flags & MAT_INVISIBLE) {
                    m->flags = MESH_INVISIBLE;
                } else {
                    m->flags = 0;
                }
                if (m->materialID == MATERIAL_WATER) {
                    m->primC = RGBA32(89, 125, 151, 64);
                } else if (m->materialID == MATERIAL_FLATPRIM_XLU) {
                    m->primC = RGBA32(0, 255, 255, 255);
                } else {
                    m->primC = RGBA32(255, 255, 255, 255);
                }
                for (int k = 0; k < 3; k++) {
                    lowPos[k] = MIN(lowPos[k], obj->aabbMin[k] * WORLD_SCALE);
                    highPos[k] = MAX(highPos[k], obj->aabbMax[k] * WORLD_SCALE);
                    c->bounds[0][k] = MIN(c->bounds[0][k], obj->aabbMin[k] * WORLD_SCALE);
                    c->bounds[1][k] = MAX(c->bounds[1][k], obj->aabbMax[k] * WORLD_SCALE);
                }
                m->next = NULL;
                m->renderBlock = NULL;
                if (c->meshList == NULL) {
                    c->meshList = m;
                } else {
                    tailM->next = m;
                }
                tailM = m;
            }
            if (s->chunkList == NULL) {
                s->chunkList = c;
            } else {
                tailC->next = c;
            }
            tailC = c;
            s->bounds[0][0] = lowPos[0];
            s->bounds[0][1] = lowPos[1];
            s->bounds[0][2] = lowPos[2];
            s->bounds[1][0] = highPos[0];
            s->bounds[1][1] = highPos[1];
            s->bounds[1][2] = highPos[2];
            next:
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
    reset_game_time();
    gSceneUpdate = 0;
#ifdef PUPPYPRINT_DEBUG
    debugf("Scene [%s] loaded in %2.3fs. Chunks: %d\n", sSceneTable[sceneID], (double) (TIMER_MICROS(DEBUG_SNAPSHOT_1_END) / 1000000.0f), chunkCount);
#endif
}