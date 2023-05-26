#ifndef PALACE_GRAPHICS_VULKANPIPELINE_HPP
#define PALACE_GRAPHICS_VULKANPIPELINE_HPP

#include "vulkaninclude.hpp"

namespace graphics::vk_ {

class Pipeline {
private:
    vk::UniqueShaderModule m_vertex;
    vk::UniqueShaderModule m_fragment;

public:
    Pipeline() = default;
};

}

#endif
