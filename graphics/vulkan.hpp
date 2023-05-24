#ifndef PALACE_GRAPHICS_VULKAN_HPP
#define PALACE_GRAPHICS_VULKAN_HPP

#include "include.hpp"

#include "vulkandevice.hpp"
#include "vulkaninstance.hpp"

namespace graphics {

class Vulkan {
private:
    SDL_Window* m_window;
    bool m_validation;

    vk::DynamicLoader m_dl;
    vulkan::instance m_instance;
    vulkan::device m_device;

    VkSurfaceKHR m_surface;

public:
    Vulkan(SDL_Window* sdlWindow, bool enableValidation);
    void cleanup();
};

}

#endif
