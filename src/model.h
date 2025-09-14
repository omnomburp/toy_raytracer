#pragma once

#include <cmath>
#include <tuple>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <array>

#include "types.h"
#include "shapes.h"

struct Model {
    Material material;
    std::vector<vec3f> vertices = {};
    std::vector<int> facet_vrt = {}; 

    Model(const std::string& file_path, const Material& m) : material(m) {
        std::ifstream file(file_path);
        if (!file) {
            std::cerr << "Can't load model " << file_path << std::endl;
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;

            std::istringstream iss(line);

            if (line.rfind("v ", 0) == 0) {
                char v;
                float x, y, z;
                iss >> v >> x >> y >> z;

                vec3f vertex = {
                    x,
                    y,
                    z
                };
                vertices.emplace_back(vertex);

            } else if (line.rfind("f ", 0) == 0) {
                char f;
                std::string v1, v2, v3;
                iss >> f >> v1 >> v2 >> v3;

                facet_vrt.push_back(std::stoi(v1) - 1);
                facet_vrt.push_back(std::stoi(v2) - 1);
                facet_vrt.push_back(std::stoi(v3) - 1);
            } 
        }

        std::cout << vertices.size() << "vertices" << std::endl;
        std::cout << facet_vrt.size() << "faces" << std::endl;

    }

    inline int nverts() const { return vertices.size(); }
    inline int nfaces() const { return facet_vrt.size()/3; }

    inline vec3f vert(const int i) const {
        return vertices[i];
    }

    inline vec3f vert(const int iface, const int nthvert) const {
        return vertices[facet_vrt[iface*3+nthvert]];
    }

    bool ray_intersect(const vec3f& origin, const vec3f& direction, const int i, float& t_dist) {
        constexpr float EPSILON = 1e-5;

        const vec3f v0 = vert(i, 0);
        const vec3f v1 = vert(i, 1);
        const vec3f v2 = vert(i, 2);

        vec3f edge1 = v1 - v0;
        vec3f edge2 = v2 - v0;

        vec3f pvec = cross(direction, edge2);
        float a = fabs(edge1 * pvec);

        if (a < EPSILON) {
            return false;
        }

        vec3f tvec = origin - v0;
        float u = tvec*pvec;
        if (u < 0 || u > a) return false;

        vec3f qvec = cross(tvec, edge1);
        float v = direction*qvec;
        if (v < 0 || u + v > a) return false;

        t_dist = edge2*qvec * (1./a);
    
        if (t_dist > EPSILON) {
            return true;
        }        
        
        return false;
    }
};