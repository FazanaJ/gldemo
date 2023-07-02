#include <libdragon.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>
#include <malloc.h>
#include <math.h>

#include "camera.h"
#include "entity.h"
#include "time_management.h"
#include "auxiliary_math.h"
#include "dummy_low.h"
#include "plane.h"

// Set this to 1 to enable rdpq debug output.
// The demo will only run for a single frame and stop.
#define DEBUG_RDP 0

static uint32_t texture_index = 0;
static surface_t zbuffer;

static GLuint textures[5];
static sprite_t *sprites[5];
static const char *texture_path[5] = {
    "rom:/circle0.sprite",
    "rom:/diamond0.sprite",
    "rom:/pentagon0.sprite",
    "rom:/skin0.sprite",
    "rom:/triangle0.sprite",
};

static GLenum shade_model = GL_SMOOTH;
static bool fog_enabled = false;

static const GLfloat environment_color[] = { 0.6f, 0.6f, 0.6f, 0.6f };

static const GLfloat light_pos[8][4] = {
    { 1, 0, 0, 0 },
    { -1, 0, 0, 0 },
    { 0, 0, 1, 0 },
    { 0, 0, -1, 0 },
    { 8, 3, 0, 1 },
    { -8, 3, 0, 1 },
    { 0, 3, 8, 1 },
    { 0, 3, -8, 1 },
};

static const GLfloat light_diffuse[8][4] = {
    { 1.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 1.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 0.0f, 1.0f },
    { 1.0f, 0.0f, 1.0f, 1.0f },
    { 0.0f, 1.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
};



TimeData time_data;

Camera camera = {
    distance_from_entity: 1200,
    pitch: 30,
    angle_around_entity: 0,
};

Entity dummy = {

    pos: {0, 0, 0},
    mesh: dummy_mesh,
};


void setup()
{
    zbuffer = surface_alloc(FMT_RGBA16, display_get_width(), display_get_height());


    for (uint32_t i = 0; i < 5; i++)
    {
        sprites[i] = sprite_load(texture_path[i]);
    }


    setup_entity();

    setup_plane();
    make_plane_mesh();


    float aspect_ratio = (float)display_get_width() / (float)display_get_height();
    float near_plane = 1.0f;
    float far_plane = 5000.0f;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-near_plane*aspect_ratio, near_plane*aspect_ratio, -near_plane, near_plane, near_plane, far_plane);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();


    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, environment_color);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

    float light_radius = 10.0f;

    for (uint32_t i = 0; i < 8; i++)
    {
        glEnable(GL_LIGHT0 + i);
        glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, light_diffuse[i]);
        glLightf(GL_LIGHT0 + i, GL_LINEAR_ATTENUATION, 2.0f/light_radius);
        glLightf(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, 1.0f/(light_radius*light_radius));
    }

    GLfloat mat_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, mat_diffuse);


    glFogf(GL_FOG_START, 5);
    glFogf(GL_FOG_END, 20);
    glFogfv(GL_FOG_COLOR, environment_color);


    glGenTextures(5, textures);

    #if 0
    GLenum min_filter = GL_LINEAR_MIPMAP_LINEAR;
    #else
    GLenum min_filter = GL_LINEAR;
    #endif


    for (uint32_t i = 0; i < 5; i++)
    {
        glBindTexture(GL_TEXTURE_2D, textures[i]);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);

        glSpriteTextureN64(GL_TEXTURE_2D, sprites[i], &(rdpq_texparms_t){.s.repeats = REPEAT_INFINITE, .t.repeats = REPEAT_INFINITE});
    }
}


void set_light_positions()
{
    glPushMatrix();
    glRotatef(0, 0, 1, 0);

    for (uint32_t i = 0; i < 8; i++)
    {
        glLightfv(GL_LIGHT0 + i, GL_POSITION, light_pos[i]);
    }
    glPopMatrix();
}


void render()
{
    surface_t *disp = display_get();

    rdpq_attach(disp, &zbuffer);

    gl_context_begin();

    glClearColor(environment_color[0], environment_color[1], environment_color[2], environment_color[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);

    set_cam(&camera, dummy);

    set_light_positions();

    // Set some global render modes that we want to apply to all models
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);





    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textures[3]);
    render_entity(dummy); 




    glBindTexture(GL_TEXTURE_2D, textures[(texture_index + 1)%4]);
    render_plane();





    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    gl_context_end();

    rdpq_detach_show();
}


int main()
{
	debug_init_isviewer();
	debug_init_usblog();
    
    dfs_init(DFS_DEFAULT_LOCATION);

    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);

    rdpq_init();
    gl_init();

#if DEBUG_RDP
    rdpq_debug_start();
    rdpq_debug_log(true);
#endif

    setup();

    controller_init();

    while (1)
    {
        controller_scan();
        struct controller_data pressed = get_keys_pressed();
        struct controller_data down = get_keys_down();

        if (pressed.c[0].A) {
        }

        if (pressed.c[0].B) {
        }

        if (down.c[0].start) {
        }

        if (down.c[0].R) {
            shade_model = shade_model == GL_SMOOTH ? GL_FLAT : GL_SMOOTH;
            glShadeModel(shade_model);
        }

        if (down.c[0].L) {
            fog_enabled = !fog_enabled;
            if (fog_enabled) {
                glEnable(GL_FOG);
            } else {
                glDisable(GL_FOG);
            }
        }

        if (down.c[0].C_up) {
        }

        if (down.c[0].C_down) {
        }

        if (down.c[0].C_right) {
            texture_index = (texture_index + 1) % 4;
        }


        float y = pressed.c[0].y;
        float x = pressed.c[0].x;
        
        if (x < 7) {x = 0;}
        if (y < 7) {y = 0;}


        if (x != 0 || y != 0) {
            dummy.yaw = deg(atan2(x, -y) - rad(camera.angle_around_entity));
            dummy.horizontal_speed = fabs(7.0f / qi_sqrt(x * x + y * y));
        }

        if ( x == 0 && y == 0) {
            dummy.horizontal_speed = 0;
        }

        
        render();
        if (DEBUG_RDP)
            rspq_wait();
    }

}
