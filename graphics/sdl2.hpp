#ifndef PALACE_GRAPHICS_SDL2_HPP
#define PALACE_GRAPHICS_SDL2_HPP

#include "include.hpp"

extern int WIDTH;
extern int HEIGHT;

namespace graphics {

SDL_Window* createSdl2Window()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Vulkan_LoadLibrary(nullptr);

    return SDL_CreateWindow("palace",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
}

void quitSdl2(SDL_Window* window)
{
    SDL_DestroyWindow(window);
    SDL_Quit();
}

}

#endif