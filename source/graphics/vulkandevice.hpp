#ifndef PALACE_GRAPHICS_VULKANDEVICE_HPP
#define PALACE_GRAPHICS_VULKANDEVICE_HPP

#include "vulkaninclude.hpp"

namespace graphics::vk_ {

class Device {
private:
    vk::PhysicalDevice m_physical;
    vk::UniqueDevice m_device;

    struct {
        uint32_t graphics;
    } queueFamilyIndices;

    vk::Queue m_gqueue;
    vk::Queue m_pqueue;

public:
    Device() = default;
    explicit Device(vk::Instance instance, vk::SurfaceKHR surface);

    vk::PhysicalDevice physicalDevice();
    vk::Device logicalDevice();
};

}

#endif
