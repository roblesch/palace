#pragma once

#include "types.hpp"
#include "vk_mem_alloc.h"

namespace pl {

struct VmaBuffer {
    size_t size;
    VkBuffer buffer;
    VmaAllocation allocation;
};

struct VmaImage {
    VkImage image;
    VmaAllocation allocation;
    uint32_t mipLevels;
};

struct MemoryHelperCreateInfo {
    vk::PhysicalDevice physicalDevice;
    vk::Device device;
    vk::Instance instance;
    vk::CommandPool commandPool;
    vk::Queue graphicsQueue;
};

class MemoryHelper {
public:
    explicit MemoryHelper(const MemoryHelperCreateInfo& createInfo);
    ~MemoryHelper();

    // TODO: move this
    vk::UniqueCommandBuffer beginSingleUseCommandBuffer();
    void endSingleUseCommandBuffer(vk::CommandBuffer& commandBuffer);

    VmaBuffer* createBuffer(size_t size, vk::BufferUsageFlags usage, VmaAllocationCreateFlags flags);
    void uploadToBuffer(VmaBuffer* buffer, void* src);
    void uploadToBufferDirect(VmaBuffer* buffer, void* src);
    VmaImage* createImage(vk::Extent3D extent, vk::Format format, vk::ImageUsageFlags usage, uint32_t mipLevels, vk::SampleCountFlagBits samples);
    VmaImage* createTextureImage(const void* src, size_t size, vk::Extent3D extent, uint32_t mipLevels);
    vk::UniqueImageView createImageViewUnique(vk::Image image, vk::Format format, vk::ImageAspectFlagBits aspectMask, uint32_t);
    vk::UniqueSampler createTextureSamplerUnique(uint32_t mipLevels);

private:
    VmaBuffer* createStagingBuffer(size_t size);
    vk::ImageMemoryBarrier imageTransitionBarrier(vk::Image image, vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels = 1);

    vk::PhysicalDevice physicalDevice_;
    vk::Device device_;
    vk::CommandPool commandPool_;
    vk::Queue graphicsQueue_;

    VmaAllocator allocator_ {};
    std::vector<VmaBuffer*> buffers_;
    std::vector<VmaImage*> images_;
};

using UniqueMemoryHelper = std::unique_ptr<MemoryHelper>;

UniqueMemoryHelper createMemoryHelperUnique(const MemoryHelperCreateInfo& createInfo);

}