#pragma once

#include "types.hpp"
#include <string>
#include <vector>

namespace pl {

struct Vertex {
    glm::vec3 pos { 0.0, 0.0, 0.0 };
    glm::vec3 normal { 0.0, 1.0, 0.0 };
    glm::vec3 color { 1.0, 1.0, 1.0 };
    glm::vec2 uv { 0.0, 0.0 };
};

struct Texture {
    std::string name;
    std::vector<unsigned char> data;
    uint32_t width;
    uint32_t height;
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

struct Node {
    Node* parent;
    Mesh* mesh;
    std::vector<uint32_t> children;
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
    glm::mat4 matrix;
};

struct GltfScene {
    std::vector<Mesh> meshes;
    std::vector<Texture> textures;
    std::vector<Node> nodes;
};

GltfScene loadGltfScene(const char* path);

}
