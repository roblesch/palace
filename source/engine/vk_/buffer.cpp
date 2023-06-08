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

Buffer::Buffer(vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::CommandPool& commandPool, vk::Queue& graphicsQueue, vk::DescriptorSetLayout& descriptorLayout, vk::ImageView& imageView, vk::Sampler& sampler, const std::vector<vk_::Vertex>& vertices, const std::vector<uint16_t>& indices, uint32_t concurrentFrames)
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
    m_vertexMemory = createDeviceMemoryUnique(physicalDevice, device, *m_vertexBuffer, vertexBufferSize, vk::MemoryPropertyFlagBits::eDeviceLocal);
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
    m_indexMemory = createDeviceMemoryUnique(physicalDevice, device, *m_indexBuffer, indexBufferSize, vk::MemoryPropertyFlagBits::eDeviceLocal);
    device.bindBufferMemory(*m_indexBuffer, *m_indexMemory, 0);
    copyBuffer(device, commandPool, graphicsQueue, *indexStagingBuffer, *m_indexBuffer, indexBufferSize);

    // uniform buffers
    vk::DeviceSize uniformBufferSize = sizeof(UniformBufferObject);

    m_uniformBuffers.resize(concurrentFrames);
    m_uniformMemorys.resize(concurrentFrames);
    m_uniformPtrs.resize(concurrentFrames);

    for (size_t i = 0; i < concurrentFrames; i++) {
        m_uniformBuffers[i] = createBufferUnique(device, uniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer);
        m_uniformMemorys[i] = createDeviceMemoryUnique(physicalDevice, device, *m_uniformBuffers[i], uniformBufferSize, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        device.bindBufferMemory(*m_uniformBuffers[i], *m_uniformMemorys[i], 0);
        m_uniformPtrs[i] = device.mapMemory(*m_uniformMemorys[i], 0, uniformBufferSize);
    }

    // descriptor pools
    vk::DescriptorPoolSize uboSize {
        .type = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = concurrentFrames
    };

    vk::DescriptorPoolSize samplerSize {
        .type = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = concurrentFrames
    };

    std::array<vk::DescriptorPoolSize, 2> poolSizes { uboSize, samplerSize };

    vk::DescriptorPoolCreateInfo descriptorPoolInfo {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = concurrentFrames,
        .poolSizeCount = 2,
        .pPoolSizes = poolSizes.data()
    };

    m_descriptorPool = device.createDescriptorPoolUnique(descriptorPoolInfo);

    // descriptor sets
    std::vector<vk::DescriptorSetLayout> layouts(concurrentFrames, descriptorLayout);

    vk::DescriptorSetAllocateInfo descriptorSetInfo {
        .descriptorPool = *m_descriptorPool,
        .descriptorSetCount = concurrentFrames,
        .pSetLayouts = layouts.data()
    };

    m_descriptorSets = device.allocateDescriptorSetsUnique(descriptorSetInfo);

    for (size_t i = 0; i < concurrentFrames; i++) {
        vk::DescriptorBufferInfo uboInfo {
            .buffer = *m_uniformBuffers[i],
            .offset = 0,
            .range = sizeof(UniformBufferObject)
        };

        vk::DescriptorImageInfo samplerInfo {
            .sampler = sampler,
            .imageView = imageView,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        };

        vk::WriteDescriptorSet uboWriteDescriptor {
            .dstSet = *m_descriptorSets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &uboInfo
        };

        vk::WriteDescriptorSet samplerWriteDescriptor {
            .dstSet = *m_descriptorSets[i],
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &samplerInfo
        };

        std::array<vk::WriteDescriptorSet, 2> writeDescriptors { uboWriteDescriptor, samplerWriteDescriptor };
        device.updateDescriptorSets(writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
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

vk::DeviceMemory& Buffer::vertexMemory()
{
    return *m_vertexMemory;
}

vk::DeviceMemory& Buffer::indexMemory()
{
    return *m_indexMemory;
}

vk::DescriptorSet& Buffer::descriptorSet(size_t frame)
{
    return *m_descriptorSets[frame];
}

void Buffer::updateUniformBuffer(size_t frame, UniformBufferObject& ubo)
{
    memcpy(m_uniformPtrs[frame], &ubo, sizeof(ubo));
}

}
