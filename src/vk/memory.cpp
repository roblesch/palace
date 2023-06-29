#include "memory.hpp"

#define VMA_IMPLEMENTATION

namespace pl {

MemoryHelper::MemoryHelper(const MemoryHelperCreateInfo& createInfo)
    : physicalDevice_(createInfo.physicalDevice)
    , device_(createInfo.device)
    , commandPool_(createInfo.commandPool)
    , graphicsQueue_(createInfo.graphicsQueue)
{
    VmaAllocatorCreateInfo allocatorInfo {
        .physicalDevice = createInfo.physicalDevice,
        .device = createInfo.device,
        .instance = createInfo.instance
    };

    vmaCreateAllocator(&allocatorInfo, &allocator_);
}

MemoryHelper::~MemoryHelper()
{
    for (auto& buffer : buffers_)
        vmaDestroyBuffer(allocator_, buffer->buffer, buffer->allocation);
    for (auto& image : images_)
        vmaDestroyImage(allocator_, image->image, image->allocation);

    vmaDestroyAllocator(allocator_);
}

vk::UniqueCommandBuffer MemoryHelper::beginSingleUseCommandBuffer()
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

void MemoryHelper::endSingleUseCommandBuffer(vk::CommandBuffer& commandBuffer)
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo {
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };

    graphicsQueue_.submit(submitInfo);
    graphicsQueue_.waitIdle();
}

VmaBuffer* MemoryHelper::createBuffer(size_t size, vk::BufferUsageFlags usage, VmaAllocationCreateFlags flags)
{
    VkBufferCreateInfo bufferInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = (VkBufferUsageFlags)usage
    };
    VmaAllocationCreateInfo allocInfo {
        .flags = flags,
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    auto buffer = new VmaBuffer;
    buffer->size = size;
    vmaCreateBuffer(allocator_, &bufferInfo, &allocInfo, &buffer->buffer, &buffer->allocation, nullptr);
    buffers_.push_back(buffer);

    return buffer;
}

void MemoryHelper::uploadToBuffer(VmaBuffer* buffer, void* src)
{
    auto staging = createStagingBuffer(buffer->size);

    void* data;
    vmaMapMemory(allocator_, staging->allocation, &data);
    memcpy(data, src, buffer->size);
    vmaUnmapMemory(allocator_, staging->allocation);

    auto cmd = beginSingleUseCommandBuffer();
    vk::BufferCopy copy {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = buffer->size
    };
    cmd->copyBuffer(staging->buffer, buffer->buffer, 1, &copy);
    endSingleUseCommandBuffer(*cmd);

    vmaDestroyBuffer(allocator_, staging->buffer, staging->allocation);
}

void MemoryHelper::uploadToBufferDirect(VmaBuffer* buffer, void* src)
{
    void* data;
    vmaMapMemory(allocator_, buffer->allocation, &data);
    memcpy(data, src, buffer->size);
    vmaUnmapMemory(allocator_, buffer->allocation);
}

VmaImage* MemoryHelper::createImage(vk::Extent3D extent, vk::Format format, vk::ImageUsageFlags usage)
{
    VkImageCreateInfo imageInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = (VkFormat)format,
        .extent = extent,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = (VkImageUsageFlags)usage
    };
    VmaAllocationCreateInfo imageAllocInfo {
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    auto image = new VmaImage;

    vmaCreateImage(allocator_, &imageInfo, &imageAllocInfo, &image->image, &image->allocation, nullptr);
    images_.push_back(image);
    return image;
}

VmaImage* MemoryHelper::createTextureImage(const void* src, size_t size, vk::Extent3D extent)
{
    // upload to staging
    auto staging = createStagingBuffer(size);
    void* data;
    vmaMapMemory(allocator_, staging->allocation, &data);
    memcpy(data, src, size);
    vmaUnmapMemory(allocator_, staging->allocation);

    // create image
    auto texture = createImage(extent, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);

    // transition staging format
    auto cmd = beginSingleUseCommandBuffer();
    auto transferBarrier = imageTransitionBarrier(texture->image, {}, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    cmd->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, transferBarrier);

    // copy to image
    vk::BufferImageCopy copy {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1 },
        .imageOffset = { 0, 0, 0 },
        .imageExtent = extent
    };
    cmd->copyBufferToImage(staging->buffer, texture->image, vk::ImageLayout::eTransferDstOptimal, copy);

    // transition image format
    auto readableBarrier = imageTransitionBarrier(texture->image, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
    cmd->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, readableBarrier);
    endSingleUseCommandBuffer(*cmd);
    vmaDestroyBuffer(allocator_, staging->buffer, staging->allocation);

    return texture;
}

vk::UniqueImageView MemoryHelper::createImageViewUnique(vk::Image image, vk::Format format, vk::ImageAspectFlagBits aspectMask)
{
    vk::ImageViewCreateInfo imageViewInfo {
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .subresourceRange {
            .aspectMask = aspectMask,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1 }
    };

    return device_.createImageViewUnique(imageViewInfo);
}

vk::UniqueSampler MemoryHelper::createImageSamplerUnique()
{
    vk::SamplerCreateInfo samplerInfo {
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = physicalDevice_.getProperties().limits.maxSamplerAnisotropy,
        .compareEnable = VK_FALSE,
        .compareOp = vk::CompareOp::eAlways,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = vk::BorderColor::eIntOpaqueBlack,
        .unnormalizedCoordinates = VK_FALSE
    };

    return device_.createSamplerUnique(samplerInfo);
}

VmaBuffer* MemoryHelper::createStagingBuffer(size_t size)
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

    auto staging = new VmaBuffer;
    vmaCreateBuffer(allocator_, &stagingBufferInfo, &stagingAllocInfo, &staging->buffer, &staging->allocation, nullptr);
    return staging;
}

vk::ImageMemoryBarrier MemoryHelper::imageTransitionBarrier(vk::Image image, vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
    return {
        .srcAccessMask = srcAccessMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1 }
    };
}

UniqueMemoryHelper createMemoryHelperUnique(const MemoryHelperCreateInfo& createInfo)
{
    auto memoryHelper = new MemoryHelper(createInfo);
    return UniqueMemoryHelper(std::move(memoryHelper));
}

}
