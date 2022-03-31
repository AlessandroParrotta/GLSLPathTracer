#pragma once

#include <parrlib/math/Vector3f.h>

using namespace prb;

class Ray{
public:
	Vector3f orig;
	Vector3f dir;

	Ray();
	Ray(Vector3f orig, Vector3f dir);

	~Ray();
};

