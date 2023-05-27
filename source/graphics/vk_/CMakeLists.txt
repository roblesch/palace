add_library(vk_
        "debug.cpp"
        "debug.hpp"
        "device.cpp"
        "device.hpp"
        "include.hpp"
        "pipeline.cpp"
        "pipeline.hpp"
        "swapchain.cpp"
        "swapchain.hpp")

target_link_libraries(vk_ Vulkan::Vulkan SDL2::SDL2)
