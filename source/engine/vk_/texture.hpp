#ifndef PALACE_ENGINE_VK_TEXTURE_HPP
#define PALACE_ENGINE_VK_TEXTURE_HPP

#include "include.hpp"

namespace vk_ {

class Texture {
private:
    vk::UniqueImage m_uniqueImage;
    vk::UniqueDeviceMemory m_uniqueImageMemory;
    vk::UniqueImageView m_uniqueImageView;
    vk::UniqueSampler m_uniqueSampler;

public:
    Texture() = default;
    Texture(const char* path, vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::CommandPool& commandPool, vk::Queue& graphicsQueue);

    vk::ImageView& imageView();
    vk::Sampler& sampler();
};

}

#endif
