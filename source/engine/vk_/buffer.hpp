#ifndef PALACE_ENGINE_VK_BUFFER_HPP
#define PALACE_ENGINE_VK_BUFFER_HPP

#include "include.hpp"
#include "primitive.hpp"

namespace vk_ {

class Buffer {
private:
    vk::UniqueBuffer m_uniqueVertexBuffer;
    vk::UniqueBuffer m_uniqueIndexBuffer;
    vk::UniqueDeviceMemory m_uniqueVertexMemory;
    vk::UniqueDeviceMemory m_uniqueIndexMemory;

    std::vector<vk::UniqueBuffer> m_uniqueUniformBuffers;
    std::vector<vk::UniqueDeviceMemory> m_uniqueUniformMemories;
    std::vector<void*> m_uniformMemoryPtrs;

    vk::UniqueDescriptorPool m_uniqueDescriptorPool;
    std::vector<vk::UniqueDescriptorSet> m_uniqueDescriptorSets;

    static uint32_t findMemoryType(vk::PhysicalDeviceMemoryProperties& memProperties, uint32_t typeFilter, const vk::MemoryPropertyFlags& properties);
    static void createBuffer(vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::DeviceSize& size, const vk::BufferUsageFlags& bufferFlags, const vk::MemoryPropertyFlags& memoryFlags, vk::UniqueBuffer& dstBuffer, vk::UniqueDeviceMemory& dstMemory);
    static void copyBuffer(vk::Device& device, vk::CommandPool& commandPool, vk::Queue& graphicsQueue, vk::Buffer src, vk::Buffer dst, vk::DeviceSize size);

public:
    Buffer() = default;
    Buffer(vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::CommandPool& commandPool, vk::Queue& graphicsQueue, vk::DescriptorSetLayout& descriptorLayout, std::vector<vk_::Vertex>& vertices, std::vector<uint16_t>& indices, uint32_t concurrentFrames);

    vk::Buffer& vertexBuffer();
    vk::Buffer& indexBuffer();
    vk::DeviceMemory& vertexMemory();
    vk::DeviceMemory& indexMemory();
    vk::DescriptorSet& descriptorSet(size_t i);

    void updateUniformBuffer(uint32_t currentImage, vk::Extent2D extent);
};

}

#endif
