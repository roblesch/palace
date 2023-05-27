add_subdirectory(vk_)

add_library(engine
        "vulkan.cpp"
        "vulkan.hpp")

target_link_libraries(engine
        Vulkan::Vulkan
        SDL2::SDL2
        vk_)
