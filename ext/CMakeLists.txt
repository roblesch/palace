add_subdirectory(glm)
add_subdirectory(SDL)
add_subdirectory(tinygltf)
add_subdirectory(VulkanMemoryAllocator)
add_library(VMA::VMA ALIAS VulkanMemoryAllocator)
add_library(imgui
		"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_vulkan.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_sdl2.cpp")
add_library(imgui::imgui ALIAS imgui)
target_link_libraries(imgui Vulkan::Vulkan SDL2::SDL2)
