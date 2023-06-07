#ifndef PALACE_ENGINE_PRIMITIVE_HPP
#define PALACE_ENGINE_PRIMITIVE_HPP

#include "include.hpp"

namespace vk_ {

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 uv;

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
        return { { { .location = 0,
                       .binding = 0,
                       .format = vk::Format::eR32G32Sfloat,
                       .offset = offsetof(Vertex, pos) },
            { .location = 1,
                .binding = 0,
                .format = vk::Format::eR32G32B32Sfloat,
                .offset = offsetof(Vertex, color) },
            {
                .location = 2,
                .binding = 0,
                .format = vk::Format::eR32G32Sfloat,
                .offset = offsetof(Vertex, uv)
            } } };
    }
};

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

}

#endif
