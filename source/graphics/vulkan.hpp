#ifndef PALACE_GRAPHICS_VULKAN_HPP
#define PALACE_GRAPHICS_VULKAN_HPP

#include "vulkaninclude.hpp"

#include "vulkandevice.hpp"
#include "vulkaninstance.hpp"

extern int WIDTH;
extern int HEIGHT;

namespace graphics {

class Vulkan {
private:
    SDL_Window* m_window;
    bool m_validation;

    vk::DynamicLoader m_dl;
    vulkan::instance m_instance;
    vulkan::device m_device;

    vk::UniqueSurfaceKHR m_surface;

public:
    Vulkan(bool enableValidation);
    ~Vulkan();

    void run();
};

}

#endif
