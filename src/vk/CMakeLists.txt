add_library(vk "camera.hpp" "gltf.hpp" "gltf.cpp" "memory.hpp" "memory.cpp" "types.hpp" "vulkan.hpp" "vulkan.cpp")
add_library(pl::vk ALIAS vk)
target_link_libraries(vk imgui::imgui glm::glm pl::util VMA::VMA Vulkan::Vulkan SDL2::SDL2 tinygltf)
