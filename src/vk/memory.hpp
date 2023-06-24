#pragma once

#include "types.hpp"
#include "vk_mem_alloc.h"

namespace pl {

struct VmaBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
};

struct VmaImage {
    VkImage image;
    VmaAllocation allocation;
};

struct MemoryCreateInfo {
    vk::PhysicalDevice physicalDevice;
    vk::Device device;
    vk::Instance instance;
    vk::CommandPool commandPool;
    vk::Queue graphicsQueue;
};

class Memory {
public:
    Memory(const MemoryCreateInfo& createInfo);
    ~Memory();

    vk::UniqueCommandBuffer beginSingleUseCommandBuffer();
    void endSingleUseCommandBuffer(vk::CommandBuffer& commandBuffer);

    VmaBuffer* createBuffer(void* src, size_t size, vk::BufferUsageFlags usage);
    VmaImage* createTextureImage(const void* src, size_t size, vk::Extent3D  extent);

private:
    vk::Device device_;
    vk::CommandPool commandPool_;
    vk::Queue graphicsQueue_;

    VmaAllocator allocator_;
    std::vector<VmaBuffer*> buffers_;
    std::vector<VmaImage*> images_;
};

using UniqueMemory = std::unique_ptr<Memory>;

UniqueMemory createMemoryUnique(const MemoryCreateInfo& createInfo);

}
