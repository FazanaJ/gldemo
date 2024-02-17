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

#define static

SceneBlock *sCurrentScene;
Environment *gEnvironment;
char gSceneUpdate;

char *sSceneTable[SCENE_TOTAL] = {
    "intro",
    "testarea",
    "testarea2",
    "testarea3",
};

char sSceneTexIDs[SCENE_TOTAL][6] = {
    {TEXTURE_INTROSIGN, TEXTURE_KITCHENTILE, TEXTURE_INTEROSIGN2},
    {TEXTURE_STONE, TEXTURE_GRASS0, TEXTURE_WATER},
    {TEXTURE_HEALTH, TEXTURE_KITCHENTILE, TEXTURE_RAILING, TEXTURE_WATER, TEXTURE_WOODWALL, TEXTURE_INTROSIGN},
    {TEXTURE_MOUNTAINSIDEBOTTOM, TEXTURE_KITCHENTILE, TEXTURE_WOODWALL, TEXTURE_WATER, TEXTURE_STONE},
};

int sSceneMeshFlags[SCENE_TOTAL][6] = {
    {MATERIAL_DEPTH_READ, 
    MATERIAL_DEPTH_READ,
    MATERIAL_DEPTH_READ},

    {MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_VTXCOL, 
    MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_VTXCOL, 
    MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_XLU},

    {MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_VTXCOL | MATERIAL_DECAL | MATERIAL_CUTOUT, 
    MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_VTXCOL, 
    MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_VTXCOL | MATERIAL_CUTOUT,
    MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_XLU,
    MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_VTXCOL,
    MATERIAL_DEPTH_READ | MATERIAL_FOG | MATERIAL_VTXCOL, },

    {MATERIAL_DEPTH_READ | MATERIAL_VTXCOL, 
    MATERIAL_DEPTH_READ | MATERIAL_VTXCOL, 
    MATERIAL_DEPTH_READ | MATERIAL_VTXCOL, 
    MATERIAL_DEPTH_READ | MATERIAL_VTXCOL, 
    MATERIAL_DEPTH_READ | MATERIAL_VTXCOL, },
};

static void setup_fog(SceneHeader *header) {
    if (gEnvironment == NULL) {
        gEnvironment = malloc(sizeof(Environment));
    }
    bzero(gEnvironment, sizeof(Environment));
    gEnvironment->fogColour[0] = ((float) header->fogColour[0]) / 255.0f;
    gEnvironment->fogColour[1] = ((float) header->fogColour[1]) / 255.0f;
    gEnvironment->fogColour[2] = ((float) header->fogColour[2]) / 255.0f;
    gEnvironment->skyColourBottom[0] = ((float) header->skyBottom[0]) / 255.0f;
    gEnvironment->skyColourBottom[1] = ((float) header->skyBottom[1]) / 255.0f;
    gEnvironment->skyColourBottom[2] = ((float) header->skyBottom[2]) / 255.0f;
    gEnvironment->skyColourTop[0] = ((float) header->skyTop[0]) / 255.0f;
    gEnvironment->skyColourTop[1] = ((float) header->skyTop[1]) / 255.0f;
    gEnvironment->skyColourTop[2] = ((float) header->skyTop[2]) / 255.0f;
    gEnvironment->flags = header->flags;
    gEnvironment->fogNear = header->fogNear;
    gEnvironment->fogFar = header->fogFar;
    gEnvironment->skyboxTextureID = header->skyTexture;
    glFogf(GL_FOG_START, gEnvironment->fogNear);
    glFogf(GL_FOG_END, gEnvironment->fogFar);
    glFogfv(GL_FOG_COLOR, gEnvironment->fogColour);
}

