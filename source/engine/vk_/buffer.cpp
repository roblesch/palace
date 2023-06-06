#include "buffer.hpp"

namespace vk_ {

uint32_t Buffer::findMemoryType(vk::PhysicalDeviceMemoryProperties& memProperties, uint32_t typeFilter, vk::MemoryPropertyFlags& properties)
{
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return 0;
}

Buffer::Buffer(vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::CommandPool& commandPool, vk::Queue& graphicsQueue, std::vector<vk_::Vertex>& vertices)
{
    // vertex buffer
    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    vk::BufferCreateInfo stagingBufferInfo {
        .size = bufferSize,
        .usage = vk::BufferUsageFlagBits::eTransferSrc,
        .sharingMode = vk::SharingMode::eExclusive
    };

    vk::UniqueBuffer stagingBuffer = device.createBufferUnique(stagingBufferInfo);

    vk::MemoryRequirements memoryRequirements = device.getBufferMemoryRequirements(*stagingBuffer);
    vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();
    vk::MemoryPropertyFlags memoryFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

    vk::MemoryAllocateInfo stagingMemoryAllocInfo {
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = findMemoryType(memoryProperties, memoryRequirements.memoryTypeBits, memoryFlags)
    };

    vk::UniqueDeviceMemory stagingMemory = device.allocateMemoryUnique(stagingMemoryAllocInfo);
    device.bindBufferMemory(*stagingBuffer, *stagingMemory, 0);

    // transfer vertices
    void* data = device.mapMemory(*stagingMemory, vk::DeviceSize(0), bufferSize);
    memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    device.unmapMemory(*stagingMemory);

    // vertex buffer
    vk::BufferCreateInfo vertexBufferInfo {
        .size = bufferSize,
        .usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        .sharingMode = vk::SharingMode::eExclusive
    };

    m_uniqueVertexBuffer = device.createBufferUnique(vertexBufferInfo);

    memoryRequirements = device.getBufferMemoryRequirements(*m_uniqueVertexBuffer);
    memoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;

    vk::MemoryAllocateInfo vertexMemoryAllocInfo {
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = findMemoryType(memoryProperties, memoryRequirements.memoryTypeBits, memoryFlags)
    };

    m_uniqueVertexMemory = device.allocateMemoryUnique(vertexMemoryAllocInfo);
    device.bindBufferMemory(*m_uniqueVertexBuffer, *m_uniqueVertexMemory, 0);

    // copy buffer
    vk::CommandBufferAllocateInfo commandBufferInfo {
        .commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };

    auto commandBuffers = device.allocateCommandBuffersUnique(commandBufferInfo);
    vk::CommandBuffer commandBuffer = *commandBuffers[0];

    commandBuffer.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    {
        vk::BufferCopy copyRegion { .size = bufferSize };
        commandBuffer.copyBuffer(*stagingBuffer, *m_uniqueVertexBuffer, copyRegion);
    }
    commandBuffer.end();

    vk::SubmitInfo submitInfo {
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };

    graphicsQueue.submit(submitInfo);
    graphicsQueue.waitIdle();
}

vk::Buffer& Buffer::vertexBuffer()
{
    return *m_uniqueVertexBuffer;
}

vk::DeviceMemory& Buffer::vertexMemory()
{
    return *m_uniqueVertexMemory;
}

}
