#ifndef PALACE_ENGINE_VK_DEVICE_HPP
#define PALACE_ENGINE_VK_DEVICE_HPP

#include "include.hpp"

namespace vk_ {

class Device {
private:
    vk::PhysicalDevice m_physicalDevice;
    vk::UniqueDevice m_uniqueDevice;

    struct {
        uint32_t graphics;
    } queueFamilyIndices {};

    vk::Queue m_graphicsQueue;

public:
    Device() = default;
    Device(const vk::Instance& instance, const vk::SurfaceKHR& surface);

    vk::PhysicalDevice& getPhysical();
    vk::Device& get();
};

} // namespace vk_

#endif