static void clear_scene(void) {
    SceneChunk *curChunk = sCurrentScene->chunkList;
    clear_objects();
    if (sRenderSkyBlock) {
        rspq_block_free(sRenderSkyBlock);
        sRenderSkyBlock = NULL;
    }
    gPlayer = NULL;
    if (sCurrentScene->model) {
        while (curChunk) {
            SceneMesh *curMesh = curChunk->meshList;
            while (curMesh) {
                SceneMesh *m = curMesh;
                curMesh = curMesh->next;
                free(m->material);
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
            for (int i = 0; i < 32; i++) {
                glDeleteTextures(1, &gEnvironment->textureSegments[i]);
            }
            sprite_free(gEnvironment->skySprite);
        }
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

typedef struct attribute_s {
    uint32_t size;                  ///< Number of components per vertex. If 0, this attribute is not defined
    uint32_t type;                  ///< The data type of each component (for example GL_FLOAT)
    uint32_t stride;                ///< The byte offset between consecutive vertices. If 0, the values are tightly packed
    void *pointer;                  ///< Pointer to the first value
} attribute_t;

/** @brief A single draw call that makes up part of a mesh (part of #mesh_t) */
typedef struct ModelPrim {
    uint32_t mode;                  ///< Primitive assembly mode (for example GL_TRIANGLES)
    attribute_t position;           ///< Vertex position attribute, if defined
    attribute_t color;              ///< Vertex color attribyte, if defined
    attribute_t texcoord;           ///< Texture coordinate attribute, if defined
    attribute_t normal;             ///< Vertex normals, if defined
    attribute_t mtx_index;          ///< Matrix indices (aka bones), if defined
    uint32_t vertex_precision;      ///< If the vertex positions use fixed point values, this defines the precision
    uint32_t texcoord_precision;    ///< If the texture coordinates use fixed point values, this defines the precision
    uint32_t index_type;            ///< Data type of indices (for example GL_UNSIGNED_SHORT)
    uint32_t num_vertices;          ///< Number of vertices
    uint32_t num_indices;           ///< Number of indices
    uint32_t local_texture;         ///< Texture index in this model
    uint32_t shared_texture;        ///< A shared texture index between other models
    void *indices;                  ///< Pointer to the first index value. If NULL, indices are not used
} ModelPrim;

typedef int16_t u_int16_t __attribute__((aligned(1)));

static void scene_mesh_boundbox(SceneChunk *c, SceneMesh *m) {
    ModelPrim *prim = (ModelPrim *) m->mesh;
    int numTris = prim->num_indices;
    attribute_t *attr = &prim->position;
    int lowPos[3] = {9999999, 9999999, 9999999};
    int highPos[3] = {-9999999, -9999999, -9999999};
    float mulFactorF = (int) (1 << (prim->vertex_precision));

    for (int i = 0; i < numTris; i += 3) {
        uint16_t *indices = (uint16_t *) prim->indices;
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

void scene_clear_chunk(SceneChunk *c) {
    SceneMesh *m = c->meshList;
    while (m) {
        if (m->renderBlock) {
            rspq_block_free(m->renderBlock);
            m->renderBlock = NULL;
        }
        m = m->next;
    }
    c->flags &= ~CHUNK_HAS_MODEL;
}

void scene_generate_chunk(SceneMesh *s) {
    rspq_block_begin();
    glPushMatrix();
    glScalef(5.0f, 5.0f, 5.0f);
    model64_draw_primitive(s->mesh);
    glPopMatrix();
    s->renderBlock = rspq_block_end();
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
    s->chunkList = NULL;
    s->overlay = dlopen(asset_dir(sSceneTable[sceneID], DFS_OVERLAY), RTLD_LOCAL);
    SceneHeader *header = dlsym(s->overlay, "header");
    SceneChunk *tailC = NULL;
    s->model = NULL;
    if (header->model) {
        s->model = model64_load(asset_dir(header->model, DFS_MODEL64));
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
                m->material = malloc(sizeof(Material));
                m->material->index = NULL;
                m->material->textureID = sSceneTexIDs[sCurrentScene->sceneID][j];
                m->material->flags = sSceneMeshFlags[sCurrentScene->sceneID][j];
                scene_mesh_boundbox(c, m);
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