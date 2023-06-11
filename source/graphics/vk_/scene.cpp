#include "scene.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include "log.hpp"

namespace vk_ {

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
        LOG_ERROR("Failed to load obj file: %s", path);
        exit(1);
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

}