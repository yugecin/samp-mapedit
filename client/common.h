/* vim: set filetype=c ts=8 noexpandtab: */

#define _CRT_SECURE_NO_DEPRECATE
#define WIN32_LEAN_AND_MEAN

#include "../shared/sizecheck.h"
#include <stdlib.h>
#include <stdio.h>

#define M_PI 3.14159265358979323846f
#define M_PI2 1.57079632679489661923f

#define HUE_COMP_R 1
#define HUE_COMP_G 0
#define HUE_COMP_B -1

unsigned char hue(float angle, int component);
