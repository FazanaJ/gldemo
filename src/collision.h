#pragma once

#include "../include/global.h"
#include "object.h"

typedef int16_t u_int16_t __attribute__((aligned(1)));

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

typedef struct CollisionData {
    short pos[3][3];
    short upperY;
    short lowerY;
    short normalY;
} CollisionData;

typedef struct CollisionCell {
    unsigned short triangleCount;
    unsigned short cellCount;
    CollisionData *data;
} CollisionCell;

void object_collide(struct Object *obj);
float collision_floor(float x, float y, float z, float *norm, int w);
void collision_normals(u_int16_t *v0, u_int16_t *v1, u_int16_t *v2, float *normals, int w);
float collision_floor_hitbox(struct Object *obj, float x, float y, float z);