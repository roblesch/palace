#ifndef PALACE_GRAPHICS_VULKANINSTANCE_HPP
#define PALACE_GRAPHICS_VULKANINSTANCE_HPP

#include "include.hpp"

#include "vulkandebug.hpp"

namespace graphics {

struct VulkanInstance {
private:
    vk::UniqueInstance instance;

public:
    static VulkanInstance create(SDL_Window* sdlWindow, bool validationEnabled);
    vk::Instance get();
};

}

#endif
