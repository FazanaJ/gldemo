#ifndef PLAYER_CONTROLS_H
#define PLAYER_CONTROLS_H

#include <libdragon.h>
#include <math.h>

#include "entity.h"
#include "camera.h"


void move_entity_analog(struct controller_data hold, struct controller_data press, struct entity_t* entity, camera_t camera) {


	if (fabs(hold.c[0].y) < 7){hold.c[0].y = 0;}
	if (fabs(hold.c[0].x) < 7){hold.c[0].x = 0;}

	float ly = hold.c[0].y ;
	float lx = hold.c[0].x ;

	if ( lx != 0 || ly != 0) {
    	entity->yaw = deg(atan2(lx, -ly) - rad(camera.angle_around_entity));
        entity->horizontal_speed = fabs(sqrt(lx * lx + ly * ly)) / 100;
    }

    if ( lx == 0 && ly == 0) {
        entity->horizontal_speed = 0;
    }
}

void move_camera_analog(struct controller_data hold, struct controller_data press, camera_t *camera){

    if (fabs(hold.c[0].y) < 7){hold.c[0].y = 0;}
	if (fabs(hold.c[0].x) < 7){hold.c[0].x = 0;}

    float ry = hold.c[1].y ;
	float rx = hold.c[1].x ;

    camera->angle_around_entity += rx / 20.0f;
    camera->pitch += ry / 20.0f;

    if (camera->angle_around_entity > 360) {camera->angle_around_entity  = 0;}
    if (camera->angle_around_entity < 0) {camera->angle_around_entity  = 360;}

    if (camera->angle_around_entity  > 360) {camera->angle_around_entity  = 0;}
    if (camera->angle_around_entity  < 0) {camera->angle_around_entity  = 360;}

    if (camera->pitch > 85) {camera->pitch = 85;}
    if (camera->pitch < -85) {camera->pitch = -85;}
}

void set_camera_zoom(struct controller_data hold, struct controller_data press, camera_t *camera){

    if (hold.c[0].L && camera->distance_from_entity > 3) {camera->distance_from_entity -= 1;}
    if (hold.c[0].R) {camera->distance_from_entity += 1;}

}

/*==============================
    move_entity_c_buttons
    Moves entity with c buttons

void move_entity_c_buttons(Entity *entity, Camera camera, NUContData cont[1]){

    float forward_input = lim(cont[0].button & D_CBUTTONS) - lim(contdata[0].button & U_CBUTTONS);
    float side_input = lim(cont[0].button & R_CBUTTONS) - lim(contdata[0].button & L_CBUTTONS);

	if (forward_input != 0 || side_input != 0) {
    	entity->yaw = deg(atan2(side_input, forward_input) - rad(camera->angle_around_entity));
        entity->horizontal_speed = 500;
    }
    
    if (forward_input == 0 && side_input == 0) {
        entity->horizontal_speed = 0;
    }
}
==============================*/

void handle_controller_input_player(struct controller_data hold, struct controller_data press, struct entity_t* player) {

  if (hold.c[0].A) {}

  if (hold.c[0].B) {}

  if (press.c[0].start) {}

  if (press.c[0].R) {}

  if (press.c[0].L) {}

  if (press.c[0].C_up) {}

  if (press.c[0].C_down) {}

  if (press.c[0].C_right) {}


  //float ly = hold.c[0].y;
  //float lx = hold.c[0].x;

  //float ry = hold.c[1].y;
  //float rx = hold.c[1].x;

}


#endif