#pragma once

#include "include.hpp"

namespace vk_ {

class Texture {
private:
    vk::UniqueImage m_image;
    vk::UniqueDeviceMemory m_imageMemory;
    vk::UniqueImageView m_imageView;
    vk::UniqueSampler m_imageSampler;

public:
    Texture() = default;
    Texture(vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::CommandPool& commandPool, vk::Queue& graphicsQueue, const unsigned char* px, vk::Extent2D extent);

    vk::ImageView& imageView();
    vk::Sampler& sampler();
};

}
