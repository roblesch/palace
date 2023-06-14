#pragma once

#include <string>
#include <vector>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>

namespace pl {

struct Texture {
    std::string name;
    const unsigned char* data;
    uint32_t width;
    uint32_t height;
};

struct Vertex {
    glm::vec3 pos { 0.0, 0.0, 0.0 };
    glm::vec3 color { 1.0, 1.0, 1.0 };
    glm::vec2 texCoord { 0.0, 0.0 };
};

struct Primitive {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    Texture* texture;
};

struct Mesh {
    std::string name;
    std::vector<Primitive> primitives;
};

}
