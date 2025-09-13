#pragma once

#include "types.h"

struct Light {
    Light(const vec3f& p, const float& i) : position(p), intensity(i) {}

    vec3f position;
    float intensity;
};

struct Material {
	Material(const float r ,const vec4f& a ,const vec3f& color, const float spec) : refractive_index(r), diffuse_color(color), albedo(a), specular_exponent(spec) {}
	Material() : refractive_index(1), albedo(1, 0, 0, 0), diffuse_color(), specular_exponent() {}

	float refractive_index;
	vec4f albedo;
	vec3f diffuse_color;
	float specular_exponent;
};

struct Sphere {
	vec3f center;
	float radius;
	Material material;

	Sphere(const vec3f& c, const float& r, const Material& m) : center{c}, radius{r}, material{m} {}

	bool ray_intersect(const vec3f& origin, const vec3f& direction, float& sphere_dist) const {
		vec3f l = center - origin;
		float tca = l * direction;
		float d2 = l*l - tca*tca;

		if (d2 > radius * radius)
			return false; 

		float thc = sqrtf(radius * radius - d2);
		sphere_dist = tca - thc;
		float t1 = tca + thc;

		if (sphere_dist < 0) {
			sphere_dist = t1;
			if (sphere_dist < 0) 
				return false;
		}

		return true;
	}
};