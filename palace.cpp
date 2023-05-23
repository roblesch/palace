#include "graphics/sdl.hpp"
#include "graphics/vk.hpp"

using namespace palace;

SDL_Window* window;
SDL_Surface* surface;

vk::Instance instance;

int SCREEN_WIDTH = 800;
int SCREEN_HEIGHT = 600;

int main() {
    sdl::init();

    while (true)
    {
        SDL_Event event;
        if (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                break;
            }
        }
    }

    sdl::quit();

    return 0;
}
