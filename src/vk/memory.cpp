#include "memory.hpp"

#define VMA_IMPLEMENTATION

namespace pl {

Memory::Memory(const MemoryCreateInfo& createInfo)
{
    device_ = createInfo.device;
    commandPool_ = createInfo.commandPool;
    graphicsQueue_ = createInfo.graphicsQueue;

    VmaAllocatorCreateInfo allocatorInfo {
        .physicalDevice = createInfo.physicalDevice,
        .device = createInfo.device,
        .instance = createInfo.instance
    };

    vmaCreateAllocator(&allocatorInfo, &allocator_);
}

Memory::~Memory()
{
    for (auto& buffer : buffers_)
        vmaDestroyBuffer(allocator_, buffer->buffer, buffer->allocation);
    for (auto& image : images_)
        vmaDestroyImage(allocator_, image->image, image->allocation);

    vmaDestroyAllocator(allocator_);
}

vk::UniqueCommandBuffer Memory::beginSingleUseCommandBuffer()
{
    vk::CommandBufferAllocateInfo bufferInfo {
        .commandPool = commandPool_,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };

    vk::UniqueCommandBuffer commandBuffer { std::move(device_.allocateCommandBuffersUnique(bufferInfo)[0]) };
    commandBuffer->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

    return commandBuffer;
}

void Memory::endSingleUseCommandBuffer(vk::CommandBuffer& commandBuffer)
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo {
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };

    graphicsQueue_.submit(submitInfo);
    graphicsQueue_.waitIdle();
}

VmaBuffer* Memory::createBuffer(void* src, size_t size, VkBufferUsageFlags usage)
{
    VkBufferCreateInfo stagingBufferInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    };
    VmaAllocationCreateInfo stagingAllocInfo {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    VmaBuffer staging;
    vmaCreateBuffer(allocator_, &stagingBufferInfo, &stagingAllocInfo, &staging.buffer, &staging.allocation, nullptr);

    void* data;
    vmaMapMemory(allocator_, staging.allocation, &data);
    memcpy(data, src, size);
    vmaUnmapMemory(allocator_, staging.allocation);

    VkBufferCreateInfo bufferInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    };
    VmaAllocationCreateInfo bufferAllocInfo {
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    auto buffer = new VmaBuffer;
    vmaCreateBuffer(allocator_, &bufferInfo, &bufferAllocInfo, &buffer->buffer, &buffer->allocation, nullptr);

    auto cmd = beginSingleUseCommandBuffer();
    vk::BufferCopy copy {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };
    cmd->copyBuffer(staging.buffer, buffer->buffer, 1, &copy);
    endSingleUseCommandBuffer(*cmd);
    vmaDestroyBuffer(allocator_, staging.buffer, staging.allocation);

    buffers_.push_back(buffer);
    return buffer;
}

VmaImage* Memory::createTextureImage(void* src, size_t size, vk::Extent3D extent)
{
    VkBufferCreateInfo stagingBufferInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    };
    VmaAllocationCreateInfo stagingAllocInfo {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    VmaBuffer staging;
    vmaCreateBuffer(allocator_, &stagingBufferInfo, &stagingAllocInfo, &staging.buffer, &staging.allocation, nullptr);

    void* data;
    vmaMapMemory(allocator_, staging.allocation, &data);
    memcpy(data, src, size);
    vmaUnmapMemory(allocator_, staging.allocation);

    VkImageCreateInfo imageInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent = extent,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
    };
    VmaAllocationCreateInfo imageAllocInfo {
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    auto image = new VmaImage;
    vmaCreateImage(allocator_, &imageInfo, &imageAllocInfo, &image->image, &image->allocation, nullptr);

    auto cmd = beginSingleUseCommandBuffer();
    // transition layout
    vk::ImageSubresourceRange range {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
    };
    vk::ImageMemoryBarrier barrier {
        .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
        .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eTransferDstOptimal,
        .image = image->image,
        .subresourceRange = range
    };
    // copy image
    vk::BufferImageCopy copy {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1 },
        .imageExtent = extent
    };
    cmd->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);
    endSingleUseCommandBuffer(*cmd);
    vmaDestroyBuffer(allocator_, staging.buffer, staging.allocation);

    images_.push_back(image);
    return image;
}

UniqueMemory createMemoryUnique(const MemoryCreateInfo& createInfo)
{
    return std::make_unique<Memory>(createInfo);
}

}