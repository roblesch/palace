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
        uint32_t present;
    } queueFamilyIndices;

    vk::Queue m_gqueue;
    vk::Queue m_pqueue;

public:
    device() = default;
    explicit device(vk::Instance instance, vk::SurfaceKHR surface);
};

}

#endif
