#include <libdragon.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>
#include <malloc.h>

#include "render.h"
#include "../include/global.h"

#include "main.h"

Environment *gEnvironment;

void setup_light(light_t light) {

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, light.color);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light.diffuse);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 2.0f/light.radius);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 1.0f/(light.radius*light.radius));

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, light.diffuse);
    gEnvironment = malloc(sizeof(Environment));
    gEnvironment->fogColour[0] = light.color[0];
    gEnvironment->fogColour[1] = light.color[1];
    gEnvironment->fogColour[2] = light.color[2];
    gEnvironment->skyColourBottom[0] = light.color[0] * 0.66f;
    gEnvironment->skyColourBottom[1] = light.color[1] * 0.66f;
    gEnvironment->skyColourBottom[2] = light.color[2] * 0.66f;
    gEnvironment->skyColourTop[0] = light.color[0] * 1.33f;
    if (gEnvironment->skyColourTop[0] > 1.0f) {
        gEnvironment->skyColourTop[0] = 1.0f;
    }
    gEnvironment->skyColourTop[1] = light.color[1] * 1.33f;
    if (gEnvironment->skyColourTop[1] > 1.0f) {
        gEnvironment->skyColourTop[1] = 1.0f;
    }
    gEnvironment->skyColourTop[2] = light.color[2] * 1.33f;
    if (gEnvironment->skyColourTop[2] > 1.0f) {
        gEnvironment->skyColourTop[2] = 1.0f;
    }
    gEnvironment->flags = ENV_FOG;
    gEnvironment->fogNear = 2.0f;
    gEnvironment->fogFar = 50.0f;
}

void set_light(light_t light) {
    glPushMatrix();
    glRotatef(light.direction[0], 1, 0, 0);
    glRotatef(light.direction[1], 0, 1, 0);
    glRotatef(light.direction[2], 0, 0, 1);
    glLightfv(GL_LIGHT0, GL_POSITION, light.position);
    glPopMatrix();

}

void setup_fog(light_t light) {
    glFogf(GL_FOG_START, gEnvironment->fogNear);
    glFogf(GL_FOG_END, gEnvironment->fogFar);
    glFogfv(GL_FOG_COLOR, light.color);
}

void render_end(void) {
    glDisable(GL_MULTISAMPLE_ARB);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);
}