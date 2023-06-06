#include "buffer.hpp"

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

Buffer::Buffer(vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::CommandPool& commandPool, vk::Queue& graphicsQueue, std::vector<vk_::Vertex>& vertices, std::vector<uint16_t>& indices)
{
    // vertex buffer
    vk::DeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
    vk::UniqueBuffer vertexStagingBuffer;
    vk::UniqueDeviceMemory vertexStagingMemory;

    createBuffer(physicalDevice, device, vertexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent, vertexStagingBuffer, vertexStagingMemory);

    void* vertexMemoryPtr = device.mapMemory(*vertexStagingMemory, vk::DeviceSize(0), vertexBufferSize);
    memcpy(vertexMemoryPtr, vertices.data(), static_cast<size_t>(vertexBufferSize));
    device.unmapMemory(*vertexStagingMemory);

    createBuffer(physicalDevice, device, vertexBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal, m_uniqueVertexBuffer, m_uniqueVertexMemory);

    copyBuffer(device, commandPool, graphicsQueue, *vertexStagingBuffer, *m_uniqueVertexBuffer, vertexBufferSize);

    // index buffer
    vk::DeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
    vk::UniqueBuffer indexStagingBuffer;
    vk::UniqueDeviceMemory indexStagingMemory;

    createBuffer(physicalDevice, device, indexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent, indexStagingBuffer, indexStagingMemory);

    void* indexMemoryPtr = device.mapMemory(*indexStagingMemory, vk::DeviceSize(0), indexBufferSize);
    memcpy(indexMemoryPtr, indices.data(), static_cast<size_t>(indexBufferSize));
    device.unmapMemory(*indexStagingMemory);

    createBuffer(physicalDevice, device, indexBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal, m_uniqueIndexBuffer, m_uniqueIndexMemory);

    copyBuffer(device, commandPool, graphicsQueue, *indexStagingBuffer, *m_uniqueIndexBuffer, indexBufferSize);
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

}
