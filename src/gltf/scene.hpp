#pragma once

#include "mesh.hpp"
#include <vector>

namespace pl {

struct Scene {
    std::vector<Mesh> meshes_;
    std::vector<Texture> textures_;
};

Scene loadGltfScene(const char* path);

}