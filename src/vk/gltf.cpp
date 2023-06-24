#include "gltf.hpp"

#include "vulkan.hpp"
#include "util/log.hpp"
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define STB_IMAGE_IMPLEMENTATION
#include "tiny_gltf.h"
#include <filesystem>

namespace fs = std::filesystem;

namespace pl {

GltfModel::GltfModel(const GltfModelCreateInfo& createInfo)
{
    auto path = createInfo.path;
    auto memory = createInfo.memory;

    defaultScene = 0;
    vertexBuffer = nullptr;
    indexBuffer = nullptr;

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string warn, err;

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);

    if (!warn.empty()) {
        pl::LOG_WARN(warn.c_str(), "GLTF");
    }

    if (!err.empty()) {
        pl::LOG_ERROR(err.c_str(), "GLTF");
        return;
    }

    if (!ret) {
        pl::LOG_ERROR("Failed to parse gltf file", "GLTF");
        return;
    }

    // scenes
    defaultScene = model.defaultScene;

    for (const auto& _scene : model.scenes) {
        std::vector<uint32_t> nodeIdxs;
        std::copy(_scene.nodes.begin(), _scene.nodes.end(), std::back_inserter(nodeIdxs));
        scenes.emplace_back(_scene.name, nodeIdxs);
    }

    // textures
    for (const auto& _image : model.images) {
        auto uri = fs::path(path).parent_path() / _image.uri;

        Texture texture {
            .name = _image.uri,
            .width = static_cast<uint32_t>(_image.width),
            .height = static_cast<uint32_t>(_image.height)
        };
        std::copy(_image.image.begin(), _image.image.end(), std::back_inserter(texture.data));
        textures.push_back(texture);
    }

    // meshes
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (const auto& _mesh : model.meshes) {
        Mesh mesh { .name = _mesh.name };

        for (const auto& _primitive : _mesh.primitives) {
            if (_primitive.indices < 0)
                continue;

            uint32_t firstVertex = static_cast<uint32_t>(vertices.size());
            uint32_t vertexCount;
            uint32_t firstIndex = static_cast<uint32_t>(indices.size());
            uint32_t indexCount;

            const float* positions;
            const float* normals;
            const float* texCoords;

            // positions
            {
                const auto& accessor = model.accessors[_primitive.attributes.at("POSITION")];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];
                vertexCount = static_cast<uint32_t>(accessor.count);
                positions = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
            }

            // normals
            if (_primitive.attributes.find("NORMAL") != _primitive.attributes.end()) {
                const auto& accessor = model.accessors[_primitive.attributes.at("NORMAL")];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];
                normals = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
            }

            // texCoords
            if (_primitive.attributes.find("TEXCOORD_0") != _primitive.attributes.end()) {
                const auto& accessor = model.accessors[_primitive.attributes.at("TEXCOORD_0")];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];
                texCoords = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
            }

            // color
            auto color = glm::make_vec4(model.materials[_primitive.material].pbrMetallicRoughness.baseColorFactor.data());

            // vertices
            for (size_t i = 0; i < vertexCount; i++) {
                vertices.emplace_back(
                    glm::make_vec3(&positions[i * 3]),
                    glm::make_vec3(&normals[i * 3]),
                    glm::vec3(color),
                    glm::make_vec2(&texCoords[i * 2]));
            }

            // indices
            {
                const auto& accessor = model.accessors[_primitive.indices];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];
                indexCount = accessor.count;

                auto readIndexBuffer = [&]<typename T>(T dummy) {
                    T* buf = new T[accessor.count];
                    memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(T));
                    for (size_t i = 0; i < accessor.count; i++) {
                        indices.push_back(buf[i] + firstVertex);
                    }
                    delete[] buf;
                };

                switch (accessor.componentType) {
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
                    readIndexBuffer(uint32_t {});
                    break;
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
                    readIndexBuffer(uint16_t {});
                    break;
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
                    readIndexBuffer(uint8_t {});
                    break;
                }
            }

            // texture
            auto texture = model.materials[_primitive.material].pbrMetallicRoughness.baseColorTexture.index;
            auto image = texture > -1 ? model.textures[texture].source : -1;

            Primitive primitive {
                .firstVertex = firstVertex,
                .vertexCount = vertexCount,
                .firstIndex = firstIndex,
                .indexCount = indexCount,
                .texture = image > -1 ? &textures[image] : nullptr
            };

            primitives.push_back(primitive);
            mesh.primitives.push_back(primitives.size() - 1);
        }
        meshes.push_back(mesh);
    }

    // buffers
    vertexBuffer = memory->createBuffer(vertices.data(), vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    indexBuffer = memory->createBuffer(indices.data(), indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    // nodes
    nodes.resize(model.nodes.size());
    for (size_t i = 0; i < model.nodes.size(); i++) {
        const auto& _node = model.nodes[i];
        for (const auto& _child : _node.children) {
            nodes[_child].parent = &nodes[i];
        }

        nodes[i].matrix = glm::mat4(1.0f);
        if (_node.translation.size() == 3)
            nodes[i].matrix = glm::translate(nodes[i].matrix, glm::vec3(glm::make_vec3(_node.translation.data())));

        if (_node.rotation.size() == 4) {
            glm::quat q = glm::make_quat(_node.rotation.data());
            nodes[i].matrix *= glm::mat4(q);
        }

        if (_node.scale.size() == 3)
            nodes[i].matrix = glm::scale(nodes[i].matrix, glm::vec3(glm::make_vec3(_node.scale.data())));

        if (_node.matrix.size() == 16)
            nodes[i].matrix = glm::make_mat4(_node.matrix.data());

        std::copy(_node.children.begin(), _node.children.end(), std::back_inserter(nodes[i].children));
        nodes[i].mesh = _node.mesh > -1 ? &meshes[_node.mesh] : nullptr;
    }
}

UniqueGltfScene createGltfModelUnique(const GltfModelCreateInfo& createInfo)
{
    return std::make_unique<GltfModel>(createInfo);
}

}
