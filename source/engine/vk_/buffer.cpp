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

Buffer::Buffer(vk::PhysicalDevice& physicalDevice, vk::Device& device, std::vector<vk_::Vertex>& vertices)
{
    vk::BufferCreateInfo vertexBufferInfo {
        .size = sizeof(vertices[0]) * vertices.size(),
        .usage = vk::BufferUsageFlagBits::eVertexBuffer,
        .sharingMode = vk::SharingMode::eExclusive
    };

    m_uniqueVertexBuffer = device.createBufferUnique(vertexBufferInfo);

    vk::MemoryRequirements memoryRequirements = device.getBufferMemoryRequirements(vertexBuffer());
    vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();
    vk::MemoryPropertyFlags memoryFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

    vk::MemoryAllocateInfo memoryAllocInfo {
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = findMemoryType(memoryProperties, memoryRequirements.memoryTypeBits, memoryFlags)
    };

    m_uniqueVertexMemory = device.allocateMemoryUnique(memoryAllocInfo);
    device.bindBufferMemory(vertexBuffer(), vertexMemory(), 0);

    void* data = device.mapMemory(vertexMemory(), vk::DeviceSize(0), vertexBufferInfo.size);
    memcpy(data, vertices.data(), size_t(vertexBufferInfo.size));
    device.unmapMemory(vertexMemory());
}

vk::Buffer& Buffer::vertexBuffer()
{
    return m_uniqueVertexBuffer.get();
}

vk::DeviceMemory& Buffer::vertexMemory()
{
    return m_uniqueVertexMemory.get();
}

}
