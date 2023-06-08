#include "texture.hpp"

#include "buffer.hpp"
#include "device.hpp"
#include "swapchain.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace vk_ {

void transitionImageLayout(vk::Device& device, vk::CommandPool& commandPool, vk::Queue& graphicsQueue, vk::Image& image, const vk::Format format, const vk::ImageLayout oldLayout, const vk::ImageLayout newLayout)
{
    vk::PipelineStageFlags srcStageMask;
    vk::PipelineStageFlags dstStageMask;
    vk::AccessFlags srcAccessMask;
    vk::AccessFlags dstAccessMask;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        srcAccessMask = {};
        dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        srcStageMask = vk::PipelineStageFlagBits::eTopOfPipe;
        dstStageMask = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        dstAccessMask = vk::AccessFlagBits::eShaderRead;
        srcStageMask = vk::PipelineStageFlagBits::eTransfer;
        dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
    }

    vk::ImageMemoryBarrier barrier {
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

    vk::UniqueCommandBuffer commandBuffer = Device::beginSingleUseCommandBuffer(device, commandPool);
    commandBuffer->pipelineBarrier(srcStageMask, dstStageMask, {}, {}, {}, barrier);
    Device::endSingleUseCommandBuffer(*commandBuffer, graphicsQueue);
}

Texture::Texture(const char* path, vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::CommandPool& commandPool, vk::Queue& graphicsQueue)
{
    // load bytes
    int w, h, c;
    stbi_uc* px = stbi_load(path, &w, &h, &c, STBI_rgb_alpha);
    vk::DeviceSize imageSize = w * h * 4;
    uint32_t width = static_cast<uint32_t>(w);
    uint32_t height = static_cast<uint32_t>(h);

    vk::UniqueBuffer stagingBuffer = Buffer::createStagingBufferUnique(device, imageSize);
    vk::UniqueDeviceMemory stagingMemory = Buffer::createStagingMemoryUnique(physicalDevice, device, *stagingBuffer, imageSize);
    device.bindBufferMemory(*stagingBuffer, *stagingMemory, 0);

    void* ptr = device.mapMemory(*stagingMemory, 0, imageSize);
    memcpy(ptr, px, imageSize);
    device.unmapMemory(*stagingMemory);

    stbi_image_free(px);

    // image
    vk::ImageCreateInfo imageInfo {
        .imageType = vk::ImageType::e2D,
        .format = vk::Format::eR8G8B8A8Srgb,
        .extent = {
            .width = width,
            .height = height,
            .depth = 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined,
    };

    m_image = device.createImageUnique(imageInfo);

    vk::MemoryRequirements memoryRequirements = device.getImageMemoryRequirements(*m_image);

    vk::MemoryAllocateInfo memoryInfo {
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = Buffer::findMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
    };

    m_imageMemory = device.allocateMemoryUnique(memoryInfo);
    device.bindImageMemory(*m_image, *m_imageMemory, 0);

    transitionImageLayout(device, commandPool, graphicsQueue, *m_image, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

    vk::BufferImageCopy region {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1 },
        .imageOffset = { 0, 0, 0 },
        .imageExtent = { width, height, 1 }
    };

    vk::UniqueCommandBuffer commandBuffer = Device::beginSingleUseCommandBuffer(device, commandPool);
    commandBuffer->copyBufferToImage(*stagingBuffer, *m_image, vk::ImageLayout::eTransferDstOptimal, region);
    Device::endSingleUseCommandBuffer(*commandBuffer, graphicsQueue);

    transitionImageLayout(device, commandPool, graphicsQueue, *m_image, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

    // image view
    m_imageView = Swapchain::createImageViewUnique(device, *m_image, vk::Format::eR8G8B8A8Srgb);

    // sampler
    vk::SamplerCreateInfo samplerInfo {
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = physicalDevice.getProperties().limits.maxSamplerAnisotropy,
        .compareEnable = VK_FALSE,
        .compareOp = vk::CompareOp::eAlways,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = vk::BorderColor::eIntOpaqueBlack,
        .unnormalizedCoordinates = VK_FALSE
    };

    m_imageSampler = device.createSamplerUnique(samplerInfo);
}

vk::ImageView& Texture::imageView()
{
    return *m_imageView;
}

vk::Sampler& Texture::sampler()
{
    return *m_imageSampler;
}

}
