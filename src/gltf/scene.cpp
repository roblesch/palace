#include "scene.hpp"

#include "util/log.hpp"
#include <glm/gtc/type_ptr.hpp>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace pl {

unsigned char* stbLoadTexture(const char* path, uint32_t* width, uint32_t* height)
{
    int w, h, c;
    stbi_uc* px = stbi_load(path, &w, &h, &c, STBI_rgb_alpha);

    *width = static_cast<uint32_t>(w);
    *height = static_cast<uint32_t>(h);

    return px;
}

void stbFreeTexture(unsigned char* px)
{
    stbi_image_free(px);
}

Scene::Scene()
    : meshes(std::vector<Mesh>())
{
}

Scene Scene::fromObj(const char* path)
{
    Scene scene;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path)) {
        pl::LOG_ERROR("Failed to load obj file: %s", "OBJ");
        return scene;
    }

    for (const auto& shape : shapes) {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        for (const auto& index : shape.mesh.indices) {
            Vertex vertex {};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = { 1.0f, 1.0f, 1.0f };

            vertices.push_back(vertex);
            indices.push_back(indices.size());
        }

        scene.meshes.emplace_back(vertices, indices);
    }

    return scene;
}

Scene Scene::fromGltf(const char* path)
{
    Scene scene;

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string warn, err;

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);

    if (!warn.empty()) {
        pl::LOG_WARN(warn.c_str(), "GLTF");
    }

    if (!err.empty()) {
        pl::LOG_ERROR(err.c_str(), "GLTF");
        return scene;
    }

    if (!ret) {
        pl::LOG_ERROR("Failed to parse gltf file", "GLTF");
        return scene;
    }

    for (const auto& mesh : model.meshes) {
        for (const auto& primitive : mesh.primitives) {
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;

            const float* positions;
            const float* normals;
            const float* texCoords;
            size_t vertexCount;

            // positions
            {
                const auto& accessor = model.accessors[primitive.attributes.at("POSITION")];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];
                positions = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                vertexCount = accessor.count;
            }

            // normals
            {
                const auto& accessor = model.accessors[primitive.attributes.at("NORMAL")];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];
                normals = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
            }

            // texCoords
            {
                const auto& accessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
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
                const auto& accessor = model.accessors[primitive.indices];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];

                const unsigned short* data = reinterpret_cast<const unsigned short*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);

                for (size_t i = 0; i < accessor.count; i++) {
                    indices.push_back(data[i]);
                }
            }

            scene.meshes.emplace_back(vertices, indices);
        }
    }

    return scene;
}

}