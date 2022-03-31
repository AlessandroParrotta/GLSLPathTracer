
#include "Ray.h"


Ray::Ray()
{
}

Ray::Ray(Vector3f orig, Vector3f dir) {
	this->orig = orig;
	this->dir = dir;
}

Ray::~Ray()
{
}
