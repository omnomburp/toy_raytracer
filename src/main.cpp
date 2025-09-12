#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <limits>
#include <cmath>
#include <vector>
#include "types.h"
#include "shapes.h"

#define PI 3.14159265358979323846

vec3f reflect(const vec3f& I, const vec3f& N) {
		return I - N*2.f*(I*N);
}

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

vec3f cast_ray(const vec3f& origin, const vec3f& direction, const std::vector<Sphere>& spheres, const std::vector<Light>& lights) {
	vec3f point, n;
    Material material;

    float diffuse_light_intensity = 0., specular_light_intensity = 0.;

    if (!scene_intersect(origin, direction, spheres, point, n, material)) {
        return vec3f(0.2, 0.7, 0.8);
    }
    
    for (size_t i = 0; i < lights.size(); ++i) {
        vec3f light_dir = (lights[i].position - point).normalize();
        float light_distance = (lights[i].position - point).norm();

        vec3f shadow_origin = light_dir * n < 0 ? point - n * 1e-3 : point + n * 1e-3; // check if the point lies in the shadow of lights[i] 
        vec3f shadow_point, shadow_N;
        Material tmp_material;

        if (scene_intersect(shadow_origin, light_dir, spheres, shadow_point, shadow_N, tmp_material))
            continue;

        diffuse_light_intensity += lights[i].intensity * std::max<float>(0., light_dir * n);
        specular_light_intensity += powf(std::max(0.f, reflect(light_dir, n)* direction), material.specular_exponent)*lights[i].intensity;
    }

    return material.diffuse_color * diffuse_light_intensity * material.albedo[0] + vec3f{1., 1., 1.}
    * specular_light_intensity * material.albedo[1];
}

void render(const std::vector<Sphere>& spheres, std::vector<Light>& lights) {
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
            framebuffer[i+j*width] = cast_ray(vec3f(0,0,0), dir, spheres, lights);
        }
    }

    std::vector<unsigned char> image(width * height * 3);

    for (size_t i = 0; i < height * width; ++i) {
        vec3f &c = framebuffer[i];
        float max = std::max(c[0], std::max(c[1], c[2]));
        if (max > 1) c = c*(1./max);
        image[3*i + 0] = (unsigned char)(255 * std::max(0.f, std::min(1.f, framebuffer[i][0])));
        image[3*i + 1] = (unsigned char)(255 * std::max(0.f, std::min(1.f, framebuffer[i][1])));
        image[3*i + 2] = (unsigned char)(255 * std::max(0.f, std::min(1.f, framebuffer[i][2])));
    }

    stbi_write_png("out.png", width, height, 3, image.data(), width * 3);
}

int main() {
    Material      ivory(vec2f(0.6, 0.3), vec3f(0.4, 0.4, 0.3), 50.);
    Material red_rubber(vec2f(0.9, 0.1) ,vec3f(0.3, 0.1, 0.1), 10.);

    std::vector<Sphere> spheres;
    spheres.push_back(Sphere(vec3f(-3,    0,   -16), 2,      ivory));
    spheres.push_back(Sphere(vec3f(-1.0, -1.5, -12), 2, red_rubber));
    spheres.push_back(Sphere(vec3f( 1.5, -0.5, -18), 3, red_rubber));
    spheres.push_back(Sphere(vec3f( 7,    5,   -18), 4,      ivory));

    std::vector<Light> lights;
    lights.push_back(Light(vec3f{-20., 20, 20.}, 1.5));
    lights.push_back(Light(vec3f( 30, 50, -25), 1.8));
    lights.push_back(Light(vec3f( 30, 20,  30), 1.7));

    render(spheres, lights);

    return 0;
}
