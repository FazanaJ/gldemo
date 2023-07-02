#ifndef AUXILIARY_MATH_H
#define AUXILIARY_MATH_H

#include <malloc.h>
#include <math.h>


/*==============================
    rad
    convert degrees to radians
==============================*/

float rad(float angle){
	float radian = M_PI / 180 * angle;
	return radian;
}

/*==============================
    deg
    convert radians to degrees
==============================*/

float deg(float rad){
	float angle = 180 / M_PI * rad;
	return angle;
}


/*==============================
    qi_sqrt
    quick inverse square root
==============================*/

float qi_sqrt(float number){

	long i;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	y  = number;
	i  = * ( long * ) &y;
	i  = 0x5f3759df - ( i >> 1 ); 
	y  = * ( float * ) &i;
	y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
//	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

	return y;
}

/*==============================
    lim
    auxiliary function for 8 directional movement
==============================*/

int lim(int input){
    if (input == 0) {return 0;}
    else {return 1;}
}



#endif
