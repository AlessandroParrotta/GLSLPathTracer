#pragma once

#include <parrlib/math/Vector3f.h>
#include "Material.h"

using namespace prb;

class Plane{
public:
	Vector3f pos;
	Vector3f normal;
	Material mat;

	Plane();
	Plane(Vector3f pos, Vector3f normal, Material mat);

	~Plane();
};

