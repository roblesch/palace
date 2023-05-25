#ifndef PALACE_GRAPHICS_VULKANDEVICE_HPP
#define PALACE_GRAPHICS_VULKANDEVICE_HPP

#include "vulkaninclude.hpp"

namespace graphics::vulkan {

struct Device {
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
    Device() = default;
    explicit Device(vk::Instance instance, vk::SurfaceKHR surface);
};

}

#endif
