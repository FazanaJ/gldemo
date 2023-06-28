#ifndef VERTEX
#define VERTEX

#include <stdint.h>

typedef struct {
    float position[3];
    float texcoord[2];
    float normal[3];
    uint32_t color;
} vertex_t;

typedef struct {
    const vertex_t vertices[];
    const uint16_t indices[];
} mesh_t;


#endif
