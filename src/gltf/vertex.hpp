#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>

namespace pl {

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
};

}
