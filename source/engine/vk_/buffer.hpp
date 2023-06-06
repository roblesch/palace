#ifndef PALACE_ENGINE_VK_BUFFER_HPP
#define PALACE_ENGINE_VK_BUFFER_HPP

#include "include.hpp"
#include "primitive.hpp"

namespace vk_ {

class Buffer {
private:
    vk::UniqueBuffer m_uniqueVertexBuffer;
    vk::UniqueDeviceMemory m_uniqueVertexMemory;

    static uint32_t findMemoryType(vk::PhysicalDeviceMemoryProperties& memProperties, uint32_t typeFilter, vk::MemoryPropertyFlags& properties);

public:
    Buffer() = default;
    Buffer(vk::PhysicalDevice& physicalDevice, vk::Device& device, std::vector<vk_::Vertex>& vertices);

    vk::Buffer& vertexBuffer();
    vk::DeviceMemory& vertexMemory();
};

}

#endif
