
#include "Sphere.h"


Sphere::Sphere(){
}

Sphere::Sphere(Vector3f pos, float radius, Material mat){
	this->pos = pos;
	this->radius = radius;
	this->mat = mat;
}

Sphere::~Sphere()
{
}
