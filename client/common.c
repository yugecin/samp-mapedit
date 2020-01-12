/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"

unsigned char hue(float t, int component)
{
	t += 1.0f / 3.0f * component;
	if (t < 0.0f) t += 1.0f;
	if (t > 1.0f) t -= 1.0f;
	if (t < 1.0f / 6.0f) {
		return (unsigned char) (255.0f * 6.0f * t);
	}
	if (t < 1.0f / 2.0f) {
		return 255;
	}
	if( t < 2.0f / 3.0f) {
		return (unsigned char) (255.0f * (4.0f - 6.0f * t));
	}
	return 0;
}
