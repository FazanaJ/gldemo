#ifndef CAMERA_H
#define CAMERA_H

#include <malloc.h>
#include <math.h>

#include "auxiliary_math.h"
#include "entity.h"

typedef struct {

	float distance_from_entity;
	float horizontal_distance_from_entity;
	float vertical_distance_from_entity;
    float angle_around_entity;

	float position[3];

	float pitch;
	float yaw;
	float roll;

} Camera;


/*==============================
    set_cam
    Sets the camera 
==============================*/

void set_cam(Camera *camera, Entity entity){

    glLoadIdentity();
    gluLookAt(
        camera->position[0], camera->position[1], camera->position[2],
        entity.position[0], entity.position[1], (entity.position[2] / 3) + 150,
        0, 0, 1);
}


/*==============================
    get_camera_position
    calculates camera coordinates

void get_camera_position(Camera *camera, Entity entity){

    camera->horizontal_distance_from_entity = camera->distance_from_entity * cos(rad(camera->pitch));
	camera->vertical_distance_from_entity = camera->distance_from_entity * sin(rad(camera->pitch));

    camera->position[0] = entity.position[0] - camera->horizontal_distance_from_entity * sin(rad(camera->angle_around_entity));
    camera->position[1] = entity.position[1] - camera->horizontal_distance_from_entity * cos(rad(camera->angle_around_entity));
    camera->position[2] = camera->vertical_distance_from_entity + entity.position[2];

    if ((camera->vertical_distance_from_entity + entity.position[2]) < 5){cam.position[2] = 5;}
}
==============================*/


/*==============================
    handle_camera_analog_stick
    moves camera with analog stick

void handle_camera_analog_stick(Camera *camera, NUContData cont[1]){

    if (fabs(cont->stick_x) < 7){cont->stick_x = 0;}
    if (fabs(cont->stick_y) < 7){cont->stick_y = 0;}

    camera->angle_around_entity += cont->stick_x / 40.0f;
    camera->pitch += cont->stick_y / 40.0f;

    if (cam.angle_around_entity > 360) {cam.angle_around_entity  = 0;}
    if (cam.angle_around_entity < 0) {cam.angle_around_entity  = 360;}

    if (cam.angle_around_entity  > 360) {cam.angle_around_entity  = 0;}
    if (cam.angle_around_entity  < 0) {cam.angle_around_entity  = 360;}

    if (cam.pitch > 85) {cam.pitch = 85;}
    if (cam.pitch < -85) {cam.pitch = -85;}
}
==============================*/


/*==============================
    handle_camera_c_buttons
    handles pitch, distance_from_entity 
    and angle_around_entity variables

void handle_camera_c_buttons(Camera *camera, NUContData cont[1]){

    if (cont[0].trigger & U_CBUTTONS && camera->distance_from_entity == 2000){
        camera->distance_from_entity = 1200;
        camera->pitch = 35;
    } else
    if (cont[0].trigger & U_CBUTTONS && camera->distance_from_entity == 1200){
        camera->distance_from_entity = 700;
        camera->pitch = 30;
    }

    if (cont[0].trigger & D_CBUTTONS && camera->distance_from_entity == 700){
        camera->distance_from_entity = 1200;
        camera->pitch = 35;
    } else
    if (cont[0].trigger & D_CBUTTONS && camera->distance_from_entity == 1200){
        camera->distance_from_entity = 2000;
        camera->pitch = 40;
    }

    if (cont[0].trigger & L_CBUTTONS){
        camera->angle_around_entity += 45;
    }
   
    if (cont[0].trigger & R_CBUTTONS && camera->angle_around_entity == 0){
        camera->angle_around_entity = 360;
        camera->angle_around_entity -= 45;
    }else
    if (cont[0].trigger & R_CBUTTONS){
        camera->angle_around_entity -= 45;
    }

    if (camera->angle_around_entity == 360){
        camera->angle_around_entity = 0;
    }
}
==============================*/


/*==============================
    move_camera
    Controls camera movement

void move_camera(Camera *camera, Entity entity, NUContData cont[1]){

    handle_camera_c_buttons(camera, cont);
    //handle_camera_analog_stick(camera, cont);
    get_camera_position(camera, entity);
}
==============================*/



#endif
