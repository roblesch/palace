#ifndef PALACE_GRAPHICS_VULKANPIPELINE_HPP
#define PALACE_GRAPHICS_VULKANPIPELINE_HPP

#include "include.hpp"

namespace vk_ {

class Pipeline {
private:
    vk::UniqueShaderModule m_vertex;
    vk::UniqueShaderModule m_fragment;

public:
    Pipeline() = default;
};

}

#endif
