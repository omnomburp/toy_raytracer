#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <limits>
#include <cmath>
#include <vector>
#include "types.h"
#include "shapes.h"

#define PI 3.14159265358979323846

vec3f reflect(const vec3f& I, const vec3f& N) {
		return I - N*2.f*(I*N);
}

vec3f refract(const vec3f& I, const vec3f& N, const float refractive_index) {
    float cosi =  - std::max<float>(-1.f, std::min(1.f, I*N));
    float etai = 1, etat = refractive_index;

    vec3f n = N;

    if (cosi < 0) {
        cosi = -cosi;
        std::swap(etai, etat);
        n = -N;
    }

    float eta = etai / etat;
    float k = 1 - eta * eta * (1 - cosi * cosi);

    return k < 0 ? vec3f(0, 0, 0) : I * eta + n * (eta * cosi - sqrtf(k));
}

bool scene_intersect(const vec3f& origin, const vec3f& direction, const std::vector<Sphere>& spheres,
vec3f& hit, vec3f& N, Material& material) {
    float spheres_dist = std::numeric_limits<float>::max();
    for (size_t i = 0; i < spheres.size(); ++i) {
        float dist_i;
        if (spheres[i].ray_intersect(origin, direction, dist_i) && dist_i < spheres_dist) {
            spheres_dist = dist_i;
            hit = origin + direction*dist_i; // the ray that hit
            N = (hit - spheres[i].center).normalize(); // surface normal
            material = spheres[i].material;
        }
    }
    
    float checkerboard_dist = std::numeric_limits<float>::max();

    if (fabs(direction.y) > 1e-3) {
        float d = -(origin.y + 4) / direction.y;
        vec3f pt = origin + direction * d;
            if (d>0 && fabs(pt.x)<10 && pt.z<-10 && pt.z>-30 && d<spheres_dist) {
                checkerboard_dist = d;
                hit = pt;
                N = vec3f(0,1,0);
                material.diffuse_color = (int(.5*hit.x+1000) + int(.5*hit.z)) & 1 ? vec3f(1,1,1) : vec3f(1, .7, .3);
                material.diffuse_color = material.diffuse_color*.3;
        }
        
    }
    return std::min(spheres_dist, checkerboard_dist) < 1000;
}

vec3f cast_ray(const vec3f& origin, const vec3f& direction, const std::vector<Sphere>& spheres, const std::vector<Light>& lights,
 const vec3f& bg, size_t depth = 0) {
	vec3f point, N;
    Material material;

    float diffuse_light_intensity = 0., specular_light_intensity = 0.;

    if (depth > 4 || !scene_intersect(origin, direction, spheres, point, N, material)) {
        return bg;
    }

    vec3f reflect_dir = reflect(direction, N).normalize();
    vec3f refract_dir = refract(direction, N, material.refractive_index).normalize();
    
    vec3f reflect_orig = reflect_dir * N < 0 ? point - N * 1e-3 : point + N * 1e-3;
    vec3f refract_orig = refract_dir * N < 0 ? point - N * 1e-3 : point + N * 1e-3;

    vec3f reflect_color = cast_ray(reflect_orig, reflect_dir, spheres, lights, bg, depth + 1);
    vec3f refract_color = cast_ray(refract_orig, refract_dir, spheres, lights, bg, depth + 1);
    
    for (size_t i = 0; i < lights.size(); ++i) {
        vec3f light_dir = (lights[i].position - point).normalize();
        float light_distance = (lights[i].position - point).norm();

        vec3f shadow_origin = light_dir * N < 0 ? point - N * 1e-3 /* pointing in different directions*/: point + N * 1e-3; // check if the point lies in the shadow of lights[i] 
        vec3f shadow_point, shadow_N;
        Material tmp_material;

        if (scene_intersect(shadow_origin, light_dir, spheres, shadow_point, shadow_N, tmp_material) && (shadow_point - shadow_origin).norm() < light_distance)
            continue;

        diffuse_light_intensity += lights[i].intensity * std::max<float>(0., light_dir * N);
        specular_light_intensity += powf(std::max(0.f, reflect(light_dir, N)* direction), material.specular_exponent)*lights[i].intensity;
    }

    return material.diffuse_color * diffuse_light_intensity * material.albedo[0] + vec3f(1., 1., 1.)
    * specular_light_intensity * material.albedo[1] + reflect_color*material.albedo[2] + refract_color*material.albedo[3];
}

void render(const std::vector<Sphere>& spheres, std::vector<Light>& lights) {
    constexpr int width    = 1024;
    constexpr int height   = 768;
    constexpr int fov = PI/2;
    int env_width, env_height, channels;
    std::vector<vec3f> framebuffer(width*height);

    float *data = stbi_loadf("envmap.jpg", &env_width, &env_height, &channels, 3);

#pragma omp parallel for
    for (size_t j = 0; j<height; j++) {
        for (size_t i = 0; i<width; i++) {
            // shift by 0.5 to get the "center" of the pixel as i just means the left boundary of i
            constexpr float aspect_ratio = width/(float)height;
            float screen_width = tan(fov/2.) * aspect_ratio;
            float x = (2*(i + 0.5) / (float)width - 1) * screen_width;
            float y = -(2*(j + 0.5) / (float)height - 1) * /* world units*/(tan(fov/2.));
                
            vec3f dir = vec3f(x, y, -1).normalize();

            float u = 0.5f + atan2(dir.z, dir.x) / (2 * PI);
            float v = 0.5f - asin(dir.y) / PI;

            int px = std::min(env_width - 1, std::max(0, int(u * env_width)));
            int py = std::min(env_height - 1, std::max(0, int(v * env_height)));
            int index = (py * env_width + px) * 3;
            float r = data[index + 0];
            float g = data[index + 1];
            float b = data[index + 2];

            framebuffer[i+j*width] = cast_ray(vec3f(0,0,0), dir, spheres, lights, vec3f(r, g, b));
        }
    }
    stbi_image_free(data);

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
    Material      ivory(1.0, vec4f(0.6,  0.3, 0.1, 0.0), vec3f(0.4, 0.4, 0.3),   50.);
    Material      glass(1.5, vec4f(0.0,  0.5, 0.1, 0.8), vec3f(0.6, 0.7, 0.8),  125.);
    Material red_rubber(1.0, vec4f(0.9,  0.1, 0.0, 0.0), vec3f(0.3, 0.1, 0.1),   10.);
    Material     mirror(1.0, vec4f(0.0, 10.0, 0.8, 0.0), vec3f(1.0, 1.0, 1.0), 1425.);

    std::vector<Sphere> spheres;
    spheres.push_back(Sphere(vec3f(-3,    0,   -16), 2,      ivory));
    spheres.push_back(Sphere(vec3f(-1.0, -1.5, -12), 2,      glass));
    spheres.push_back(Sphere(vec3f( 1.5, -0.5, -18), 3, red_rubber));
    spheres.push_back(Sphere(vec3f( 7,    5,   -18), 4,     mirror));

    std::vector<Light> lights;
    lights.push_back(Light(vec3f{-20., 20, 20.}, 1.5));
    lights.push_back(Light(vec3f( 30, 50, -25), 1.8));
    lights.push_back(Light(vec3f( 30, 20,  30), 1.7));

    render(spheres, lights);

    return 0;
}
