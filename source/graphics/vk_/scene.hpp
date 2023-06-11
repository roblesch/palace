#pragma once

#include "include.hpp"
#include "vertex.hpp"

namespace vk_ {

struct Scene {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    Scene();

    static Scene fromObj(const char* path);
    static Scene fromGltf(const char* path);
};

}