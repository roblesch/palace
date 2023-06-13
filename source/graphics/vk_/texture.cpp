#include "texture.hpp"

#include "buffer.hpp"
#include "device.hpp"
#include "swapchain.hpp"

namespace vk_ {

Texture::Texture(vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::CommandPool& commandPool, vk::Queue& graphicsQueue, const unsigned char* px, vk::Extent2D extent)
{
    vk::DeviceSize imageSize = extent.width * extent.height * 4;
    vk::UniqueBuffer stagingBuffer = Buffer::createStagingBufferUnique(device, imageSize);
    vk::UniqueDeviceMemory stagingMemory = Buffer::createStagingMemoryUnique(physicalDevice, device, *stagingBuffer, imageSize);
    device.bindBufferMemory(*stagingBuffer, *stagingMemory, 0);

    void* ptr = device.mapMemory(*stagingMemory, 0, imageSize);
    memcpy(ptr, px, imageSize);
    device.unmapMemory(*stagingMemory);

    // image
    auto imageFormat = vk::Format::eR8G8B8A8Unorm;

    m_image = Swapchain::createImageUnique(device, extent, imageFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
    m_imageMemory = Buffer::createDeviceMemoryUnique(physicalDevice, device, device.getImageMemoryRequirements(*m_image), vk::MemoryPropertyFlagBits::eDeviceLocal);
    device.bindImageMemory(*m_image, *m_imageMemory, 0);

    Swapchain::transitionImageLayout(device, commandPool, graphicsQueue, *m_image, imageFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

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
        .imageExtent = { extent.width, extent.height, 1 }
    };

    vk::UniqueCommandBuffer commandBuffer = Device::beginSingleUseCommandBuffer(device, commandPool);
    commandBuffer->copyBufferToImage(*stagingBuffer, *m_image, vk::ImageLayout::eTransferDstOptimal, region);
    Device::endSingleUseCommandBuffer(*commandBuffer, graphicsQueue);

    Swapchain::transitionImageLayout(device, commandPool, graphicsQueue, *m_image, imageFormat, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

    // image view
    m_imageView = Swapchain::createImageViewUnique(device, *m_image, imageFormat, vk::ImageAspectFlagBits::eColor);

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
