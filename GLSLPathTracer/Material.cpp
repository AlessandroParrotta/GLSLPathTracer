
#include "Material.h"


Material::Material()
{
}
Material::Material(Vector3f albedo, float metallic, float roughness, float emit) {
	this->albedo = albedo;
	this->metallic = metallic;
	this->roughness = roughness;
	this->emit = emit;
}

Material::~Material()
{
}
