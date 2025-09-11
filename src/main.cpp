#include <limits>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include "types.h"
#include "shapes.h"

#define PI 3.14159265358979323846

bool scene_intersect(const vec3f& origin, const vec3f& direction, const std::vector<Sphere>& spheres,
vec3f& hit, vec3f& n, Material& material) {
    float spheres_dist = std::numeric_limits<float>::max();
    for (size_t i = 0; i < spheres.size(); ++i) {
        float dist_i;
        if (spheres[i].ray_intersect(origin, direction, dist_i) && dist_i < spheres_dist) {
            spheres_dist = dist_i;
            hit = origin + direction*dist_i; // the ray that hit
            n = (hit - spheres[i].center).normalize(); // surface normal
            material = spheres[i].material;
        }
    }
    return spheres_dist < 1000;
}

vec3f cast_ray(const vec3f& origin, const vec3f& direction, const std::vector<Sphere>& spheres) {
	vec3f point, n;
    Material material;

    if (!scene_intersect(origin, direction, spheres, point, n, material)) {
        return vec3f(0.2, 0.7, 0.8);
    }

    return material.diffuse_color;
}

void render(const std::vector<Sphere>& spheres) {
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
            framebuffer[i+j*width] = cast_ray(vec3f(0,0,0), dir, spheres);
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
    Material      ivory(vec3f(0.4, 0.4, 0.3));
    Material red_rubber(vec3f(0.3, 0.1, 0.1));

    std::vector<Sphere> spheres;
    spheres.push_back(Sphere(vec3f(-3,    0,   -16), 2,      ivory));
    spheres.push_back(Sphere(vec3f(-1.0, -1.5, -12), 2, red_rubber));
    spheres.push_back(Sphere(vec3f( 1.5, -0.5, -18), 3, red_rubber));
    spheres.push_back(Sphere(vec3f( 7,    5,   -18), 4,      ivory));

    render(spheres);

    return 0;
}
