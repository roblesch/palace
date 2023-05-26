#ifndef PALACE_GRAPHICS_VULKAN_HPP
#define PALACE_GRAPHICS_VULKAN_HPP

#include "vulkaninclude.hpp"
#include "vulkandebug.hpp"
#include "vulkandevice.hpp"
#include "vulkanswapchain.hpp"

extern int WIDTH;
extern int HEIGHT;

namespace graphics {

class Vulkan {
private:
    bool m_validation;
    SDL_Window* m_window;

    vk::DynamicLoader m_dynamicloader;
    vk::UniqueInstance m_instance;
    vk_::Device m_device;
    vk::UniqueSurfaceKHR m_surface;
    vk_::SwapChain m_swapchain;

public:
    Vulkan(bool enableValidation);
    ~Vulkan();

    void run();
};

}

#endif
