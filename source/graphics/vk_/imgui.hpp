#pragma once

#include "include.hpp"

#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_vulkan.h>

namespace vk_ {

class Imgui {
private:
    vk::UniqueDescriptorPool m_descriptorPool;

public:
    Imgui() = default;
    Imgui(SDL_Window* window, vk::Instance& instance, vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::Queue& graphicsQueue, vk::RenderPass& renderPass, vk::CommandPool& commandPool);

    void cleanup();
};

}