#ifndef PALACE_ENGINE_PRIMITIVE_HPP
#define PALACE_ENGINE_PRIMITIVE_HPP

#include "include.hpp"
#include <glm/glm.hpp>

namespace vk_ {

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static vk::VertexInputBindingDescription bindingDescription()
    {
        return {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = vk::VertexInputRate::eVertex
        };
    }

    static std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions()
    {
        return { { { .location = 0,
                       .binding = 0,
                       .format = vk::Format::eR32G32Sfloat,
                       .offset = offsetof(Vertex, pos) },
            { .location = 1,
                .binding = 0,
                .format = vk::Format::eR32G32B32Sfloat,
                .offset = offsetof(Vertex, color)

            } } };
    }
};

}

#endif
