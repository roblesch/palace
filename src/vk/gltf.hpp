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
    vk::Extent3D extent;
    VmaImage* image;
    vk::UniqueImageView view;
};

struct Material {
    Texture* baseColor;
    Texture* normal;
    vk::UniquePipeline pipeline;
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
    std::vector<Primitive*> primitives;
};

struct Node {
    Node* parent;
    Mesh* mesh;
    std::vector<Node*> children;
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
    glm::mat4 matrix;
};

struct Scene {
    std::string name;
    std::vector<Node*> nodes;
};

struct GltfModelCreateInfo {
    const char* path;
    MemoryHelper* memory;
};

class GltfModel {
public:
    explicit GltfModel(const GltfModelCreateInfo& createInfo);

    std::vector<std::shared_ptr<Scene>> scenes;
    std::vector<std::shared_ptr<Mesh>> meshes;
    std::vector<std::shared_ptr<Primitive>> primitives;
    std::vector<std::shared_ptr<Texture>> textures;
    std::vector<std::shared_ptr<Material>> materials;
    std::vector<std::shared_ptr<Node>> nodes;

    Scene* defaultScene;
    VmaBuffer* vertexBuffer;
    VmaBuffer* indexBuffer;
};

using UniqueGltfModel = std::unique_ptr<GltfModel>;

UniqueGltfModel createGltfModelUnique(const GltfModelCreateInfo& createInfo);

}
