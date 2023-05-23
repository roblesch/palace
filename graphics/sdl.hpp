#ifndef PALACE_SDL_HPP
#define PALACE_SDL_HPP

#include <iostream>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_vulkan.h>

extern SDL_Window* window;
extern SDL_Surface* surface;

extern int SCREEN_WIDTH;
extern int SCREEN_HEIGHT;

namespace palace::sdl {
    void init() {
        SDL_Init(SDL_INIT_VIDEO);
        SDL_Vulkan_LoadLibrary(nullptr);

        window = SDL_CreateWindow("palace",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            SCREEN_WIDTH, SCREEN_HEIGHT,
            SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);

        if (window == nullptr) {
            SDL_Log("Failed to create window: %s", SDL_GetError());
            exit(1);
        }

        surface = SDL_GetWindowSurface(window);

        if (surface == nullptr) {
            SDL_Log("Failed to get surface: %s", SDL_GetError());
        }
    }

    void quit() {
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
}

#endif
