#include <libdragon.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>
#include <malloc.h>

#include "render.h"
#include "../include/global.h"

#include "main.h"
#include "camera.h"

Environment *gEnvironment;
float gAspectRatio = 1.0f;

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

void project_camera(void) {
    Camera *c = gCamera;
    float nearClip = 1.0f;
    float farClip = 100.0f;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gAspectRatio = (float) display_get_width() / (float) (display_get_height());
    glFrustum(-nearClip * gAspectRatio, nearClip * gAspectRatio, -nearClip, nearClip, nearClip, farClip);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(c->pos[0], c->pos[1], c->pos[2], c->focus[0], c->focus[1], c->focus[2], 0.0f, 0.0f, 1.0f);
}

void render_sky(void) {
    Environment *e = gEnvironment;
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0f, display_get_width(), display_get_height(), 0.0f, -1.0f, 1.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glBegin(GL_QUADS);
    glColor3f(e->skyColourTop[0], e->skyColourTop[1], e->skyColourTop[2]);
    glVertex2i(0, 0);
    glColor3f(e->skyColourBottom[0], e->skyColourBottom[1], e->skyColourBottom[2]);
    glVertex2i(0, display_get_height());
    glVertex2i(display_get_width(), display_get_height());
    glColor3f(e->skyColourTop[0], e->skyColourTop[1], e->skyColourTop[2]);
    glVertex2i(display_get_width(), 0);
    glEnd();
}

void render_bush(void) {
    Environment *e = gEnvironment;
    glBegin(GL_QUADS);
    glColor3f(e->skyColourTop[0], e->skyColourTop[1], e->skyColourTop[2]);
    glTexCoord2f(0, 0);
    glVertex3i(-1, 0, 2);
    glColor3f(e->skyColourBottom[0], e->skyColourBottom[1], e->skyColourBottom[2]);
    glTexCoord2f(0, 1.024f);
    glVertex3i(-1, 0, 0);
    glTexCoord2f(1.024f, 1.024f);
    glVertex3i(1, 0, 0);
    glColor3f(e->skyColourTop[0], e->skyColourTop[1], e->skyColourTop[2]);
    glTexCoord2f(1.024f, 0);
    glVertex3i(1, 0, 2);
    glEnd();
}

void render_end(void) {
    glDisable(GL_MULTISAMPLE_ARB);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glScissor(0, 0, display_get_width(), display_get_height());
}