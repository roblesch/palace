#include "vulkan.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace graphics {

Vulkan::Vulkan(SDL_Window* sdlWindow, bool enableValidation)
{
    m_window = sdlWindow;
    m_validation = enableValidation;

    // setup dynamic dispatcher
    auto vkGetInstanceProcAddr = m_dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    // instance
    m_instance = vulkan::instance(m_window, m_validation);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance.get());

    // device
    m_device = vulkan::device(m_instance.get());

    // surface
    // TODO: vulkan-hpp uniquesurface destructor fails
    SDL_Vulkan_CreateSurface(m_window, m_instance.get(), &m_surface);
}

void Vulkan::cleanup()
{
    vkDestroySurfaceKHR(m_instance.get(), m_surface, nullptr);
}

}
