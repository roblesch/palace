#include "scene.hpp"

#include "util/log.hpp"
#include <glm/gtc/type_ptr.hpp>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <filesystem>
#include <tiny_obj_loader.h>

namespace fs = std::filesystem;

namespace pl {

unsigned char* stbLoadTexture(std::filesystem::path path)
{
    int w, h, c;
    return stbi_load(path.string().c_str(), &w, &h, &c, STBI_rgb_alpha);
}

void stbFreeTexture(unsigned char* px)
{
    stbi_image_free(px);
}

Scene loadGltfScene(const char* path)
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

    // textures
    for (const auto& _image : model.images) {
        auto uri = fs::path(path).parent_path() / _image.uri;

        Texture texture {
            .name = _image.uri, 
            .data = stbLoadTexture(uri),
            .width = static_cast<uint32_t>(_image.width),
            .height = static_cast<uint32_t>(_image.height)
        };

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
            size_t textureIdx = model.materials[_primitive.material].pbrMetallicRoughness.baseColorTexture.index;

            Primitive primitive {
                .vertices = std::move(vertices),
                .indices = std::move(indices),
                .texture = &textures[textureIdx]
            };

            mesh.primitives.emplace_back(std::move(primitive));
        }
        meshes.emplace_back(std::move(mesh));
    }

    return { std::move(meshes), std::move(textures) };
}

}