#pragma once

#include "memory.hpp"
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
    VmaImage* image;
    vk::ImageView view;
    vk::DescriptorImageInfo descriptor;
    vk::Sampler sampler;
    uint32_t width;
    uint32_t height;
};

struct Material {
    Texture* baseColor;
    Texture* normal;
    vk::DescriptorSet descriptorSet;
};

struct Primitive {
    uint32_t firstVertex;
    uint32_t vertexCount;
    uint32_t firstIndex;
    uint32_t indexCount;
    Texture* texture;
};

struct Mesh {
    std::string name;
    std::vector<uint32_t> primitives;
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

struct Scene {
    std::string name;
    std::vector<uint32_t> nodes;
};

struct GltfModelCreateInfo {
    const char* path;
    Memory* memory;
};

class GltfModel {
public:
    GltfModel(const GltfModelCreateInfo& createInfo);

    uint32_t defaultScene;
    std::vector<Scene> scenes;
    std::vector<Mesh> meshes;
    std::vector<Primitive> primitives;
    std::vector<Texture> textures;
    std::vector<Material> materials;
    std::vector<Node> nodes;

    VmaBuffer* vertexBuffer;
    VmaBuffer* indexBuffer;
};

using UniqueGltfScene = std::unique_ptr<GltfModel>;

UniqueGltfScene createGltfModelUnique(const GltfModelCreateInfo& createInfo);

}
