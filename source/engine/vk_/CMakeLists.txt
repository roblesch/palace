add_library(vk_
        "buffer.hpp"
        "buffer.cpp"
        "debug.cpp"
        "debug.hpp"
        "device.cpp"
        "device.hpp"
        "include.hpp"
        "log.hpp"
        "pipeline.cpp"
        "pipeline.hpp"
        "primitive.hpp"
        "sdl2.hpp"
        "swapchain.cpp"
        "swapchain.hpp"
        "texture.hpp"
        "texture.cpp")
target_link_libraries(vk_ Vulkan::Vulkan SDL2::SDL2)
