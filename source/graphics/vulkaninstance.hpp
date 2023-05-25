#ifndef PALACE_GRAPHICS_VULKANINSTANCE_HPP
#define PALACE_GRAPHICS_VULKANINSTANCE_HPP

#include "vulkaninclude.hpp"
#include "vulkandebug.hpp"

namespace graphics::vulkan {

struct Instance {
private:
    vk::UniqueInstance m_vkinstance;

public:
    Instance() = default;
    Instance(SDL_Window* sdlWindow, bool validationEnabled);

    vk::Instance get();
};

}

#endif
