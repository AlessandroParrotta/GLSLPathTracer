#pragma once

#include <parrlib/math/vector2f.h>
#include <parrlib/math/vector3f.h>

using namespace prb;

class Material
{
public:
	Vector3f albedo;
	float metallic;
	float roughness;
	float emit;

	Material();
	Material(Vector3f albedo, float metallic, float roughness, float emit);

	~Material();
};

