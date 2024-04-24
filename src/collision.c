#include <libdragon.h>

#include "collision.h"
#include "../include/global.h"

#include "object.h"
#include "scene.h"
#include "debug.h"
#include "math_util.h"

typedef int16_t u_int16_t __attribute__((aligned(1)));

static void collision_normals(u_int16_t *v0, u_int16_t *v1, u_int16_t *v2, float *normals, int w) {
    float nx, ny, nz;
    float mag;
    
    float x1, y1, z1;
    float x2, y2, z2;
    float x3, y3, z3;

    x1 = v0[0];
    y1 = v0[1];
    z1 = v0[2];
    x2 = v1[0];
    y2 = v1[1];
    z2 = v1[2];
    x3 = v2[0];
    y3 = v2[1];
    z3 = v2[2];

    nx = (y2 - y1) * (z3 - z2) - (z2 - z1) * (y3 - y2);
    ny = (z2 - z1) * (x3 - x2) - (x2 - x1) * (z3 - z2);
    nz = (x2 - x1) * (y3 - y2) - (y2 - y1) * (x3 - x2);
    mag = nx * nx + ny * ny + nz * nz;
    if (fabsf(mag) < 0.0001f) {
        normals[0] = 0.0f;
        normals[1] = 1.0f;
        normals[2] = 0.0f;
        if (w) {
            normals[3] = 0.0f;
        }
        return;
    }
    mag = 1.0f / sqrtf(mag);
    nx *= mag;
    ny *= mag;
    nz *= mag;

    normals[0] = nx;
    normals[1] = ny;
    normals[2] = nz;
    if (w) {
        normals[3] = -(nx * x1 + ny * y1 + nz * z1);
    }
}

static float collision_surface_down(float *posF, int16_t *pos, u_int16_t *v0, u_int16_t *v1, u_int16_t *v2, float *normY) {
    int posCheck = pos[1] + 15;
    if (posCheck < v0[1] && posCheck < v1[1] && posCheck < v2[1]) {
        return -30000;
    }

    if ((v0[2] - pos[2]) * (v1[0] - v0[0]) - (v0[0] - pos[0]) * (v1[2] - v0[2]) < 0) {
        return -30000;
    }
    if ((v1[2] - pos[2]) * (v2[0] - v1[0]) - (v1[0] - pos[0]) * (v2[2] - v1[2]) < 0) {
        return -30000;
    }
    if ((v2[2] - pos[2]) * (v0[0] - v2[0]) - (v2[0] - pos[0]) * (v0[2] - v2[2]) < 0) {
        return -30000;
    }

    float normals[4];

    collision_normals(v0, v1, v2, normals, true);

    if (fabsf(normals[1]) <= 0.0001f) {
        return v0[1];
    }

    float y = -(posF[0] * normals[0] + normals[2] * posF[2] + normals[3]) / normals[1];

    if (posF[1] - (y -78.0f) < 0.0f) {
        return -30000;
    } else {
        *normY = normals[1];
        return y;
    }
}

static void collision_surface_side(float *posF, int16_t *pos, u_int16_t *v0, u_int16_t *v1, u_int16_t *v2, float size, Object *obj, float mulFactorF) {
    int posCheck = pos[1] + 15;
    if (posCheck < v0[1] && posCheck < v1[1] && posCheck < v2[1]) {
        return;
    }

    float normal[4];
    collision_normals(v0, v1, v2, normal, true);

    float offset = normal[0] * posF[0] + normal[1] * posF[1] + normal[2] * posF[2] + normal[3];


    if (fabsf(offset) > size) {
        return;
    }

    int px = pos[0];
    int pz = pos[2];

    float norm;
    float ppz;
    int w1;
    int w2;
    int w3;
    if (fabsf(normal[0]) > 0.707f) {
        w1 = -v0[2];
        w2 = -v1[2];
        w3 = -v2[2];
        norm = normal[0];
        ppz = -pz;
    } else {
        w1 = v0[0];
        w2 = v1[0];
        w3 = v2[0];
        norm = normal[2];
        ppz = px;
    }

    if (norm > 0.0f) {
        if ((v0[1] - pos[1]) * (w2 - w1) - (w1 - ppz) * (v1[1] - v0[1]) > 0.0f) {
            return;
        }
        if ((v1[1] - pos[1]) * (w3 - w2) - (w2 - ppz) * (v2[1] - v1[1]) > 0.0f) {
            return;
        }
        if ((v2[1] - pos[1]) * (w1 - w3) - (w3 - ppz) * (v0[1] - v2[1]) > 0.0f) {
            return;
        }
    } else {
        if ((v0[1] - pos[1]) * (w2 - w1) - (w1 - ppz) * (v1[1] - v0[1]) < 0.0f) {
            return;
        }
        if ((v1[1] - pos[1]) * (w3 - w2) - (w2 - ppz) * (v2[1] - v1[1]) < 0.0f) {
            return;
        }
        if ((v2[1] - pos[1]) * (w1 - w3) - (w3 - ppz) * (v0[1] - v2[1]) < 0.0f) {
            return;
        }
    }

    float of = ((size - offset) / mulFactorF) * 5.0f;

    obj->pos[0] += normal[0] * of;
    obj->pos[2] += normal[2] * of;
}

