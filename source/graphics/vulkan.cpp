#include "vulkan.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace graphics {

Vulkan::Vulkan(bool enableValidation)
{
    m_validation = enableValidation;

    // sdl2 initialization
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Vulkan_LoadLibrary(nullptr);
    m_window = SDL_CreateWindow("palace",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);

    // setup dynamic dispatcher
    auto vkGetInstanceProcAddr = m_dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    // create instance
    m_instance = vulkan::instance(m_window, m_validation);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance.get());

    // create device
    m_device = vulkan::device(m_instance.get());

    // create surface
    // TODO: vulkan-hpp uniquesurface destructor fails
    SDL_Vulkan_CreateSurface(m_window, m_instance.get(), &m_surface);
}

Vulkan::~Vulkan()
{
    vkDestroySurfaceKHR(m_instance.get(), m_surface, nullptr);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void Vulkan::run()
{
    while (true) {
        SDL_Event event;
        if (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                break;
        }
    }
}

}
