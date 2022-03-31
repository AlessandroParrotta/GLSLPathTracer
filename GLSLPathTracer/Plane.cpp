
#include "Plane.h"


Plane::Plane()
{
}

Plane::Plane(Vector3f pos, Vector3f normal, Material mat) {
	this->pos = pos;
	this->normal = normal;
	this->mat = mat;
}


Plane::~Plane()
{
}
