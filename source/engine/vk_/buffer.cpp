#include "buffer.hpp"

#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include "device.hpp"

namespace vk_ {

void copyBuffer(vk::Device& device, vk::CommandPool& commandPool, vk::Queue& graphicsQueue, vk::Buffer src, vk::Buffer dst, vk::DeviceSize size)
{
    vk::CommandBufferAllocateInfo commandBufferInfo {
        .commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };

    vk::UniqueCommandBuffer commandBuffer = Device::beginSingleUseCommandBuffer(device, commandPool);
    commandBuffer->copyBuffer(src, dst, { { .size = size } });
    Device::endSingleUseCommandBuffer(*commandBuffer, graphicsQueue);

    vk::SubmitInfo submitInfo {
        .commandBufferCount = 1,
        .pCommandBuffers = &(*commandBuffer)
    };

    graphicsQueue.submit(submitInfo);
    graphicsQueue.waitIdle();
}

uint32_t Buffer::findMemoryType(vk::PhysicalDevice& physicalDevice, uint32_t typeFilter, const vk::MemoryPropertyFlags memPropertyFlags)
{
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & memPropertyFlags) == memPropertyFlags) {
            return i;
        }
    }
    return 0;
}

vk::UniqueBuffer Buffer::createBufferUnique(vk::Device& device, vk::DeviceSize& size, const vk::BufferUsageFlags usage)
{
    vk::BufferCreateInfo bufferInfo {
        .size = size,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive
    };

    return device.createBufferUnique(bufferInfo);
}

vk::UniqueBuffer Buffer::createStagingBufferUnique(vk::Device& device, vk::DeviceSize& size)
{
    return createBufferUnique(device, size, vk::BufferUsageFlagBits::eTransferSrc);
}

vk::UniqueDeviceMemory Buffer::createDeviceMemoryUnique(vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::Buffer& buffer, vk::DeviceSize& size, const vk::MemoryPropertyFlags memoryFlags)
{
    vk::MemoryRequirements memoryRequirements = device.getBufferMemoryRequirements(buffer);

    vk::MemoryAllocateInfo memoryInfo {
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = Buffer::findMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, memoryFlags)
    };

    return device.allocateMemoryUnique(memoryInfo);
}

vk::UniqueDeviceMemory Buffer::createStagingMemoryUnique(vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::Buffer& buffer, vk::DeviceSize& size)
{
    return createDeviceMemoryUnique(physicalDevice, device, buffer, size, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
}

Buffer::Buffer(vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::CommandPool& commandPool, vk::Queue& graphicsQueue, vk::DescriptorSetLayout& descriptorLayout, std::vector<vk_::Vertex>& vertices, std::vector<uint16_t>& indices, uint32_t concurrentFrames)
{
    // unique handle deleter
    vk::ObjectDestroy<vk::Device, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> deleter(device);
    vk::ObjectFree<vk::Device, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> deleter2();

    // vertex buffer
    vk::DeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
    auto vertexStagingBuffer = createStagingBufferUnique(device, vertexBufferSize);
    auto vertexStagingMemory = createStagingMemoryUnique(physicalDevice, device, *vertexStagingBuffer, vertexBufferSize);
    device.bindBufferMemory(*vertexStagingBuffer, *vertexStagingMemory, 0);

    void* vertexMemoryPtr = device.mapMemory(*vertexStagingMemory, 0, vertexBufferSize);
    memcpy(vertexMemoryPtr, vertices.data(), vertexBufferSize);
    device.unmapMemory(*vertexStagingMemory);

    m_uniqueVertexBuffer = createBufferUnique(device, vertexBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer);
    m_uniqueVertexMemory = createDeviceMemoryUnique(physicalDevice, device, *m_uniqueVertexBuffer, vertexBufferSize, vk::MemoryPropertyFlagBits::eDeviceLocal);
    device.bindBufferMemory(*m_uniqueVertexBuffer, *m_uniqueVertexMemory, 0);
    copyBuffer(device, commandPool, graphicsQueue, *vertexStagingBuffer, *m_uniqueVertexBuffer, vertexBufferSize);

    // index buffer
    vk::DeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
    auto indexStagingBuffer = createStagingBufferUnique(device, indexBufferSize);
    auto indexStagingMemory = createStagingMemoryUnique(physicalDevice, device, *indexStagingBuffer, indexBufferSize);
    device.bindBufferMemory(*indexStagingBuffer, *indexStagingMemory, 0);

    void* indexMemoryPtr = device.mapMemory(*indexStagingMemory, 0, indexBufferSize);
    memcpy(indexMemoryPtr, indices.data(), indexBufferSize);
    device.unmapMemory(*indexStagingMemory);

    m_uniqueIndexBuffer = createBufferUnique(device, indexBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer);
    m_uniqueIndexMemory = createDeviceMemoryUnique(physicalDevice, device, *m_uniqueIndexBuffer, indexBufferSize, vk::MemoryPropertyFlagBits::eDeviceLocal);
    device.bindBufferMemory(*m_uniqueIndexBuffer, *m_uniqueIndexMemory, 0);
    copyBuffer(device, commandPool, graphicsQueue, *indexStagingBuffer, *m_uniqueIndexBuffer, indexBufferSize);

    // uniform buffers
    vk::DeviceSize uniformBufferSize = sizeof(UniformBufferObject);

    m_uniqueUniformBuffers.resize(concurrentFrames);
    m_uniqueUniformMemories.resize(concurrentFrames);
    m_uniformMemoryPtrs.resize(concurrentFrames);

    for (size_t i = 0; i < concurrentFrames; i++) {
        m_uniqueUniformBuffers[i] = createBufferUnique(device, uniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer);
        m_uniqueUniformMemories[i] = createDeviceMemoryUnique(physicalDevice, device, *m_uniqueUniformBuffers[i], uniformBufferSize, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        device.bindBufferMemory(*m_uniqueUniformBuffers[i], *m_uniqueUniformMemories[i], 0);
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

void Buffer::updateUniformBuffer(size_t i, vk::Extent2D extent)
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

    memcpy(m_uniformMemoryPtrs[i], &ubo, sizeof(ubo));
}

}
