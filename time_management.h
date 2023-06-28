#ifndef TIME_MANAGEMENT_H
#define TIME_MANAGEMENT_H

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdint.h>


typedef struct{
	int cur_frame;
	int last_frame;
	float frame_duration;
	int FPS;
}TimeData;

 
/*==============================
    get_time
    returns time in seconds
==============================*/

float get_time(){

    float time = timer_ticks() / 1000000.0f;
    return time;
}

/*==============================
    cycles_to_sec
    converts cycles to seconds
==============================*/

float ticks_to_sec(int ticks){

    float time = ticks / 1000000.0f;
    return time;
}


/*==============================
    time_management
    calculates FPS and frame_duration variable    
==============================*/

void time_management(TimeData *time){

    time->cur_frame = timer_ticks();

    time->frame_duration = ticks_to_sec(time->cur_frame - time->last_frame);

    time->FPS = 1 / time->frame_duration;

    time->last_frame = time->cur_frame;

}


#endif