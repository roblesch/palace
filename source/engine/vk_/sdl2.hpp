#ifndef PALACE_ENGINE_SDL2_HPP
#define PALACE_ENGINE_SDL2_HPP

#include "include.hpp"

namespace sdl2 {

SDL_Window* createWindow(int width, int height)
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Vulkan_LoadLibrary(nullptr);
    return SDL_CreateWindow("palace", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
}

std::vector<const char*> getInstanceExtensions(SDL_Window* window)
{
    unsigned int extensionCount;
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
    std::vector<const char*> extensions(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensions.data());
    return extensions;
}

}

#endif
