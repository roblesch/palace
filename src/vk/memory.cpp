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

VmaImage* MemoryHelper::createImage(vk::Extent3D extent, vk::Format format, vk::ImageUsageFlags usage, uint32_t mipLevels, vk::SampleCountFlagBits samples)
{
    VkImageCreateInfo imageInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = (VkFormat)format,
        .extent = extent,
        .mipLevels = mipLevels,
        .arrayLayers = 1,
        .samples = (VkSampleCountFlagBits)samples,
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

VmaImage* MemoryHelper::createTextureImage(const void* src, size_t size, vk::Extent3D extent, uint32_t mipLevels)
{
    // upload to staging
    auto staging = createStagingBuffer(size);
    void* data;
    vmaMapMemory(allocator_, staging->allocation, &data);
    memcpy(data, src, size);
    vmaUnmapMemory(allocator_, staging->allocation);

    // create image
    auto texture = createImage(extent, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, mipLevels, vk::SampleCountFlagBits::e1);

    // transition staging format
    auto cmd = beginSingleUseCommandBuffer();
    auto transferBarrier = imageTransitionBarrier(texture->image, {}, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, mipLevels);
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

    // check if filter supported
    if (!(physicalDevice_.getFormatProperties(vk::Format::eR8G8B8A8Unorm).optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
    }

    // generate mipmaps
    vk::ImageMemoryBarrier barrier {
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = texture->image,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1 }
    };

    uint32_t mipWidth = extent.width;
    uint32_t mipHeight = extent.height;

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

        cmd->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);

        vk::ImageBlit blit {
            .srcSubresource = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = i - 1,
                .baseArrayLayer = 0,
                .layerCount = 1 },
            .dstSubresource = { .aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = i, .baseArrayLayer = 0, .layerCount = 1 }
        };
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { static_cast<int>(mipWidth), static_cast<int>(mipHeight), 1 };
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { static_cast<int>(mipWidth > 1 ? mipWidth / 2 : 1), static_cast<int>(mipHeight > 1 ? mipHeight / 2 : 1), 1 };

        cmd->blitImage(texture->image, vk::ImageLayout::eTransferSrcOptimal, texture->image, vk::ImageLayout::eTransferDstOptimal, 1, &blit, vk::Filter::eLinear);

        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        cmd->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);

        mipWidth = mipWidth > 1 ? mipWidth / 2 : mipWidth;
        mipHeight = mipHeight > 1 ? mipHeight / 2 : mipHeight;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    cmd->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);

    endSingleUseCommandBuffer(*cmd);
    vmaDestroyBuffer(allocator_, staging->buffer, staging->allocation);

    return texture;
}

vk::UniqueImageView MemoryHelper::createImageViewUnique(vk::Image image, vk::Format format, vk::ImageAspectFlagBits aspectMask, uint32_t mipLevels)
{
    vk::ImageViewCreateInfo imageViewInfo {
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .subresourceRange {
            .aspectMask = aspectMask,
            .baseMipLevel = 0,
            .levelCount = mipLevels,
            .baseArrayLayer = 0,
            .layerCount = 1 }
    };

    return device_.createImageViewUnique(imageViewInfo);
}

vk::UniqueSampler MemoryHelper::createImageSamplerUnique(uint32_t mipLevels)
{
    vk::SamplerCreateInfo samplerInfo
    {
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eNearest,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = physicalDevice_.getProperties().limits.maxSamplerAnisotropy,
        .compareEnable = VK_FALSE,
        .compareOp = vk::CompareOp::eAlways,
        .minLod = 0.0f,
        .maxLod = static_cast<float>(mipLevels),
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

vk::ImageMemoryBarrier MemoryHelper::imageTransitionBarrier(vk::Image image, vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels)
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
            .levelCount = mipLevels,
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
