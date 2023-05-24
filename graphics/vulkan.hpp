#ifndef PALACE_GRAPHICS_VULKAN_HPP
#define PALACE_GRAPHICS_VULKAN_HPP

#include "include.hpp"

#include "vulkaninstance.hpp"

namespace graphics {

class Vulkan {
public:
    void init(SDL_Window* sdlWindow, bool enableValidation);
    void cleanup();

private:
    SDL_Window* window;
    bool validationEnabled;

    vk::DynamicLoader dl;
    VulkanInstance instance;
};

}

#endif
