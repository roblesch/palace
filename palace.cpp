#include "graphics/sdl2.hpp"
#include "graphics/vulkan.hpp"

using namespace pl;

uint32_t WIDTH = 800;
uint32_t HEIGHT = 600;

int main() {
    Vulkan vulkan;

    SDL_Window* window = Sdl2::createWindow();
    vulkan.init(window);

    while (true) {
        SDL_Event event;
        if (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                break;
        }
    }

    vulkan.cleanup();
    Sdl2::quit(window);

    return 0;
}
