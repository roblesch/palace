add_library(graphics
        "sdl2.hpp"
        "vulkan.cpp"
        "vulkan.hpp"
        "vulkandebug.cpp"
        "vulkandebug.hpp"
        "vulkandevice.cpp"
        "vulkandevice.hpp"
        "vulkaninstance.cpp"
        "vulkaninstance.hpp")

target_link_libraries(graphics Vulkan::Vulkan SDL2::SDL2)
