#include "gltf.hpp"

#include "util/log.hpp"
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define STB_IMAGE_IMPLEMENTATION
#include "tiny_gltf.h"
#include <filesystem>

namespace fs = std::filesystem;

namespace pl {

GltfScene loadGltfScene(const char* path)
{
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string warn, err;

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);

    if (!warn.empty()) {
        pl::LOG_WARN(warn.c_str(), "GLTF");
    }

    if (!err.empty()) {
        pl::LOG_ERROR(err.c_str(), "GLTF");
        return {};
    }

    if (!ret) {
        pl::LOG_ERROR("Failed to parse gltf file", "GLTF");
        return {};
    }

    std::vector<Mesh> meshes;
    std::vector<Texture> textures;
    std::vector<Node> nodes;

    // textures
    for (const auto& _image : model.images) {
        auto uri = fs::path(path).parent_path() / _image.uri;

        Texture texture {
            .name = _image.uri,
            .width = static_cast<uint32_t>(_image.width),
            .height = static_cast<uint32_t>(_image.height)
        };
        std::copy(_image.image.begin(), _image.image.end(), std::back_inserter(texture.data));
        textures.emplace_back(std::move(texture));
    }

    // meshes
    for (const auto& _mesh : model.meshes) {
        Mesh mesh;

        for (const auto& _primitive : _mesh.primitives) {

            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;

            size_t vertexCount;
            const float* positions;
            const float* normals;
            const float* texCoords;

            // positions
            {
                const auto& accessor = model.accessors[_primitive.attributes.at("POSITION")];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];
                vertexCount = accessor.count;
                positions = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
            }

            // normals
            {
                const auto& accessor = model.accessors[_primitive.attributes.at("NORMAL")];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];
                normals = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
            }

            // texCoords
            {
                const auto& accessor = model.accessors[_primitive.attributes.at("TEXCOORD_0")];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];
                texCoords = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
            }

            // vertices
            for (size_t i = 0; i < vertexCount; i++) {
                vertices.emplace_back(
                    glm::make_vec3(&positions[i * 3]),
                    glm::make_vec3(&normals[i * 3]),
                    glm::vec3 { 1.0, 1.0, 1.0 },
                    glm::make_vec2(&texCoords[i * 2]));
            }

            // indices
            {
                const auto& accessor = model.accessors[_primitive.indices];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];
                const unsigned short* data = reinterpret_cast<const unsigned short*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                std::copy(data, data + accessor.count, std::back_inserter(indices));
            }

            // texture
            auto texture = model.materials[_primitive.material].pbrMetallicRoughness.baseColorTexture.index;
            auto image = texture > -1 ? model.textures[texture].source : -1;

            Primitive primitive {
                .vertices = std::move(vertices),
                .indices = std::move(indices),
                .texture = image > -1 ? &textures[image] : nullptr
            };

            mesh.primitives.emplace_back(std::move(primitive));
        }
        meshes.emplace_back(std::move(mesh));
    }

    // nodes
    nodes.resize(model.nodes.size());
    for (size_t i = 0; i < model.nodes.size(); i++) {
        const auto& _node = model.nodes[i];

        for (const auto& _child : _node.children) {
            nodes[_child].parent = &nodes[i];
        }

        nodes[i].mesh = _node.mesh > -1 ? &meshes[_node.mesh] : nullptr;

        std::copy(_node.children.begin(), _node.children.end(), std::back_inserter(nodes[i].children));

        nodes[i].translation = glm::vec3 { 0.0f };
        if (_node.translation.size() == 3)
            nodes[i].translation = glm::make_vec3(_node.translation.data());

        nodes[i].rotation = glm::mat4 { 1.0f };
        if (_node.rotation.size() == 4) {
            glm::quat q = glm::make_quat(_node.rotation.data());
            nodes[i].rotation = glm::mat4(q);
        }

        nodes[i].scale = glm::vec3 { 1.0f };
        if (_node.scale.size() == 3)
            nodes[i].scale = glm::make_vec3(_node.scale.data());

        if (_node.matrix.size() == 16)
            nodes[i].matrix = glm::make_mat4(_node.matrix.data());
    }

    return { std::move(meshes), std::move(textures), std::move(nodes) };
}

}
