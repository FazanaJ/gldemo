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

SceneBlock *gCurrentScene;
Environment *gEnvironment;
char gSceneUpdate;

char *sSceneTable[SCENE_TOTAL] = {
    "intro",
    "testarea",
    "testarea2",
    "testarea3",
};

char sSceneTexIDs[SCENE_TOTAL][7] = {
    {MATERIAL_INTROSIGN, MATERIAL_KITCHENTILE, MATERIAL_INTROSIGN2},
    {MATERIAL_STONE, MATERIAL_GRASS0, MATERIAL_WATER, MATERIAL_GRASS0_NZB},
    {MATERIAL_HEALTH, MATERIAL_KITCHENTILE, MATERIAL_RAILING, MATERIAL_WATER, MATERIAL_LOGWALL, MATERIAL_INTROSIGN, MATERIAL_FLATPRIM_XLU},
    {MATERIAL_GRASS0, MATERIAL_KITCHENTILE, MATERIAL_LOGWALL, MATERIAL_WATER, MATERIAL_STONE},
};

static void setup_fog(SceneHeader *header) {
    if (gEnvironment == NULL) {
        gEnvironment = malloc(sizeof(Environment));
    }
    bzero(gEnvironment, sizeof(Environment));
    gEnvironment->fogColour[0] = ((float) header->fogColour[0]) / 255.0f;
    gEnvironment->fogColour[1] = ((float) header->fogColour[1]) / 255.0f;
    gEnvironment->fogColour[2] = ((float) header->fogColour[2]) / 255.0f;
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
#if OPENGL
    glFogf(GL_FOG_END, gEnvironment->fogNear);
    glFogf(GL_FOG_START, gEnvironment->fogFar);
    glFogfv(GL_FOG_COLOR, gEnvironment->fogColour);
#elif TINY3D
    rdpq_mode_fog(RDPQ_FOG_STANDARD);
    rdpq_set_fog_color((color_t){gEnvironment->fogColour[0], gEnvironment->fogColour[1], gEnvironment->fogColour[2], 0xFF});
    t3d_fog_set_range(gEnvironment->fogNear, gEnvironment->fogFar);
#endif
}

static void clear_scene(void) {
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
            if (gEnvironment->skyInit) {
                rspq_block_free(gEnvironment->skyInit);
                gEnvironment->skyInit = NULL;
            }
            for (int i = 0; i < 32; i++) {
                if (gEnvironment->skySegment[i]) {
                    rspq_block_free(gEnvironment->skySegment[i]);
                    gEnvironment->skySegment[i] = NULL;
                }
                gNumMaterials--;
            }
            sprite_free(gEnvironment->skySprite);
        }
        //free(gEnvironment);
    }
    if (gCurrentScene->model) {
        MODEL_FREE(gCurrentScene->model);
    }
    if (gCurrentScene->overlay) {
        dlclose(gCurrentScene->overlay);
    }
    free(gCurrentScene);
}

#if OPENGL
static void scene_mesh_boundbox(SceneChunk *c, SceneMesh *m) {
    ModelPrim *prim = (ModelPrim *) m->mesh;
    int numTris = prim->num_indices;
    attribute_t *attr = &prim->position;
    int lowPos[3] = {9999999, 9999999, 9999999};
    int highPos[3] = {-9999999, -9999999, -9999999};
    float mulFactorF = (int) (1 << (prim->vertex_precision));

    for (int i = 0; i < numTris; i += 3) {
        unsigned short *indices = (unsigned short *) prim->indices;
        u_int16_t *v1 = (u_int16_t *) (attr->pointer + attr->stride * indices[i + 0]);
        u_int16_t *v2 = (u_int16_t *) (attr->pointer + attr->stride * indices[i + 1]);
        u_int16_t *v3 = (u_int16_t *) (attr->pointer + attr->stride * indices[i + 2]);

        for (int j = 0; j < 3; j++) {
            if (v1[j] < lowPos[j]) lowPos[j] = v1[j];
            if (v2[j] < lowPos[j]) lowPos[j] = v2[j];
            if (v3[j] < lowPos[j]) lowPos[j] = v3[j];
            if (v1[j] > highPos[j]) highPos[j] = v1[j];
            if (v2[j] > highPos[j]) highPos[j] = v2[j];
            if (v3[j] > highPos[j]) highPos[j] = v3[j];
        }
    }

    for (int i = 0; i < 3; i++) {
        float new = (((float) lowPos[i] / mulFactorF) * 5.0f) - 1.0f;
        if (new < c->bounds[0][i]) {
            c->bounds[0][i] = new;
        }
        new = (((float) highPos[i] / mulFactorF) * 5.0f) + 1.0f;
        if (new > c->bounds[1][i]) {
            c->bounds[1][i] = new;
        }
    }
}
#endif

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

