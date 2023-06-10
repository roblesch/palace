#include "buffer.hpp"

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

vk::UniqueDeviceMemory Buffer::createDeviceMemoryUnique(vk::PhysicalDevice& physicalDevice, vk::Device& device, const vk::MemoryRequirements requirements, const vk::MemoryPropertyFlags memoryFlags)
{
    vk::MemoryAllocateInfo memoryInfo {
        .allocationSize = requirements.size,
        .memoryTypeIndex = Buffer::findMemoryType(physicalDevice, requirements.memoryTypeBits, memoryFlags)
    };

    return device.allocateMemoryUnique(memoryInfo);
}

vk::UniqueDeviceMemory Buffer::createStagingMemoryUnique(vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::Buffer& buffer, vk::DeviceSize& size)
{
    return createDeviceMemoryUnique(physicalDevice, device, device.getBufferMemoryRequirements(buffer), vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
}

Buffer::Buffer(vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::CommandPool& commandPool, vk::Queue& graphicsQueue, const std::vector<vk_::Vertex>& vertices, const std::vector<uint16_t>& indices, uint32_t concurrentFrames)
{
    // vertex buffer
    vk::DeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
    auto vertexStagingBuffer = createStagingBufferUnique(device, vertexBufferSize);
    auto vertexStagingMemory = createStagingMemoryUnique(physicalDevice, device, *vertexStagingBuffer, vertexBufferSize);
    device.bindBufferMemory(*vertexStagingBuffer, *vertexStagingMemory, 0);

    void* vertexMemoryPtr = device.mapMemory(*vertexStagingMemory, 0, vertexBufferSize);
    memcpy(vertexMemoryPtr, vertices.data(), vertexBufferSize);
    device.unmapMemory(*vertexStagingMemory);

    m_vertexBuffer = createBufferUnique(device, vertexBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer);
    m_vertexMemory = createDeviceMemoryUnique(physicalDevice, device, device.getBufferMemoryRequirements(*m_vertexBuffer), vk::MemoryPropertyFlagBits::eDeviceLocal);
    device.bindBufferMemory(*m_vertexBuffer, *m_vertexMemory, 0);
    copyBuffer(device, commandPool, graphicsQueue, *vertexStagingBuffer, *m_vertexBuffer, vertexBufferSize);

    // index buffer
    vk::DeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
    auto indexStagingBuffer = createStagingBufferUnique(device, indexBufferSize);
    auto indexStagingMemory = createStagingMemoryUnique(physicalDevice, device, *indexStagingBuffer, indexBufferSize);
    device.bindBufferMemory(*indexStagingBuffer, *indexStagingMemory, 0);

    void* indexMemoryPtr = device.mapMemory(*indexStagingMemory, 0, indexBufferSize);
    memcpy(indexMemoryPtr, indices.data(), indexBufferSize);
    device.unmapMemory(*indexStagingMemory);

    m_indexBuffer = createBufferUnique(device, indexBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer);
    m_indexMemory = createDeviceMemoryUnique(physicalDevice, device, device.getBufferMemoryRequirements(*m_indexBuffer), vk::MemoryPropertyFlagBits::eDeviceLocal);
    device.bindBufferMemory(*m_indexBuffer, *m_indexMemory, 0);
    copyBuffer(device, commandPool, graphicsQueue, *indexStagingBuffer, *m_indexBuffer, indexBufferSize);

    // uniform buffers
    vk::DeviceSize uniformBufferSize = sizeof(UniformBufferObject);

    m_uniformBuffers.resize(concurrentFrames);
    m_uniformMemorys.resize(concurrentFrames);
    m_uniformPtrs.resize(concurrentFrames);

    for (size_t i = 0; i < concurrentFrames; i++) {
        m_uniformBuffers[i] = createBufferUnique(device, uniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer);
        m_uniformMemorys[i] = createDeviceMemoryUnique(physicalDevice, device, device.getBufferMemoryRequirements(*m_uniformBuffers[i]), vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        device.bindBufferMemory(*m_uniformBuffers[i], *m_uniformMemorys[i], 0);
        m_uniformPtrs[i] = device.mapMemory(*m_uniformMemorys[i], 0, uniformBufferSize);
    }

}

vk::Buffer& Buffer::vertexBuffer()
{
    return *m_vertexBuffer;
}

vk::Buffer& Buffer::indexBuffer()
{
    return *m_indexBuffer;
}

std::vector<vk::UniqueBuffer>& Buffer::uniformBuffers()
{
    return m_uniformBuffers;
}

vk::DeviceMemory& Buffer::vertexMemory()
{
    return *m_vertexMemory;
}

vk::DeviceMemory& Buffer::indexMemory()
{
    return *m_indexMemory;
}

void Buffer::updateUniformBuffer(size_t frame, UniformBufferObject& ubo)
{
    memcpy(m_uniformPtrs[frame], &ubo, sizeof(ubo));
}

}
