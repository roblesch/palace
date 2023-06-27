add_library(vk "vulkan.cpp" "gltf.cpp" "memory.cpp" "pipelines.cpp")
add_library(pl::vk ALIAS vk)
target_link_libraries(vk imgui::imgui glm::glm pl::util VMA::VMA Vulkan::Vulkan SDL2::SDL2 tinygltf)
