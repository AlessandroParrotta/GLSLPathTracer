#pragma once

#include <parrlib/math/vector3f.h>
#include "Material.h"

using namespace prb;

class Sphere{
public:
	Vector3f pos;
	float radius;
	Material mat;
	int emit;

	Sphere();
	Sphere(Vector3f pos, float radius, Material mat);

	~Sphere();
};

