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

public:
    static uint32_t findMemoryType(vk::PhysicalDevice& physicalDevice, uint32_t typeFilter, const vk::MemoryPropertyFlags memPropertyFlags);
    static vk::UniqueBuffer createBufferUnique(vk::Device& device, vk::DeviceSize& size, const vk::BufferUsageFlags usage);
    static vk::UniqueBuffer createStagingBufferUnique(vk::Device& device, vk::DeviceSize& size);
    static vk::UniqueDeviceMemory createDeviceMemoryUnique(vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::Buffer& buffer, vk::DeviceSize& size, const vk::MemoryPropertyFlags memoryFlags);
    static vk::UniqueDeviceMemory createStagingMemoryUnique(vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::Buffer& buffer, vk::DeviceSize& size);

    Buffer() = default;
    Buffer(vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::CommandPool& commandPool, vk::Queue& graphicsQueue, vk::DescriptorSetLayout& descriptorLayout, vk::ImageView& imageView, vk::Sampler& sampler, std::vector<vk_::Vertex>& vertices, std::vector<uint16_t>& indices, uint32_t concurrentFrames);

    vk::Buffer& vertexBuffer();
    vk::Buffer& indexBuffer();
    vk::DeviceMemory& vertexMemory();
    vk::DeviceMemory& indexMemory();
    vk::DescriptorSet& descriptorSet(size_t i);

    void updateUniformBuffer(size_t i, vk::Extent2D extent);
};

}

#endif
