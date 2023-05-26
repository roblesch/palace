#ifndef PALACE_GRAPHICS_VULKAN_HPP
#define PALACE_GRAPHICS_VULKAN_HPP

#include "vulkandebug.hpp"
#include "vulkandevice.hpp"
#include "vulkaninclude.hpp"
#include "vulkanpipeline.hpp"
#include "vulkanswapchain.hpp"

extern int WIDTH;
extern int HEIGHT;

namespace graphics {

class Vulkan {
private:
    bool m_validation;

    SDL_Window* m_window;
    vk::DynamicLoader m_dynamicLoader;
    vk::UniqueInstance m_uniqueInstance;
    vk_::Device m_device;
    vk::UniqueSurfaceKHR m_uniqueSurface;
    vk_::Swapchain m_swapchain;
    vk_::Pipeline m_pipeline;

public:
    Vulkan(bool enableValidation);
    ~Vulkan();

    void run();
};

}

#endif
