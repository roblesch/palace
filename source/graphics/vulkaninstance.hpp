#ifndef PALACE_GRAPHICS_VULKANINSTANCE_HPP
#define PALACE_GRAPHICS_VULKANINSTANCE_HPP

#include "vulkaninclude.hpp"
#include "vulkandebug.hpp"

namespace graphics::vulkan {

struct instance {
private:
    vk::UniqueInstance m_vkinstance;

public:
    instance() = default;
    instance(SDL_Window* sdlWindow, bool validationEnabled);

    vk::Instance get();
};

}

#endif
