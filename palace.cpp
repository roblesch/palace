#include "graphics/sdl2.hpp"
#include "graphics/vulkan.hpp"

int WIDTH = 800;
int HEIGHT = 600;

int main() {
    graphics::Vulkan vulkan;

    SDL_Window* window = graphics::createSdl2Window();
    vulkan.init(window, true);

    while (true) {
        SDL_Event event;
        if (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                break;
        }
    }

    vulkan.cleanup();
    graphics::quitSdl2(window);

    return 0;
}
