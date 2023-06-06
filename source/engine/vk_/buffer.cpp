#include "buffer.hpp"

#include <chrono>
#include <glm/gtc/matrix_transform.hpp>

namespace vk_ {

uint32_t Buffer::findMemoryType(vk::PhysicalDeviceMemoryProperties& memProperties, uint32_t typeFilter, const vk::MemoryPropertyFlags& properties)
{
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return 0;
}

void Buffer::createBuffer(vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::DeviceSize& size, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& memoryFlags, vk::UniqueBuffer& dstBuffer, vk::UniqueDeviceMemory& dstMemory)
{
    vk::BufferCreateInfo bufferInfo {
        .size = size,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive
    };

    dstBuffer = device.createBufferUnique(bufferInfo);

    vk::MemoryRequirements memoryRequirements = device.getBufferMemoryRequirements(*dstBuffer);
    vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();

    vk::MemoryAllocateInfo memoryInfo {
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = findMemoryType(memoryProperties, memoryRequirements.memoryTypeBits, memoryFlags)
    };

    dstMemory = device.allocateMemoryUnique(memoryInfo);
    device.bindBufferMemory(*dstBuffer, *dstMemory, 0);
}

void Buffer::copyBuffer(vk::Device& device, vk::CommandPool& commandPool, vk::Queue& graphicsQueue, vk::Buffer src, vk::Buffer dst, vk::DeviceSize size)
{
    vk::CommandBufferAllocateInfo commandBufferInfo {
        .commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };

    auto commandBuffers = device.allocateCommandBuffersUnique(commandBufferInfo);
    vk::CommandBuffer commandBuffer = *commandBuffers[0];

    commandBuffer.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    commandBuffer.copyBuffer(src, dst, { { .size = size } });
    commandBuffer.end();

    vk::SubmitInfo submitInfo {
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };

    graphicsQueue.submit(submitInfo);
    graphicsQueue.waitIdle();
}

Buffer::Buffer(vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::CommandPool& commandPool, vk::Queue& graphicsQueue, vk::DescriptorSetLayout& descriptorLayout, std::vector<vk_::Vertex>& vertices, std::vector<uint16_t>& indices, uint32_t concurrentFrames)
{
    // vertex buffer
    vk::DeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
    vk::UniqueBuffer vertexStagingBuffer;
    vk::UniqueDeviceMemory vertexStagingMemory;

    createBuffer(physicalDevice, device, vertexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vertexStagingBuffer, vertexStagingMemory);

    void* vertexMemoryPtr = device.mapMemory(*vertexStagingMemory, 0, vertexBufferSize);
    memcpy(vertexMemoryPtr, vertices.data(), vertexBufferSize);
    device.unmapMemory(*vertexStagingMemory);

    createBuffer(physicalDevice, device, vertexBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, m_uniqueVertexBuffer, m_uniqueVertexMemory);
    copyBuffer(device, commandPool, graphicsQueue, *vertexStagingBuffer, *m_uniqueVertexBuffer, vertexBufferSize);

    // index buffer
    vk::DeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
    vk::UniqueBuffer indexStagingBuffer;
    vk::UniqueDeviceMemory indexStagingMemory;

    createBuffer(physicalDevice, device, indexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, indexStagingBuffer, indexStagingMemory);

    void* indexMemoryPtr = device.mapMemory(*indexStagingMemory, 0, indexBufferSize);
    memcpy(indexMemoryPtr, indices.data(), indexBufferSize);
    device.unmapMemory(*indexStagingMemory);

    createBuffer(physicalDevice, device, indexBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, m_uniqueIndexBuffer, m_uniqueIndexMemory);
    copyBuffer(device, commandPool, graphicsQueue, *indexStagingBuffer, *m_uniqueIndexBuffer, indexBufferSize);

    // uniform buffers
    vk::DeviceSize uniformBufferSize = sizeof(UniformBufferObject);

    m_uniqueUniformBuffers.resize(concurrentFrames);
    m_uniqueUniformMemories.resize(concurrentFrames);
    m_uniformMemoryPtrs.resize(concurrentFrames);

    for (size_t i = 0; i < concurrentFrames; i++) {
        createBuffer(physicalDevice, device, uniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, m_uniqueUniformBuffers[i], m_uniqueUniformMemories[i]);
        m_uniformMemoryPtrs[i] = device.mapMemory(*m_uniqueUniformMemories[i], 0, uniformBufferSize);
    }

    // descriptor pool
    vk::DescriptorPoolSize descriptorPoolSize {
        .type = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = concurrentFrames
    };

    vk::DescriptorPoolCreateInfo descriptorPoolInfo {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = concurrentFrames,
        .poolSizeCount = 1,
        .pPoolSizes = &descriptorPoolSize
    };

    m_uniqueDescriptorPool = device.createDescriptorPoolUnique(descriptorPoolInfo);

    // descriptor sets
    std::vector<vk::DescriptorSetLayout> layouts(concurrentFrames, descriptorLayout);

    vk::DescriptorSetAllocateInfo descriptorSetInfo {
        .descriptorPool = *m_uniqueDescriptorPool,
        .descriptorSetCount = concurrentFrames,
        .pSetLayouts = layouts.data()
    };

    m_uniqueDescriptorSets = device.allocateDescriptorSetsUnique(descriptorSetInfo);

    for (size_t i = 0; i < concurrentFrames; i++) {
        vk::DescriptorBufferInfo descriptorBufferInfo {
            .buffer = *m_uniqueUniformBuffers[i],
            .offset = 0,
            .range = sizeof(UniformBufferObject)
        };

        vk::WriteDescriptorSet writeDescriptorSet {
            .dstSet = *m_uniqueDescriptorSets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &descriptorBufferInfo
        };

        device.updateDescriptorSets(1, &writeDescriptorSet, 0, nullptr);
    }
}

vk::Buffer& Buffer::vertexBuffer()
{
    return *m_uniqueVertexBuffer;
}

vk::Buffer& Buffer::indexBuffer()
{
    return *m_uniqueIndexBuffer;
}

vk::DeviceMemory& Buffer::vertexMemory()
{
    return *m_uniqueVertexMemory;
}

vk::DeviceMemory& Buffer::indexMemory()
{
    return *m_uniqueIndexMemory;
}

vk::DescriptorSet& Buffer::descriptorSet(size_t i)
{
    return *m_uniqueDescriptorSets[i];
}

void Buffer::updateUniformBuffer(uint32_t currentImage, vk::Extent2D extent)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo {
        .model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .proj = glm::perspective(glm::radians(45.0f), extent.width / (float)extent.height, 0.1f, 10.0f)
    };
    ubo.proj[1][1] *= -1;

    memcpy(m_uniformMemoryPtrs[currentImage], &ubo, sizeof(ubo));
}

}
