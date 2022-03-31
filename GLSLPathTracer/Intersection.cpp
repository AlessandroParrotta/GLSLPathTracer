
#include "Intersection.h"


Intersection::Intersection()
{

}

Intersection::Intersection(Vector3f point, Vector3f normal, float dist, Material mat) {
	this->point = point;
	this->normal = normal;
	this->dist = dist;
	this->mat = mat;
	this->notNull = true;
}

Intersection::~Intersection()
{
}
