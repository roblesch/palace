#include "vulkan.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace graphics {

void Vulkan::init(SDL_Window* sdlWindow, bool enableValidation)
{
    // dynamic dispatcher
    auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    this->window = sdlWindow;
    this->validationEnabled = enableValidation;

    this->instance = VulkanInstance::create(window, validationEnabled);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance.get());
}

void Vulkan::cleanup()
{
}

}
