add_subdirectory(vk_)

add_library(graphics
        "vulkan.cpp"
        "vulkan.hpp")

target_link_libraries(graphics
        Vulkan::Vulkan
        SDL2::SDL2
        vk_)
