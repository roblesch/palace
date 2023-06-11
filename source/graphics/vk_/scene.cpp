#include "scene.hpp"

#include "log.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

namespace vk_ {

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
    : vertices(std::vector<Vertex>())
    , indices(std::vector<uint32_t>())
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
        LOG_ERROR("Failed to load obj file: %s", "OBJ");
        return scene;
    }

    for (const auto& shape : shapes) {
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

            scene.vertices.push_back(vertex);
            scene.indices.push_back(scene.indices.size());
        }
    }

    return scene;
}

Scene Scene::fromGltf(const char* path)
{
    Scene scene;

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string warn, err;

    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, path);

    if (!warn.empty()) {
        LOG_WARN(warn.c_str(), "GLTF");
    }

    if (!err.empty()) {
        LOG_ERROR(err.c_str(), "GLTF");
        return scene;
    }

    if (!ret) {
        LOG_ERROR("Failed to parse gltf file", "GLTF");
        return scene;
    }

    return scene;
}

}