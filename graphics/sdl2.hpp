#ifndef PALACE_SDL2_HPP
#define PALACE_SDL2_HPP

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_vulkan.h>

extern uint32_t WIDTH;
extern uint32_t HEIGHT;

namespace pl {

class Sdl2 {
public:
    static SDL_Window* createWindow()
    {
        SDL_Init(SDL_INIT_VIDEO);
        SDL_Vulkan_LoadLibrary(nullptr);

        return SDL_CreateWindow("palace",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
    }

    static void quit(SDL_Window* window)
    {
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

}

#endif