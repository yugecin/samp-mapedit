/* vim: set filetype=c ts=8 noexpandtab: */

#define _CRT_SECURE_NO_DEPRECATE
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

/*pragmas seen in win10 api files that vc2005 can't handle*/
#pragma warning(disable : 4068)
#pragma warning(disable : 4163)

#include "../shared/sizecheck.h"
#include <stdlib.h>
#include <stdio.h>

#define M_PI 3.14159265358979323846f
#define M_PI2 1.57079632679489661923f

#define HUE_COMP_R 1
#define HUE_COMP_G 0
#define HUE_COMP_B -1

#if 0
#define TRACE(X) common_trace(X);
#else
#define TRACE(X)
#endif

#define INPUT_TEXTLEN 250

typedef void (msgboxcb)(int);

unsigned char hue(float angle, int component);
char *stristr(char *haystack, char *needle);
void center_camera_on(struct RwV3D *pos);
void common_trace(char *data);
void common_init();
void common_dispose();
int common_col(char *text, int *out);