#if OPENGL
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

#endif

float collision_floor(float x, float y, float z, float *norm, int w) {
#if OPENGL
    DEBUG_SNAPSHOT_1();
    SceneChunk *chunk = gCurrentScene->chunkList;
    float peakY = -30000.0f;
    float scale = 1.0f;
    float normY = 1.0f;

    while (chunk) {
        SceneMesh *mesh = chunk->meshList;
        if (x < chunk->bounds[0][0] || x > chunk->bounds[1][0] || 
            y < chunk->bounds[0][1] || y - 10.0f > chunk->bounds[1][1] || 
            z < chunk->bounds[0][2] || z > chunk->bounds[1][2]) {
            chunk = chunk->next;
            continue;
        }
        while (mesh) {
            ModelPrim *prim = (ModelPrim *) mesh->mesh;
            int numTris = prim->num_indices;
            attribute_t *attr = &prim->position;
            float mulFactorF = (int) (1 << (prim->vertex_precision));
            float plF[3] = {((x) * mulFactorF) / 5, (((y) + 5) * mulFactorF) / 5, ((z) * mulFactorF) / 5};
            int16_t pl[3] = {plF[0], plF[1], plF[2]};

            for (int i = 0; i < numTris; i += 3) {
                unsigned short *indices = (unsigned short *) prim->indices;
                u_int16_t *v1 = (u_int16_t *) (attr->pointer + attr->stride * indices[i + 0]);
                u_int16_t *v2 = (u_int16_t *) (attr->pointer + attr->stride * indices[i + 1]);
                u_int16_t *v3 = (u_int16_t *) (attr->pointer + attr->stride * indices[i + 2]);
                float normals[3];
                collision_normals(v1, v2, v3, normals, false);
                if (fabsf(normals[1] < 0.3f)) {
                } else {
                    float h = collision_surface_down(plF, pl, v1, v2, v3, &normY);
                    if (h > peakY) {
                        peakY = h;
                        scale = mulFactorF;
                    }
                }

            }
            mesh = mesh->next;
        }
        chunk = chunk->next;
    }
    float recordHeight = (peakY / scale) * 5;
    get_time_snapshot(PP_COLLISION, DEBUG_SNAPSHOT_1_END);
    return recordHeight;
#else
    return 0.0f;
#endif
}

void object_collide(Object *obj) {
#if OPENGL
    DEBUG_SNAPSHOT_1();
    SceneChunk *chunk = gCurrentScene->chunkList;
    float peakY = -30000.0f;
    float scale = 1.0f;
    float recordNorm = 1.0f;
    float normY = 1.0f;

    while (chunk) {
        SceneMesh *mesh = chunk->meshList;
        if (obj->pos[0] < chunk->bounds[0][0] || obj->pos[0] > chunk->bounds[1][0] || 
            obj->pos[1] < chunk->bounds[0][1] || obj->pos[1] - 10.0f > chunk->bounds[1][1] || 
            obj->pos[2] < chunk->bounds[0][2] || obj->pos[2] > chunk->bounds[1][2]) {
            chunk = chunk->next;
            continue;
        }
        while (mesh) {
            ModelPrim *prim = (ModelPrim *) mesh->mesh;
            int numTris = prim->num_indices;
            attribute_t *attr = &prim->position;
            float mulFactorF = (int) (1 << (prim->vertex_precision));
            float plF[3] = {((obj->pos[0]) * mulFactorF) / 5, (((obj->pos[1]) + 15 - (10 * obj->collision->floorNorm)) * mulFactorF) / 5, ((obj->pos[2]) * mulFactorF) / 5};
            int16_t pl[3] = {plF[0], plF[1], plF[2]};

            for (int i = 0; i < numTris; i += 3) {
                unsigned short *indices = (unsigned short *) prim->indices;
                u_int16_t *v1 = (u_int16_t *) (attr->pointer + attr->stride * indices[i + 0]);
                u_int16_t *v2 = (u_int16_t *) (attr->pointer + attr->stride * indices[i + 1]);
                u_int16_t *v3 = (u_int16_t *) (attr->pointer + attr->stride * indices[i + 2]);
                float normals[3];
                collision_normals(v1, v2, v3, normals, false);
                if (fabsf(normals[1] < 0.3f)) {
                    collision_surface_side(plF, pl, v1, v2, v3, (4.0f * mulFactorF) / 5.0f, obj, mulFactorF);
                } else {
                    float h = collision_surface_down(plF, pl, v1, v2, v3, &normY);
                    if (h > peakY) {
                        peakY = h;
                        scale = mulFactorF;
                        recordNorm = normY;
                    }
                }

            }
            mesh = mesh->next;
        }
        chunk = chunk->next;
    }
    obj->collision->floorHeight = (peakY / scale) * 5;
    obj->collision->floorNorm = recordNorm;
    get_time_snapshot(PP_COLLISION, DEBUG_SNAPSHOT_1_END);
    #endif
}
