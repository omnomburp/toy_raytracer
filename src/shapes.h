#pragma once

#include "types.h"

struct Sphere {
	vec3f center;
	float radius;

	Sphere(const vec3f& c, const float& r) : center{c}, radius{r} {}

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