#ifndef PALACE_GRAPHICS_VULKANDEVICE_HPP
#define PALACE_GRAPHICS_VULKANDEVICE_HPP

#include "include.hpp"

namespace vk_ {

class Device {
private:
    vk::PhysicalDevice m_physicalDevice;
    vk::UniqueDevice m_uniqueDevice;

    struct {
        uint32_t graphics;
    } queueFamilyIndices{};

    vk::Queue m_graphicsQueue;

public:
    Device() = default;
    Device(const vk::Instance& instance, const vk::SurfaceKHR& surface);

    vk::PhysicalDevice physicalDevice();
    vk::Device device();
};

}

#endif
