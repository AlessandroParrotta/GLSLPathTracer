#pragma once

#include <parrlibcore/vector3f.h>
#include "Material.h"

using namespace prb;

class Intersection{
public:
	Vector3f point;
	Vector3f normal;
	float dist;
	Material mat;
	bool notNull;

	Intersection();
	Intersection(Vector3f point, Vector3f normal, float dist, Material mat);

	~Intersection();
};

