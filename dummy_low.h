#ifndef DUMMY_H
#define DUMMY_H


#include "vertex.h"

const mesh_t dummy_mesh = {
    
    vertices: { 
        {.position = { 0.5, 0.5, 2},    .texcoord = { 1.024f, 0.992f},    .normal = { 0.f,  0.f,  1.f}, .color = 0xFF0000FF }, /* 0 */
        {.position = { -0.5, 0.5, 2},    .texcoord = { 0.f, 0.992f},    .normal = { 0.f,  0.f,  1.f}, .color = 0xFF0000FF }, /* 1 */
        {.position = { -0.5, -0.5, 2},    .texcoord = { 0.f, 0.512f},    .normal = { 0.f,  0.f,  1.f}, .color = 0xFF0000FF }, /* 2 */
        {.position = { 0.5, -0.5, 2},    .texcoord = { 1.024f, 0.512f},    .normal = { 0.f,  0.f,  1.f}, .color = 0xFF0000FF }, /* 3 */

        {.position = { 0.5, -0.5, 0},    .texcoord = { 1.024f, 1.112f},    .normal = {-1.f,  0.f,  0.f}, .color = 0x00FFFFFF }, /* 4 */
        {.position = { 0.5, -0.5, 2},    .texcoord = { 1.024f, 0.064f},    .normal = {-1.f,  0.f,  0.f}, .color = 0x00FFFFFF }, /* 5 */
        {.position = { -0.5, -0.5, 2},    .texcoord = { 0.f, 0.064f},    .normal = {-1.f,  0.f,  0.f}, .color = 0x00FFFFFF }, /* 6 */
        {.position = { -0.5, -0.5, 0},    .texcoord = { 0.f, 1.112f},    .normal = {-1.f,  0.f,  0.f}, .color = 0x00FFFFFF }, /* 7 */

        {.position = { -0.5, -0.5, 0},    .texcoord = { 1.024f, 0.676f},    .normal = { 0.f,  1.f,  0.f}, .color = 0x00FF00FF }, /* 8 */
        {.position = { -0.5, -0.5, 2},    .texcoord = { 1.024f, 1.056f},    .normal = { 0.f,  1.f,  0.f}, .color = 0x00FF00FF }, /* 9 */
        {.position = { -0.5, 0.5, 2},    .texcoord = { 0.f, 1.056f},    .normal = { 0.f,  1.f,  0.f}, .color = 0x00FF00FF }, /* 10 */
        {.position = { -0.5, 0.5, 0},    .texcoord = { 0.f, 0.676f},    .normal = { 0.f,  1.f,  0.f}, .color = 0x00FF00FF }, /* 11 */

        {.position = { -0.5, 0.5, 0},    .texcoord = { 0.960f, 0.576f},    .normal = { 0.f, 0.f,  -1.f}, .color = 0xFF00FFFF }, /* 12 */
        {.position = { 0.5, 0.5, 0},    .texcoord = { 0.960f, 0.896f},    .normal = { 0.f, 0.f,  -1.f}, .color = 0xFF00FFFF }, /* 13 */
        {.position = { 0.5, -0.5, 0},    .texcoord = { 0.064f, 0.896f},    .normal = { 0.f, 0.f,  -1.f}, .color = 0xFF00FFFF }, /* 14 */
        {.position = { -0.5, -0.5, 0},    .texcoord = { 0.064f, 0.576f},    .normal = { 0.f, 0.f,  -1.f}, .color = 0xFF00FFFF }, /* 15 */

        {.position = { 0.5, 0.5, 0},    .texcoord = { 1.024f, 0.992f},    .normal = { 0.f,  0.f,  1.f}, .color = 0x0000FFFF }, /* 16 */
        {.position = { 0.5, 0.5, 2},    .texcoord = { 1.024f, 0.928f},    .normal = { 0.f,  0.f,  1.f}, .color = 0x0000FFFF }, /* 17 */
        {.position = { 0.5, -0.5, 2},    .texcoord = { 0.f, 0.928f},    .normal = { 0.f,  0.f,  1.f}, .color = 0x0000FFFF }, /* 18 */
        {.position = { 0.5, -0.5, 0},    .texcoord = { 0.f, 0.992f},    .normal = { 0.f,  0.f,  1.f}, .color = 0x0000FFFF }, /* 19 */

        {.position = { -0.5, 0.5, 0},    .texcoord = { 1.024f, 0.648f},    .normal = { 0.f,  0.f, -1.f}, .color = 0xFFFF00FF }, /* 20 */
        {.position = { -0.5, 0.5, 2},    .texcoord = { 1.024f, 0.928f},    .normal = { 0.f,  0.f, -1.f}, .color = 0xFFFF00FF }, /* 21 */
        {.position = { 0.5, 0.5, 2},    .texcoord = { 0.f, 0.928f},    .normal = { 0.f,  0.f, -1.f}, .color = 0xFFFF00FF }, /* 22 */
        {.position = { 0.5, 0.5, 0},    .texcoord = { 0.f, 0.648f},    .normal = { 0.f,  0.f, -1.f}, .color = 0xFFFF00FF }, /* 23 */
    },

   indices: {
        0,  1,  2,  0,  2,  3,
        4,  5,  6,  4,  6,  7,
        8,  9, 10,  8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23,
    },

};

#endif
