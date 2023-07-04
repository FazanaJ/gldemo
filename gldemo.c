#include <libdragon.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>
#include <malloc.h>
#include <math.h>

#include "time.h"
#include "lights.h"
#include "sphere.h"
#include "plane.h"
#include "dummy_low.h"
#include "controls.h"
#include "entity.h"
#include "camera.h"

// Set this to 1 to enable rdpq debug output.
// The demo will only run for a single frame and stop.
#define DEBUG_RDP 0

// global variables??

static surface_t zbuffer;

static GLenum shade_model = GL_SMOOTH;
static bool fog_enabled = false;

static GLuint textures[5];
static sprite_t *sprites[5];
static const char *texture_path[5] = {
    "rom:/circle0.sprite",
    "rom:/diamond0.sprite",
    "rom:/pentagon0.sprite",
    "rom:/skin0.sprite",
    "rom:/triangle0.sprite",
};


static time_data_t time_data;

static camera_t camera= {
    distance_from_entity: 6,
    pitch: 30,
};

struct entity_t player = {
    position: {0, 0, 0,},
};


void setup(){


    zbuffer = surface_alloc(FMT_RGBA16, display_get_width(), display_get_height());

    for (uint32_t i = 0; i < 5; i++)
    {
        sprites[i] = sprite_load(texture_path[i]);
    }

    setup_cube();

    setup_sphere();
    make_sphere_mesh();

    setup_plane();
    make_plane_mesh();


    float aspect_ratio = (float)display_get_width() / (float)display_get_height();
    float near_plane = 1.0f;
    float far_plane = 6000.0f;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-near_plane*aspect_ratio, near_plane*aspect_ratio, -near_plane, near_plane, near_plane, far_plane);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    setup_lights();

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


void render(){

    if (fog_enabled) {
        glEnable(GL_FOG);
    } else {
        glDisable(GL_FOG);
    }
    glShadeModel(shade_model);

    surface_t *disp = display_get();

    rdpq_attach(disp, &zbuffer);

    gl_context_begin();

    glClearColor(environment_color[0], environment_color[1], environment_color[2], environment_color[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    
    set_camera(camera, player);

    set_light_positions(1.0f);

    // Set some global render modes that we want to apply to all models
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textures[3]);

    glPushMatrix();
	glTranslatef(player.position[0], player.position[1], player.position[2]);
    glRotatef(player.yaw, 0, 0, 1);
	glScalef(1.f, 1.f, 1.f);
    render_cube(); 
    glPopMatrix();

    glBindTexture(GL_TEXTURE_2D, textures[0%4]);
    render_plane();

    glBindTexture(GL_TEXTURE_2D, textures[1%4]);
    render_sphere(0);

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    gl_context_end();

    rdpq_detach_show();
}


int main(){

	debug_init_isviewer();
	debug_init_usblog();

    console_init();
    console_set_render_mode(RENDER_MANUAL);

    timer_init();
    print_time(time_data);
    
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

    while (1){

        time_management(&time_data);

        controller_scan();
        struct controller_data hold = get_keys_pressed();
        struct controller_data press = get_keys_down();
    
        move_entity_analog(hold, press, &player, camera);
        get_entity_position(&player, time_data);

        move_camera_analog(hold, press, &camera);
        set_camera_zoom(hold, press, &camera);
        get_camera_position(&camera, player);


        render();

        if (DEBUG_RDP) rspq_wait();
    }

}
