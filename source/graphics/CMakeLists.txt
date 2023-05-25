add_library(graphics
        "vulkan.cpp"
        "vulkan.hpp"
        "vulkandebug.cpp"
        "vulkandebug.hpp"
        "vulkandevice.cpp"
        "vulkandevice.hpp"
        "vulkaninclude.hpp")

target_link_libraries(graphics Vulkan::Vulkan SDL2::SDL2)
