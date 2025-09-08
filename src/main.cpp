#include <limits>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include "types.h"

void render() {
    constexpr int width    = 1024;
    constexpr int height   = 768;
    std::vector<vec3f> framebuffer(width*height);

    for (size_t j = 0; j<height; j++) {
        for (size_t i = 0; i<width; i++) {
            framebuffer[i+j*width] = vec3f(j/float(height),i/float(width), 0);
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
    render();

    return 0;
}
