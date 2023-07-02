#ifndef ENTITY_H
#define ENTITY_H

#include <libdragon.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>
#include <malloc.h>
#include <math.h>
#include "vertex.h"

#include "camera.h"
#include "auxiliary_math.h"
#include "time_management.h"


typedef struct {
	
	float scale[3];
	float position[3];
	float dir[3];

	float pitch;
	float yaw;
    float roll;
	float horizontal_speed;
	float vertical_speed;

    mesh_t mesh;

} Entity;


/*==============================
    get_entity_position
    calculates entity coordinates
==============================*/

void get_entity_position(Entity *entity, TimeData time_data){

    float horizontal_distance = time_data.frame_duration * entity->horizontal_speed;

    entity->position[0] += horizontal_distance * sin(rad(entity->yaw));
    entity->position[1] -= horizontal_distance * cos(rad(entity->yaw));

    float vertical_distance = time_data.frame_duration * entity->vertical_speed;

    entity->position[2] += vertical_distance;

    if (entity->position[2] > 0 || entity->vertical_speed > 0 || entity->vertical_speed < 0 ) {

        entity->vertical_speed -= 10 * time_data.frame_duration;
    }

    if (entity->position[2] <= 0) {

        entity->vertical_speed = 0;
        entity->position[2] = 0;
    }
}


void setup_entity()
{
}

void draw_entity(Entity entity)
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glVertexPointer(3, GL_FLOAT, sizeof(vertex_t), (void*)(0*sizeof(float) + (void*)entity.mesh.vertices));
    glTexCoordPointer(2, GL_FLOAT, sizeof(vertex_t), (void*)(3*sizeof(float) + (void*)entity.mesh.vertices));
    glNormalPointer(GL_FLOAT, sizeof(vertex_t), (void*)(5*sizeof(float) + (void*)entity.mesh.vertices));
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(vertex_t), (void*)(8*sizeof(float) + (void*)entity.mesh.vertices));

    glDrawElements(GL_TRIANGLES, sizeof(entity.mesh.indices) / sizeof(uint16_t), GL_UNSIGNED_SHORT, entity.mesh.indices);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
}

void render_entity(Entity entity)
{
    rdpq_debug_log_msg("entity");
    glPushMatrix();
    glTranslatef(0,0.1f,0);

    draw_entity(entity);
    

    glPopMatrix();
}

/*==============================
    move_entity_analog_stick
    Moves entity with analog stick

void move_entity_analog_stick(Entity *entity, Camera camera, NUContData cont[1]){
	
	if (fabs(cont->stick_x) < 7){cont->stick_x = 0;}
	if (fabs(cont->stick_y) < 7){cont->stick_y = 0;}


	if ( cont->stick_x != 0 || cont->stick_y != 0) {
    	entity->yaw = deg(atan2(cont->stick_x, -cont->stick_y) - rad(camera.angle_around_entity));
        entity->horizontal_speed = fabs(7.0f / qi_sqrt(cont->stick_x * cont->stick_x + cont->stick_y * cont->stick_y));
    }

    if ( cont->stick_x == 0 && cont->stick_y == 0) {
        entity->horizontal_speed = 0;
    }
}
==============================*/


/*==============================
    move_entity_c_buttons
    Moves entity with c buttons

void move_entity_c_buttons(Entity *entity, Camera camera, NUContData cont[1]){

    float forward_input = lim(cont[0].button & D_CBUTTONS) - lim(contdata[0].button & U_CBUTTONS);
    float side_input = lim(cont[0].button & R_CBUTTONS) - lim(contdata[0].button & L_CBUTTONS);

	if (forward_input != 0 || side_input != 0) {
    	entity->yaw = deg(atan2(side_input, forward_input) - rad(camera.angle_around_entity));
        entity->horizontal_speed = 500;
    }
    
    if (forward_input == 0 && side_input == 0) {
        entity->horizontal_speed = 0;
    }
}
==============================*/


/*==============================

void handle_entity_actions(Entity *entity, NUContData cont[1]){

    if (cont[0].trigger & A_BUTTON) entity->vertical_speed = 700;
}
==============================*/


/*==============================
    move_entity
    Controls entity movement

void handle_entity(Entity *entity, Camera camera, NUContData cont[1]){

    handle_entity_actions(entity, cont);
    move_entity_analog_stick(&nick.entity, camera, contdata);
    //move_entity_c_buttons(&nick.entity, camera, contdata);
    get_entity_position(entity);
}
==============================*/



#endif
