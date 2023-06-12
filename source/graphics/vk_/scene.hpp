#pragma once

#include "include.hpp"
#include "vertex.hpp"

namespace vk_ {

unsigned char* stbLoadTexture(const char* path, uint32_t* width, uint32_t* height);
void stbFreeTexture(unsigned char* px);

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    Mesh() = default;

    Mesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
        : vertices(vertices)
        , indices(indices)
    {
    }
};

struct Scene {
    std::vector<Mesh> meshes;

    Scene();

    static Scene fromObj(const char* path);
    static Scene fromGltf(const char* path);
};

}