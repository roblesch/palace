#pragma once

#include "include.hpp"
#include "primitive.hpp"

namespace vk_ {

class Buffer {
private:
    vk::UniqueBuffer m_vertexBuffer;
    vk::UniqueBuffer m_indexBuffer;

    vk::UniqueDeviceMemory m_vertexMemory;
    vk::UniqueDeviceMemory m_indexMemory;

    std::vector<vk::UniqueBuffer> m_uniformBuffers;
    std::vector<vk::UniqueDeviceMemory> m_uniformMemorys;
    std::vector<void*> m_uniformPtrs;

public:
    static uint32_t findMemoryType(vk::PhysicalDevice& physicalDevice, uint32_t typeFilter, const vk::MemoryPropertyFlags memPropertyFlags);
    static vk::UniqueBuffer createBufferUnique(vk::Device& device, vk::DeviceSize& size, const vk::BufferUsageFlags usage);
    static vk::UniqueBuffer createStagingBufferUnique(vk::Device& device, vk::DeviceSize& size);
    static vk::UniqueDeviceMemory createDeviceMemoryUnique(vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::Buffer& buffer, vk::DeviceSize& size, const vk::MemoryPropertyFlags memoryFlags);
    static vk::UniqueDeviceMemory createStagingMemoryUnique(vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::Buffer& buffer, vk::DeviceSize& size);

    Buffer() = default;
    Buffer(vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::CommandPool& commandPool, vk::Queue& graphicsQueue, const std::vector<vk_::Vertex>& vertices, const std::vector<uint16_t>& indices, uint32_t concurrentFrames);

    vk::Buffer& vertexBuffer();
    vk::Buffer& indexBuffer();
    std::vector<vk::UniqueBuffer>& uniformBuffers();
    vk::DeviceMemory& vertexMemory();
    vk::DeviceMemory& indexMemory();

    void updateUniformBuffer(size_t frame, UniformBufferObject& ubo);
};

}
