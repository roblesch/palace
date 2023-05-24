#ifndef PALACE_GRAPHICS_VULKANINSTANCE_HPP
#define PALACE_GRAPHICS_VULKANINSTANCE_HPP

#include "include.hpp"

#include "vulkandebug.hpp"

namespace graphics::vulkan {

struct instance {
private:
    vk::UniqueInstance m_vkinstance;

public:
    instance();
    instance(SDL_Window* sdlWindow, bool validationEnabled);

    vk::Instance get();
};

}

#endif