#if OPENGL
void scene_generate_collision(SceneChunk *c) {
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
}
#endif

void scene_generate_chunk(SceneMesh *s) {
    s->material = material_init(s->materialID);
    rspq_block_begin();
    MATRIX_PUSH();
#if OPENGL
    glScalef(5.0f, 5.0f, 5.0f);
    model64_draw_primitive(s->mesh);
#endif
    MATRIX_POP();
    s->renderBlock = rspq_block_end();
}

void load_scene(int sceneID) {
    DEBUG_SNAPSHOT_1();
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
#if OPENGL
    if (header->model) {
        int lowPos[3] = {9999999, 9999999, 9999999};
        int highPos[3] = {-9999999, -9999999, -9999999};
        s->model = MODEL_LOAD(header->model);
        s->sceneID = sceneID;
        int numMeshes = model64_get_mesh_count(s->model);
        SceneMesh *tailM = NULL;
        for (int i = 0; i < numMeshes; i++) {
            SceneChunk *c = malloc(sizeof(SceneChunk));
            mesh_t *mesh = model64_get_mesh(s->model, i);
            int primCount = model64_get_primitive_count(mesh);
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
                SceneMesh *m = malloc(sizeof(SceneMesh));
                m->mesh = model64_get_primitive(mesh, j);
                m->materialID = sSceneTexIDs[gCurrentScene->sceneID][j];
                if ((gMaterialIDs[m->materialID].collisionFlags & COLFLAG_INTANGIBLE) == false) {
                    ModelPrim *prim = (ModelPrim *) m->mesh;
                    c->collisionTriCount += prim->num_indices;
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
                scene_mesh_boundbox(c, m);
                lowPos[0] = MIN(lowPos[0], c->bounds[0][0]);
                lowPos[1] = MIN(lowPos[1], c->bounds[0][1]);
                lowPos[2] = MIN(lowPos[2], c->bounds[0][2]);
                highPos[0] = MAX(highPos[0], c->bounds[1][0]);
                highPos[1] = MAX(highPos[1], c->bounds[1][1]);
                highPos[2] = MAX(highPos[2], c->bounds[1][2]);
                m->next = NULL;
                m->renderBlock = NULL;
                if (c->meshList == NULL) {
                    c->meshList = m;
                } else {
                    tailM->next = m;
                }
                tailM = m;
            }
            //debugf("X1: %2.2f, Y1: %2.2f, Z2: %2.2f, X2: %2.2f, Y2: %2.2f, Z2: %2.2f\n",
            //c->bounds[0][0], c->bounds[0][1], c->bounds[0][2], c->bounds[1][0], c->bounds[1][1], c->bounds[1][2]);
            if (s->chunkList == NULL) {
                s->chunkList = c;
            } else {
                tailC->next = c;
            }
            tailC = c;
        }
        s->bounds[0][0] = lowPos[0];
        s->bounds[0][1] = lowPos[1];
        s->bounds[0][2] = lowPos[2];
        s->bounds[1][0] = highPos[0];
        s->bounds[1][1] = highPos[1];
        s->bounds[1][2] = highPos[2];
    }
#endif
    
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
    debugf("Scene [%s] loaded in %2.3fs. Chunks: %d\n", sSceneTable[sceneID], (double) (TIMER_MICROS(DEBUG_SNAPSHOT_1_END) / 1000000.0f), chunkCount);
#endif
}