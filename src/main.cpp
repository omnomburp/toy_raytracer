#include <limits>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include "types.h"
#include "shapes.h"

#define PI 3.14159265358979323846

vec3f cast_ray(const vec3f& origin, const vec3f& direction, const Sphere& sphere) {
	float sphere_dist = std::numeric_limits<float>::max();
	const auto mask = sphere.ray_intersect(origin, direction, sphere_dist);
	return (vec3f(0.4, 0.4, 0.3) * mask) + (vec3f(0.2, 0.7, 0.8) * !mask);
}

void render(const Sphere& sphere) {
    constexpr int width    = 1024;
    constexpr int height   = 768;
    constexpr int fov = PI/2;
    std::vector<vec3f> framebuffer(width*height);

#pragma omp parallel for
    for (size_t j = 0; j<height; j++) {
        for (size_t i = 0; i<width; i++) {
            // shift by 0.5 to get the "center" of the pixel as i just means the left boundary of i
            constexpr float aspect_ratio = width/(float)height;
            float screen_width = tan(fov/2.) * aspect_ratio;
            float x = (2*(i + 0.5) / (float)width - 1) * screen_width;
            float y = -(2*(j + 0.5) / (float)height - 1) * /* world units*/(tan(fov/2.));
            vec3f dir = vec3f(x, y, -1).normalize();
            framebuffer[i+j*width] = cast_ray(vec3f(0,0,0), dir, sphere);
        }
    }

	std::ofstream ofs;
	ofs.open(".\\out.ppm", std::ofstream::out | std::ofstream::binary);
	ofs << "P6\n" << width << " " << height << "\n255\n";
    for (size_t i = 0; i < height*width; ++i) {
        for (size_t j = 0; j<3; j++) {
            ofs << (char)(255 * std::max(0.f, std::min(1.f, framebuffer[i][j])));
        }
    }
    ofs.close();
}

int main() {
	Sphere sphere(vec3f(-3, 0, -16), 2);
    render(sphere);

    return 0;
}
