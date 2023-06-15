add_library(vk "vulkan.cpp")
add_library(pl::vk ALIAS vk)
target_link_libraries(vk imgui glm::glm pl::util Vulkan::Vulkan SDL2::SDL2)
