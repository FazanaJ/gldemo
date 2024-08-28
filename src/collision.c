#include <libdragon.h>

#include "collision.h"
#include "../include/global.h"

#include "object.h"
#include "scene.h"
#include "debug.h"
#include "math_util.h"

void collision_normals(int16_t *v0, int16_t *v1, int16_t *v2, float *normals, int w) {
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

static float collision_surface_down(int *pos, int16_t *v0, int16_t *v1, int16_t *v2, float *normY) {
    int posCheck = pos[1] + 120;
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

    float y = -(pos[0] * normals[0] + normals[2] * pos[2] + normals[3]) / normals[1];

    if (pos[1] - (y -78.0f) < 0.0f) {
        return -30000;
    } else {
        *normY = normals[1];
        return y;
    }
    return -30000;
}

tstatic void collision_surface_side(int *pos, int16_t *v0, int16_t *v1, int16_t *v2, int size, Object *obj) {
    int posCheck = pos[1];
    if ((posCheck < v0[1] && posCheck < v1[1] && posCheck < v2[1]) || (posCheck > v0[1] && posCheck > v1[1] && posCheck > v2[1])) {
        return;
    }

    float normal[4];
    collision_normals(v0, v1, v2, normal, true);

    float offset = (normal[0] * ((float) pos[0])) + (normal[1] * ((float) pos[1])) + (normal[2] * ((float) pos[2])) + normal[3];

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

    float of = ((size - offset) * WORLD_SCALE);

    obj->pos[0] += normal[0] * of;
    obj->pos[2] += normal[2] * of;
}

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

float collision_floor_hitbox(Object *obj, float x, float y, float z) {
    DEBUG_SNAPSHOT_1();
    float recordHeight = -30000.0f;
    for (int i = 0; i < obj->hitbox->numNear; i++) {
        Object *hit = obj->hitbox->nearObj[i];

    }
    get_time_snapshot(PP_COLLISION, DEBUG_SNAPSHOT_1_END);
    return recordHeight;
}

void object_collide(Object *obj) {
    DEBUG_SNAPSHOT_1();
    SceneChunk *chunk = gCurrentScene->chunkList;
    float peakY = -30000.0f;
    float recordNorm = 1.0f;
    float normY = 1.0f;
    int recordFlags = 0;
    int pos[3];

    pos[0] = obj->pos[0] / WORLD_SCALE;
    pos[1] = obj->pos[1] / WORLD_SCALE;
    pos[2] = obj->pos[2] / WORLD_SCALE;

    while (chunk) {
        SceneMesh *mesh = chunk->meshList;
        if (obj->pos[0] < chunk->bounds[0][0] || obj->pos[0] > chunk->bounds[1][0] || 
            obj->pos[1] < chunk->bounds[0][1] || obj->pos[1] - 80.0f > chunk->bounds[1][1] || 
            obj->pos[2] < chunk->bounds[0][2] || obj->pos[2] > chunk->bounds[1][2] || !(chunk->flags & CHUNK_HAS_MODEL)) {
            chunk = chunk->next;
            continue;
        }
        /*if (chunk->collision == NULL) {
            scene_generate_collision(chunk);
            if (chunk->collision == NULL) {
                chunk = chunk->next;
            }
        }
        chunk->collisionTimer = 30;*/
        while (mesh) {
            if (mesh->material && mesh->material->collisionFlags & COLFLAG_INTANGIBLE) {
                mesh = mesh->next;
                continue;
            }
            T3DObject *t = (T3DObject *) mesh->mesh;
            for (int i = 0; i < t->numParts; i++) {
                const T3DObjectPart *part = &t->parts[i];
                for (int j = 0; j < part->numIndices; j += 3) {
                    int16_t vertex[3][3];
                    for (int l = 0; l < 3; l++) {
                        int ind = part->indices[j + l];
                        for (int k = 0; k < 3; k++) {
                            if ((ind % 2) == 0) {
                                vertex[l][k] = part->vert[ind / 2].posA[k];
                            } else {
                                vertex[l][k] = part->vert[ind / 2].posB[k];
                            }
                        }
                    }
                    float normals[3];
                    collision_normals(vertex[0], vertex[1], vertex[2], normals, false);
                    if (fabsf(normals[1] < 0.3f)) {
                        collision_surface_side(pos, vertex[0], vertex[1], vertex[2], 4 * 8, obj);
                    } else {
                        float h = collision_surface_down(pos, vertex[0], vertex[1], vertex[2], &normY);
                        if (h > peakY) {
                            peakY = h;
                            recordNorm = normY;
                            recordFlags = mesh->material->collisionFlags;
                        }
                    }
                    //debugf("X0: %d, Y0: %d, Z0: %d, X1: %d, Y1: %d, Z1: %d, X2: %d, Y2: %d, Z2: %d\n",
                    //vertex[0][0], vertex[0][1], vertex[0][2], vertex[1][0], vertex[1][1], vertex[1][2], vertex[2][0], vertex[2][1], vertex[2][2]);
                }
            }
            mesh = mesh->next;
        }
        chunk = chunk->next;
    }
    obj->collision->floorHeight = (peakY * WORLD_SCALE);
    obj->collision->floorNorm = recordNorm;
    obj->collision->floorFlags = recordFlags;
    get_time_snapshot(PP_COLLISION, DEBUG_SNAPSHOT_1_END);
}
