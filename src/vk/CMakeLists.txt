add_library(vk "vulkan.cpp")
target_link_libraries(vk
        util
        imgui
        glm::glm
        Vulkan::Vulkan
        SDL2::SDL2)
