#include <libdragon.h>

#include "collision.h"
#include "../include/global.h"

#include "object.h"
#include "scene.h"
#include "debug.h"
#include "math_util.h"

typedef struct CollisionInfo {
    float incline;
    float normal[3];
    int flags;
} CollisionInfo;

static void vec3f_cross(float dest[3], float a[3], float b[3]) {
    dest[0] = a[1] * b[2] - b[1] * a[2];
    dest[1] = a[2] * b[0] - b[2] * a[0];
    dest[2] = a[0] * b[1] - b[0] * a[1];
}

static void vec3f_normalize(float dest[3]) {
    float size = (dest[0] * dest[0] + dest[1] * dest[1] + dest[2] * dest[2]);
    float invsqrt;
    if (size > 0.01f) {

        invsqrt = sqrtf(size);

        dest[0] *= invsqrt;
        dest[1] *= invsqrt;
        dest[2] *= invsqrt;
    } else {
        dest[0] = 0.0f;
        ((unsigned int *)dest)[1] = 0x3F800000;
        dest[2] = 0.0f;
    }
}

/**************************************************
 *                    RAYCASTING                  *
 **************************************************/

 /// Multiply vector 'dest' by a
static void vec3f_mul(float dest[3], float a) {
    dest[0] *= a;
    dest[1] *= a;
    dest[2] *= a;
}

static void vec3f_sum(float dest[3], float a[3], float b[3]) {
    dest[0] = a[0] + b[0];
    dest[1] = a[1] + b[1];
    dest[2] = a[2] + b[2];
}

/// Make 'dest' the difference of vectors a and b.
static void vec3f_diff(float dest[3], float a[3], float b[3]) {
    dest[0] = a[0] - b[0];
    dest[1] = a[1] - b[1];
    dest[2] = a[2] - b[2];
}

static float vec3f_dot(float a[3], float b[3]) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static int ray_surface_intersect(float orig[3], float dir[3], float dir_length, float *hit_pos, float *length, float v0[3], float v1[3], float v2[3]) {
    float e1[3];
    vec3f_diff(e1, v1, v0);
    float e2[3];
    vec3f_diff(e2, v2, v0);
    float h[3];
    vec3f_cross(h, dir, e2);
    float det = vec3f_dot(e1, h);
    if (det > -0.0001f && det < 0.0001f) return false;
    float f = 1.0f / det; // invDet
    float s[3];
    vec3f_diff(s, orig, v0);
    float u = f * vec3f_dot(s, h);
    if (u < 0.0f || u > 1.0f) return false;
    float q[3];
    vec3f_cross(q, s, e1);
    float v = f * vec3f_dot(dir, q);
    if (v < 0.0f || (u + v) > 1.0f) return false;
    *length = f * vec3f_dot(e2, q);
    if (*length <= 0.01f || *length > dir_length) return false;
    float add_dir[3];
    add_dir[0] = dir[0];
    add_dir[1] = dir[1];
    add_dir[2] = dir[2];
    vec3f_mul(add_dir, *length);
    vec3f_sum(hit_pos, orig, add_dir);
    return true;
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
    void *indices;                  ///< Pointer to the first index value. If NULL, indices are not used
} ModelPrim;

void object_collide(Object *obj) {
    DEBUG_SNAPSHOT_1();

    SceneMesh *mesh = sCurrentScene->meshList;
    float peakY = 0;
    while (mesh) {
        ModelPrim *prim = (ModelPrim *) mesh->mesh;
        int numTris = prim->num_indices;
        attribute_t *attr = &prim->position;
        //attribute_t *col = &prim->color;
        int mulFactor = prim->vertex_precision - 1;
        obj->floorHeight = 0.0f;
        //float scale = (int) (1 << (prim->vertex_precision - 1)));
        float dir[3];
        float tempP[3] = {obj->pos[0] / 5, (obj->pos[1] / 5) + 25.0f, obj->pos[2] / 5};
        dir[0] = tempP[0];// + ((obj->forwardVel * sins(obj->moveAngle[2])) / 20.0f);
        dir[1] = tempP[1] - (50.0f);// - ((obj->forwardVel * coss(obj->moveAngle[2])) / 20.0f);
        dir[2] = tempP[2];
        vec3f_normalize(dir);
        float dirLen = sqrtf(SQR(dir[0]) + SQR(dir[1]) + SQR(dir[2]));

        typedef int16_t u_int16_t __attribute__((aligned(1)));
        //typedef int32_t u_int32_t __attribute__((aligned(1)));

        for (int i = 0; i < numTris; i += 3) {
            uint16_t *indices = (uint16_t *) prim->indices;
            u_int16_t *v1 = (u_int16_t *) (attr->pointer + attr->stride * indices[i + 0]);
            u_int16_t *v2 = (u_int16_t *) (attr->pointer + attr->stride * indices[i + 1]);
            u_int16_t *v3 = (u_int16_t *) (attr->pointer + attr->stride * indices[i + 2]);
            //u_int32_t *c1 = (u_int32_t *) (col->pointer + col->stride * indices[i + 0]);
            //u_int32_t *c2 = (u_int32_t *) (col->pointer + col->stride * indices[i + 1]);
            //u_int32_t *c3 = (u_int32_t *) (col->pointer + col->stride * indices[i + 2]);
            float hit[3] = {0, 0, 0};
            
            float length;
            float vert0[3] = {v1[0] >> mulFactor, v1[1] >> mulFactor, v1[2] >> mulFactor};
            float vert1[3] = {v2[0] >> mulFactor, v2[1] >> mulFactor, v2[2] >> mulFactor};
            float vert2[3] = {v3[0] >> mulFactor, v3[1] >> mulFactor, v3[2] >> mulFactor};
            
            //*c1 = 0xFFFFFFFF;
            //*c2 = 0xFFFFFFFF;
            //*c3 = 0xFFFFFFFF;
                //debugf("Obj: X: %2.2f, Y: %2.2f, Z: %2.2f\n", tempP[0], tempP[1], tempP[2]);
                //debugf("Tri %d: (X1: %2.2f, Y1: %2.2f, Z1: %2.2f), (X2: %2.2f, Y2: %2.2f, Z2: %2.2f), (X3: %2.2f, Y3: %2.2f, Z3: %2.2f)\n", i / 3, vert0[0], vert0[1], vert0[2], vert1[0], vert1[1], vert1[2], vert2[0], vert2[1], vert2[2]);
            int surf = ray_surface_intersect(tempP, dir, dirLen, hit, &length, vert0, vert1, vert2);
            if (surf) {
                //debugf("Hit %d: %2.2f, %2.2f, %2.2f\n", i / 3, hit[0], hit[1], hit[2]);
                //*c1 = 0xFF0000FF;
                //*c2 = 0xFF0000FF;
                //*c3 = 0xFF0000FF;
                //obj->pos[0] = hit[0];
                //obj->pos[1] = hit[1];
                if ((hit[1] / 2) * 5 > peakY) {
                    peakY = (hit[1] / 2) * 5;
                }
                obj->yVel = 0.0f;
                //obj->floorHeight = (hit[2] * 5);
            }
        }
        mesh = mesh->next;
    }
    obj->pos[1] = peakY;

    get_time_snapshot(PP_COLLISION, DEBUG_SNAPSHOT_1_END);
}
