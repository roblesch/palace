#pragma once

#include "include.hpp"
#include "vertex.hpp"

namespace vk_ {

unsigned char* stbLoadTexture(const char* path, uint32_t* width, uint32_t* height);
void stbFreeTexture(unsigned char* px);

struct Scene {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    Scene();

    static Scene fromObj(const char* path);
    static Scene fromGltf(const char* path);
};

}