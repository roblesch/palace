#ifndef PALACE_GRAPHICS_VULKANDEVICE_HPP
#define PALACE_GRAPHICS_VULKANDEVICE_HPP

#include "vulkaninclude.hpp"

namespace graphics::vulkan {

struct device {
private:
    vk::PhysicalDevice m_pdevice;
    vk::UniqueDevice m_ldevice;

    struct {
        uint32_t graphics;
    } queueFamilyIndices;

public:
    device() = default;
    explicit device(vk::Instance instance);
};

}

#endif
