#ifndef RENDER_H
#define RENDER_H


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
    camera_transform(&camera);

    set_light_positions(1.0f);

    // Set some global render modes that we want to apply to all models
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textures[3]);

    glPushMatrix();
	glScalef(3.f, 3.f, 3.f);
	glTranslatef(player.pos[0], player.pos[1], 0.f);

    render_cube(); 

    glPopMatrix();

    glBindTexture(GL_TEXTURE_2D, textures[(texture_index + 1)%4]);
    render_plane();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    gl_context_end();

    rdpq_detach_show();
}


#endif