#pragma once

#include "include.hpp"

namespace vk_ {

struct Vertex {
    glm::vec3 pos { 0.0, 0.0, 0.0 };
    glm::vec3 color { 1.0, 1.0, 1.0 };
    glm::vec2 texCoord { 0.0, 0.0 };

    Vertex() = default;

    Vertex(glm::vec3 pos, glm::vec3 color, glm::vec2 texCoord)
        : pos(pos)
        , color(color)
        , texCoord(texCoord)
    {
    }

    static vk::VertexInputBindingDescription bindingDescription()
    {
        return {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = vk::VertexInputRate::eVertex
        };
    }

    static std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions()
    {
        vk::VertexInputAttributeDescription posDescription {
            .location = 0,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(Vertex, pos)
        };
        vk::VertexInputAttributeDescription colorDescription {
            .location = 1,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(Vertex, color)
        };
        vk::VertexInputAttributeDescription texCoordDescription {
            .location = 2,
            .binding = 0,
            .format = vk::Format::eR32G32Sfloat,
            .offset = offsetof(Vertex, texCoord)
        };
        return { posDescription, colorDescription, texCoordDescription };
    }
};

}
